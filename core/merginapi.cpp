/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "merginapi.h"

#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDate>
#include <QByteArray>
#include <QSet>
#include <QUuid>
#include <QtMath>
#include <QElapsedTimer>

#include "projectchecksumcache.h"
#include "coreutils.h"
#include "geodiffutils.h"
#include "localprojectsmanager.h"
#include "../app/enumhelper.h"
#include "merginerrortypes.h"

#include <geodiff.h>

const QString MerginApi::sMetadataFile = QStringLiteral( "/.mergin/mergin.json" );
const QString MerginApi::sMetadataFolder = QStringLiteral( ".mergin" );
const QString MerginApi::sMerginConfigFile = QStringLiteral( "mergin-config.json" );
const QString MerginApi::sMarketingPageRoot = QStringLiteral( "https://merginmaps.com/" );
const QString MerginApi::sDefaultApiRoot = QStringLiteral( "https://app.merginmaps.com/" );
const QSet<QString> MerginApi::sIgnoreExtensions = QSet<QString>() << "gpkg-shm" << "gpkg-wal" << "qgs~" << "qgz~" << "pyc" << "swap";
const QSet<QString> MerginApi::sIgnoreImageExtensions = QSet<QString>() << "jpg" << "jpeg" << "png";
const QSet<QString> MerginApi::sIgnoreFiles = QSet<QString>() << "mergin.json" << ".DS_Store";
const int MerginApi::UPLOAD_CHUNK_SIZE = 10 * 1024 * 1024; // Should be the same as on Mergin server
const QString MerginApi::sSyncCanceledMessage = QObject::tr( "Synchronisation canceled" );


MerginApi::MerginApi( LocalProjectsManager &localProjects, QObject *parent )
  : QObject( parent )
  , mLocalProjects( localProjects )
  , mDataDir( localProjects.dataDir() )
  , mUserInfo( new MerginUserInfo )
  , mWorkspaceInfo( new MerginWorkspaceInfo )
  , mSubscriptionInfo( new MerginSubscriptionInfo )
  , mUserAuth( new MerginUserAuth )
{
  // load cached data if there are any
  QSettings cache;
  if ( cache.contains( QStringLiteral( "Input/apiRoot" ) ) )
  {
    loadCache();
  }
  else
  {
    // set default api root
    setApiRoot( defaultApiRoot() );
  }

  qRegisterMetaType<Transactions>();

  QObject::connect( this, &MerginApi::authChanged, this, &MerginApi::saveAuthData );
  QObject::connect( this, &MerginApi::apiRootChanged, this, &MerginApi::pingMergin );
  QObject::connect( this, &MerginApi::apiRootChanged, this, &MerginApi::getServerConfig );
  QObject::connect( this, &MerginApi::pingMerginFinished, this, &MerginApi::checkMerginVersion );
  QObject::connect( this, &MerginApi::workspaceCreated, this, &MerginApi::getUserInfo );
  QObject::connect( this, &MerginApi::serverTypeChanged, this, [this]()
  {
    if ( mUserAuth->hasAuthData() )
    {
      // do not call /user/profile when user just logged out
      getUserInfo();
    }
  } );
  QObject::connect( this, &MerginApi::processInvitationFinished, this, &MerginApi::getUserInfo );
  QObject::connect( this, &MerginApi::getWorkspaceInfoFinished, this, &MerginApi::getServiceInfo );
  QObject::connect( mUserInfo, &MerginUserInfo::userInfoChanged, this, &MerginApi::userInfoChanged );
  QObject::connect( mUserInfo, &MerginUserInfo::activeWorkspaceChanged, this, &MerginApi::activeWorkspaceChanged );
  QObject::connect( mUserInfo, &MerginUserInfo::activeWorkspaceChanged, this, &MerginApi::getWorkspaceInfo );
  QObject::connect( mUserInfo, &MerginUserInfo::hasWorkspacesChanged, this, &MerginApi::hasWorkspacesChanged );
  QObject::connect( mSubscriptionInfo, &MerginSubscriptionInfo::subscriptionInfoChanged, this, &MerginApi::subscriptionInfoChanged );
  QObject::connect( mSubscriptionInfo, &MerginSubscriptionInfo::planProductIdChanged, this, &MerginApi::onPlanProductIdChanged );
  QObject::connect( mUserAuth, &MerginUserAuth::authChanged, this, &MerginApi::authChanged );
  QObject::connect( mUserAuth, &MerginUserAuth::authChanged, this, [this]()
  {
    if ( mUserAuth->hasValidToken() )
    {
      // do not call /user/profile when user just logged out
      getUserInfo();
    }
  } );

  //
  // check if the cache is up to date:
  //  - server url and type
  //  - user auth and info
  //  - workspace info
  //

  getServerConfig();
  pingMergin();

  if ( mUserAuth->hasAuthData() )
  {
    QObject::connect( this, &MerginApi::pingMerginFinished, this, &MerginApi::getUserInfo, Qt::SingleShotConnection );
    QObject::connect( this, &MerginApi::userInfoReplyFinished, this, &MerginApi::getWorkspaceInfo, Qt::SingleShotConnection );
  }
}

void MerginApi::loadCache()
{
  QSettings settings;
  mApiRoot = settings.value( QStringLiteral( "Input/apiRoot" ) ).toString();
  int serverType = settings.value( QStringLiteral( "Input/serverType" ) ).toInt();

  mServerType = static_cast<MerginServerType::ServerType>( serverType );

  mUserAuth->loadAuthData();
  mUserInfo->loadWorkspacesData();
}

MerginUserAuth *MerginApi::userAuth() const
{
  return mUserAuth;
}

MerginUserInfo *MerginApi::userInfo() const
{
  return mUserInfo;
}

MerginWorkspaceInfo *MerginApi::workspaceInfo() const
{
  return mWorkspaceInfo;
}

MerginSubscriptionInfo *MerginApi::subscriptionInfo() const
{
  return mSubscriptionInfo;
}

QString MerginApi::listProjects( const QString &searchExpression, const QString &flag, const int page )
{
  bool authorize = flag != "public";

  if ( ( authorize && !validateAuth() ) || mApiVersionStatus != MerginApiStatus::OK )
  {
    emit listProjectsFailed();
    return QString();
  }

  QUrlQuery query;

  if ( flag == "workspace" )
  {
    if ( mUserInfo->activeWorkspaceId() < 0 )
    {
      emit listProjectsFailed();
      return QString();
    }

    query.addQueryItem( "only_namespace", mUserInfo->activeWorkspaceName() );
  }
  else if ( flag == "created" )
  {
    query.addQueryItem( "flag", "created" );
  }
  else if ( flag == "shared" )
  {
    query.addQueryItem( "flag", "shared" );
  }
  else if ( flag == "public" )
  {
    query.addQueryItem( "only_public", "true" );
  }

  if ( !searchExpression.isEmpty() )
  {
    query.addQueryItem( "name", searchExpression.toUtf8().toPercentEncoding() );
  }

  query.addQueryItem( "order_params", QStringLiteral( "namespace_asc,name_asc" ) );

  // Required query parameters
  query.addQueryItem( "page", QString::number( page ) );
  query.addQueryItem( "per_page", QString::number( PROJECT_PER_PAGE ) );

  QUrl url( mApiRoot + QStringLiteral( "/v1/project/paginated" ) );
  url.setQuery( query );

  // Even if the authorization is not required, it can be include to fetch more results
  QNetworkRequest request = getDefaultRequest( mUserAuth->hasAuthData() );
  request.setUrl( url );

  QString requestId = CoreUtils::uuidWithoutBraces( QUuid::createUuid() );

  QNetworkReply *reply = mManager.get( request );
  CoreUtils::log( "list projects", QStringLiteral( "Requesting: " ) + url.toString() );
  connect( reply, &QNetworkReply::finished, this, [this, requestId]() {this->listProjectsReplyFinished( requestId );} );

  return requestId;
}

QString MerginApi::listProjectsByName( const QStringList &projectNames )
{
  if ( mApiVersionStatus != MerginApiStatus::OK )
  {
    emit listProjectsFailed();
    return QLatin1String();
  }

  const int listProjectsByNameApiLimit = 50;
  QStringList projectNamesToRequest( projectNames );

  if ( projectNamesToRequest.count() > listProjectsByNameApiLimit )
  {
    CoreUtils::log( "list projects by name", QStringLiteral( "Too many local projects: " ) + QString::number( projectNames.count(), 'f', 0 ) );
    const int projectsToRemoveCount = projectNames.count() - listProjectsByNameApiLimit;
    QString msg = tr( "Please remove some projects as the app currently\nonly allows up to %1 downloaded projects." ).arg( listProjectsByNameApiLimit );
    notifyInfo( msg );
    projectNamesToRequest.erase( projectNamesToRequest.begin() + listProjectsByNameApiLimit, projectNamesToRequest.end() );
    Q_ASSERT( projectNamesToRequest.count() == listProjectsByNameApiLimit );
  }

  // Authentification is optional in this case, as there might be public projects without the need to be logged in.
  // We only want to include auth token when user is logged in.
  // User's token, however, might have already expired, so let's just refresh it.
  refreshAuthToken();

  // construct JSON body
  QJsonDocument body;
  QJsonObject projects;
  QJsonArray projectsArr = QJsonArray::fromStringList( projectNamesToRequest );

  projects.insert( "projects", projectsArr );
  body.setObject( projects );

  QUrl url( mApiRoot + QStringLiteral( "/v1/project/by_names" ) );

  QNetworkRequest request = getDefaultRequest( true );
  request.setUrl( url );
  request.setRawHeader( "Content-type", "application/json" );

  QString requestId = CoreUtils::uuidWithoutBraces( QUuid::createUuid() );

  QNetworkReply *reply = mManager.post( request, body.toJson() );
  CoreUtils::log( "list projects by name", QStringLiteral( "Requesting: " ) + url.toString() );
  connect( reply, &QNetworkReply::finished, this, [this, requestId]() {this->listProjectsByNameReplyFinished( requestId );} );

  return requestId;
}


void MerginApi::downloadNextItem( const QString &projectFullName )
{
  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  Q_ASSERT( !transaction.downloadQueue.isEmpty() );

  DownloadQueueItem item = transaction.downloadQueue.takeFirst();

  QUrl url( mApiRoot + QStringLiteral( "/v1/project/raw/" ) + projectFullName );
  QUrlQuery query;
  // Handles special chars in a filePath (e.g prevents to convert "+" sign into a space)
  query.addQueryItem( "file", item.filePath.toUtf8().toPercentEncoding() );
  query.addQueryItem( "version", QStringLiteral( "v%1" ).arg( item.version ) );
  if ( item.downloadDiff )
    query.addQueryItem( "diff", "true" );
  url.setQuery( query );

  QNetworkRequest request = getDefaultRequest();
  request.setUrl( url );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ), projectFullName );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrTempFileName ), item.tempFileName );

  QString range;
  if ( item.rangeFrom != -1 && item.rangeTo != -1 )
  {
    range = QStringLiteral( "bytes=%1-%2" ).arg( item.rangeFrom ).arg( item.rangeTo );
    request.setRawHeader( "Range", range.toUtf8() );
  }

  QNetworkReply *reply = mManager.get( request );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::downloadItemReplyFinished );
  transaction.replyPullItems.insert( reply );

  CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Requesting item: " ) + url.toString() +
                  ( !range.isEmpty() ? " Range: " + range : QString() ) );
}

void MerginApi::removeProjectsTempFolder( const QString &projectNamespace, const QString &projectName )
{
  if ( projectNamespace.isEmpty() || projectName.isEmpty() )
    return; // otherwise we could remove enitre users temp or entire .temp

  QString path = getTempProjectDir( getFullProjectName( projectNamespace, projectName ) );
  QDir( path ).removeRecursively();
}

QNetworkRequest MerginApi::getDefaultRequest( bool withAuth )
{
  QNetworkRequest request;
  QString info = CoreUtils::appInfo();
  request.setRawHeader( "User-Agent", QByteArray( info.toUtf8() ) );
  QString deviceId = CoreUtils::deviceUuid();
  request.setRawHeader( "X-Device-Id", QByteArray( deviceId.toUtf8() ) );
  if ( withAuth )
    request.setRawHeader( "Authorization", QByteArray( "Bearer " + mUserAuth->authToken() ) );

  return request;
}

bool MerginApi::projectFileHasBeenUpdated( const ProjectDiff &diff )
{
  for ( QString filePath : diff.remoteAdded )
  {
    if ( CoreUtils::hasProjectFileExtension( filePath ) )
      return true;
  }

  for ( QString filePath : diff.remoteUpdated )
  {
    if ( CoreUtils::hasProjectFileExtension( filePath ) )
      return true;
  }

  return false;
}

bool MerginApi::supportsSelectiveSync() const
{
  return mSupportsSelectiveSync;
}

void MerginApi::setSupportsSelectiveSync( bool supportsSelectiveSync )
{
  mSupportsSelectiveSync = supportsSelectiveSync;
}

bool MerginApi::apiSupportsSubscriptions() const
{
  return mApiSupportsSubscriptions;
}

void MerginApi::setApiSupportsSubscriptions( bool apiSupportsSubscriptions )
{
  if ( mApiSupportsSubscriptions != apiSupportsSubscriptions )
  {
    mApiSupportsSubscriptions = apiSupportsSubscriptions;
    emit apiSupportsSubscriptionsChanged();
  }
}

#if !defined(USE_MERGIN_DUMMY_API_KEY)
#include "merginsecrets.cpp"
#endif

QString MerginApi::getApiKey( const QString &serverName )
{
#if defined(USE_MERGIN_DUMMY_API_KEY)
  Q_UNUSED( serverName );
#else
  QString secretKey = __getSecretApiKey( serverName );
  if ( !secretKey.isEmpty() )
    return secretKey;
#endif
  return "not-secret-key";
}

void MerginApi::downloadItemReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();
  QString tempFileName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrTempFileName ) ).toString();

  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];
  Q_ASSERT( transaction.replyPullItems.contains( r ) );

  if ( r->error() == QNetworkReply::NoError )
  {
    QByteArray data = r->readAll();

    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Downloaded item (%1 bytes)" ).arg( data.size() ) );

    QString tempFolder = getTempProjectDir( projectFullName );
    QString tempFilePath = tempFolder + "/" + tempFileName;
    createPathIfNotExists( tempFilePath );

    // save to a tmp file, assemble at the end
    QFile file( tempFilePath );
    if ( file.open( QIODevice::WriteOnly ) )
    {
      file.write( data );
      file.close();
    }
    else
    {
      CoreUtils::log( "pull " + projectFullName, "Failed to open for writing: " + file.fileName() );
    }

    transaction.transferedSize += data.size();
    emit syncProjectStatusChanged( projectFullName, transaction.transferedSize / transaction.totalSize );

    transaction.replyPullItems.remove( r );
    r->deleteLater();

    if ( !transaction.downloadQueue.isEmpty() )
    {
      // one request finished, let's start another one
      downloadNextItem( projectFullName );
    }
    else if ( transaction.replyPullItems.isEmpty() )
    {
      // nothing else to download and all requests are finished, we're done
      finalizeProjectPull( projectFullName );
    }
    else
    {
      // no more requests to start, but there are pending requests - let's do nothing and wait
    }
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    if ( serverMsg.isEmpty() )
    {
      if ( r->error() == QNetworkReply::OperationCanceledError )
        serverMsg = sSyncCanceledMessage;
      else
        serverMsg = r->errorString();
    }
    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );

    transaction.replyPullItems.remove( r );
    r->deleteLater();

    if ( !transaction.pullItemsAborting )
    {
      // the first failed request will abort all the other pending requests too, and finish pull with error
      abortPullItems( projectFullName );

      // signal a networking error - we may retry
      int httpCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
      emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: downloadFile" ), httpCode, projectFullName );
    }
    else
    {
      // do nothing more: we are already aborting requests and handling finalization in abortPullItems()
    }
  }
}

void MerginApi::abortPullItems( const QString &projectFullName )
{
  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  transaction.pullItemsAborting = true;

  CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Aborting pending downloads" ) );
  for ( QNetworkReply *r : transaction.replyPullItems )
    r->abort();  // abort will trigger downloadItemReplyFinished slot

  // get rid of the temporary download dir where we may have left some downloaded files
  QDir( getTempProjectDir( projectFullName ) ).removeRecursively();

  if ( transaction.firstTimeDownload )
  {
    Q_ASSERT( !transaction.projectDir.isEmpty() );
    QDir( transaction.projectDir ).removeRecursively();
  }

  finishProjectSync( projectFullName, false );
}

void MerginApi::cacheServerConfig()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();

  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];
  Q_ASSERT( r == transaction.replyPullServerConfig );

  if ( r->error() == QNetworkReply::NoError )
  {
    QByteArray data = r->readAll();

    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Downloaded mergin config (%1 bytes)" ).arg( data.size() ) );
    transaction.config = MerginConfig::fromJson( data );

    transaction.replyPullServerConfig->deleteLater();
    transaction.replyPullServerConfig = nullptr;

    prepareDownloadConfig( projectFullName, true );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    if ( serverMsg.isEmpty() )
    {
      serverMsg = r->errorString();
    }
    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Failed to cache mergin config - %1. %2" ).arg( r->errorString(), serverMsg ) );

    transaction.replyPullServerConfig->deleteLater();
    transaction.replyPullServerConfig = nullptr;

    // get rid of the temporary download dir where we may have left some downloaded files
    CoreUtils::removeDir( getTempProjectDir( projectFullName ) );

    if ( transaction.firstTimeDownload )
    {
      CoreUtils::removeDir( transaction.projectDir );
    }

    int httpCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: downloadFile" ), httpCode, projectFullName );

    finishProjectSync( projectFullName, false );
  }
}


void MerginApi::pushFile( const QString &projectFullName, const QString &transactionUUID, MerginFile file, int chunkNo )
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  QString chunkID = file.chunks.at( chunkNo );

  QString filePath;
  if ( file.diffName.isEmpty() )
    filePath = transaction.projectDir + "/" + file.path;
  else  // use diff file instead of full file
    filePath = transaction.projectDir + "/.mergin/" + file.diffName;

  QFile f( filePath );
  QByteArray data;

  if ( f.open( QIODevice::ReadOnly ) )
  {
    f.seek( chunkNo * UPLOAD_CHUNK_SIZE );
    data = f.read( UPLOAD_CHUNK_SIZE );
  }

  QNetworkRequest request = getDefaultRequest();
  QUrl url( mApiRoot + QStringLiteral( "/v1/project/push/chunk/%1/%2" ).arg( transactionUUID, chunkID ) );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/octet-stream" );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ), projectFullName );

  Q_ASSERT( !transaction.replyPushFile );
  transaction.replyPushFile = mManager.post( request, data );
  connect( transaction.replyPushFile, &QNetworkReply::finished, this, &MerginApi::pushFileReplyFinished );

  CoreUtils::log( "push " + projectFullName, QStringLiteral( "Uploading item: " ) + url.toString() );
}

void MerginApi::pushStart( const QString &projectFullName, const QByteArray &json )
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  QNetworkRequest request = getDefaultRequest();
  QUrl url( mApiRoot + QStringLiteral( "v1/project/push/%1" ).arg( projectFullName ) );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/json" );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ), projectFullName );

  Q_ASSERT( !transaction.replyPushStart );
  transaction.replyPushStart = mManager.post( request, json );
  connect( transaction.replyPushStart, &QNetworkReply::finished, this, &MerginApi::pushStartReplyFinished );

  CoreUtils::log( "push " + projectFullName, QStringLiteral( "Starting push request: " ) + url.toString() );
}

void MerginApi::cancelPush( const QString &projectFullName )
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  if ( !mTransactionalStatus.contains( projectFullName ) )
    return;

  CoreUtils::log( "push " + projectFullName, QStringLiteral( "User requested cancel" ) );

  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  // There is an open transaction, abort it followed by calling cancelUpload again.
  if ( transaction.replyPushProjectInfo )
  {
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "Aborting project info request" ) );
    transaction.replyPushProjectInfo->abort();  // will trigger uploadInfoReplyFinished slot and emit sync finished
  }
  else if ( transaction.replyPushStart )
  {
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "Aborting upload start" ) );
    transaction.replyPushStart->abort();  // will trigger uploadStartReplyFinished slot and emit sync finished
  }
  else if ( transaction.replyPushFile )
  {
    QString transactionUUID = transaction.transactionUUID;  // copy transaction uuid as the transaction object will be gone after abort
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "Aborting upload file" ) );
    transaction.replyPushFile->abort();  // will trigger pushFileReplyFinished slot and emit sync finished

    // also need to cancel the transaction
    sendPushCancelRequest( projectFullName, transactionUUID );
  }
  else if ( transaction.replyPushFinish )
  {
    QString transactionUUID = transaction.transactionUUID;  // copy transaction uuid as the transaction object will be gone after abort
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "Aborting upload finish" ) );
    transaction.replyPushFinish->abort();  // will trigger pushFinishReplyFinished slot and emit sync finished

    sendPushCancelRequest( projectFullName, transactionUUID );
  }
  else
  {
    Q_ASSERT( false );  // unexpected state
  }
}


void MerginApi::sendPushCancelRequest( const QString &projectFullName, const QString &transactionUUID )
{
  QNetworkRequest request = getDefaultRequest();
  QUrl url( mApiRoot + QStringLiteral( "v1/project/push/cancel/%1" ).arg( transactionUUID ) );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/json" );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ), projectFullName );

  QNetworkReply *reply = mManager.post( request, QByteArray() );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::pushCancelReplyFinished );
  CoreUtils::log( "push " + projectFullName, QStringLiteral( "Requesting upload transaction cancel: " ) + url.toString() );
}

void MerginApi::cancelPull( const QString &projectFullName )
{
  if ( !mTransactionalStatus.contains( projectFullName ) )
    return;

  CoreUtils::log( "pull " + projectFullName, QStringLiteral( "User requested cancel" ) );

  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  if ( transaction.replyPullProjectInfo )
  {
    // we're still fetching project info
    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Aborting project info request" ) );
    transaction.replyPullProjectInfo->abort();  // abort will trigger pullInfoReplyFinished() slot
  }
  else if ( transaction.replyPullServerConfig )
  {
    // we're getting server config info
    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Aborting server config download" ) );
    transaction.replyPullServerConfig->abort();  // abort will trigger cacheServerConfig slot
  }
  else if ( !transaction.replyPullItems.isEmpty() )
  {
    // we're already downloading some files
    abortPullItems( projectFullName );
  }
  else
  {
    Q_ASSERT( false );  // unexpected state
  }
}

void MerginApi::pushFinish( const QString &projectFullName, const QString &transactionUUID )
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  QNetworkRequest request = getDefaultRequest();
  QUrl url( mApiRoot + QStringLiteral( "v1/project/push/finish/%1" ).arg( transactionUUID ) );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/json" );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ), projectFullName );

  Q_ASSERT( !transaction.replyPushFinish );
  transaction.replyPushFinish = mManager.post( request, QByteArray() );
  connect( transaction.replyPushFinish, &QNetworkReply::finished, this, &MerginApi::pushFinishReplyFinished );

  CoreUtils::log( "push " + projectFullName, QStringLiteral( "Requesting transaction finish: " ) + transactionUUID );
}

bool MerginApi::pullProject( const QString &projectNamespace, const QString &projectName, bool withAuth )
{
  QString projectFullName = getFullProjectName( projectNamespace, projectName );
  bool pullHasStarted = false;

  CoreUtils::log( "pull " + projectFullName, "### Starting ###" );

  QNetworkReply *reply = getProjectInfo( projectFullName, withAuth );
  if ( reply )
  {
    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Requesting project info: " ) + reply->request().url().toString() );

    Q_ASSERT( !mTransactionalStatus.contains( projectFullName ) );
    mTransactionalStatus.insert( projectFullName, TransactionStatus() );
    mTransactionalStatus[projectFullName].replyPullProjectInfo = reply;
    mTransactionalStatus[projectFullName].configAllowed = mSupportsSelectiveSync;
    mTransactionalStatus[projectFullName].type = TransactionStatus::Pull;

    emit syncProjectStatusChanged( projectFullName, 0 );

    connect( reply, &QNetworkReply::finished, this, &MerginApi::pullInfoReplyFinished );
    pullHasStarted = true;
  }
  else
  {
    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "FAILED to create project info request!" ) );
  }

  return pullHasStarted;
}

bool MerginApi::pushProject( const QString &projectNamespace, const QString &projectName, bool isInitialPush )
{
  QString projectFullName = getFullProjectName( projectNamespace, projectName );
  bool pushHasStarted = false;

  CoreUtils::log( "push " + projectFullName, "### Starting ###" );

  QNetworkReply *reply = getProjectInfo( projectFullName );
  if ( reply )
  {
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "Requesting project info: " ) + reply->request().url().toString() );

    // create entry about pending upload for the project
    Q_ASSERT( !mTransactionalStatus.contains( projectFullName ) );
    mTransactionalStatus.insert( projectFullName, TransactionStatus() );
    mTransactionalStatus[projectFullName].replyPushProjectInfo = reply;
    mTransactionalStatus[projectFullName].isInitialPush = isInitialPush;
    mTransactionalStatus[projectFullName].configAllowed = mSupportsSelectiveSync;
    mTransactionalStatus[projectFullName].type = TransactionStatus::Push;

    emit syncProjectStatusChanged( projectFullName, 0 );

    connect( reply, &QNetworkReply::finished, this, &MerginApi::pushInfoReplyFinished );
    pushHasStarted = true;
  }
  else
  {
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "FAILED to create project info request!" ) );
  }

  return pushHasStarted;
}

void MerginApi::authorize( const QString &login, const QString &password )
{
  if ( login.isEmpty() || password.isEmpty() )
  {
    emit authFailed();
    emit notifyError( QStringLiteral( "Please enter your login details" ) );
    return;
  }

  mUserAuth->blockSignals( true );
  mUserAuth->setPassword( password );
  mUserAuth->blockSignals( false );

  QNetworkRequest request = getDefaultRequest( false );
  QString urlString = mApiRoot + QStringLiteral( "v1/auth/login" );
  QUrl url( urlString );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/json" );

  QJsonDocument jsonDoc;
  QJsonObject jsonObject;
  jsonObject.insert( QStringLiteral( "login" ), login );
  jsonObject.insert( QStringLiteral( "password" ), mUserAuth->password() );
  jsonDoc.setObject( jsonObject );
  QByteArray json = jsonDoc.toJson( QJsonDocument::Compact );

  QNetworkReply *reply = mManager.post( request, json );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::authorizeFinished );
  CoreUtils::log( "auth", QStringLiteral( "Requesting authorization: " ) + url.toString() );
}

void MerginApi::registerUser( const QString &username,
                              const QString &email,
                              const QString &password,
                              const QString &confirmPassword,
                              bool acceptedTOC )
{
  // Some very basic checks, so we do not validate everything
  if ( username.isEmpty() || username.length() < 4 )
  {
    QString msg = tr( "Username must have at least 4 characters" );
    emit registrationFailed( msg, RegistrationError::RegistrationErrorType::USERNAME );
    return;
  }

  if ( !CoreUtils::isValidName( username ) )
  {
    QString msg = tr( "Username contains invalid characters" );
    emit registrationFailed( msg, RegistrationError::RegistrationErrorType::USERNAME );
    return;
  }

  if ( email.isEmpty() || !email.contains( '@' ) || !email.contains( '.' ) )
  {
    QString msg = tr( "Please enter a valid email" );
    emit registrationFailed( msg, RegistrationError::RegistrationErrorType::EMAIL );
    return;
  }

  if ( password.isEmpty() || password.length() < 8 )
  {
    QString msg = tr( "Password not strong enough. It must"
                      "%1 be at least 8 characters long"
                      "%1 contain lowercase characters"
                      "%1 contain uppercase characters"
                      "%1 contain digits or special characters" )
                  .arg( "<br />  -" );
    emit registrationFailed( msg, RegistrationError::RegistrationErrorType::PASSWORD );
    return;

  }

  if ( confirmPassword != password )
  {
    QString msg = tr( "Passwords do not match" );
    emit registrationFailed( msg, RegistrationError::RegistrationErrorType::CONFIRM_PASSWORD );
    return;
  }

  if ( !acceptedTOC )
  {
    QString msg = tr( "Please accept Terms and Privacy Policy" );
    emit registrationFailed( msg, RegistrationError::RegistrationErrorType::TOC );
    return;
  }

  // request
  QNetworkRequest request = getDefaultRequest( false );
  QString urlString = mApiRoot + QStringLiteral( "v1/auth/register" );
  QUrl url( urlString );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/json" );

  QJsonDocument jsonDoc;
  QJsonObject jsonObject;
  jsonObject.insert( QStringLiteral( "username" ), username );
  jsonObject.insert( QStringLiteral( "email" ), email );
  jsonObject.insert( QStringLiteral( "password" ), password );
  jsonObject.insert( QStringLiteral( "api_key" ), getApiKey( mApiRoot ) );
  jsonDoc.setObject( jsonObject );
  QByteArray json = jsonDoc.toJson( QJsonDocument::Compact );
  QNetworkReply *reply = mManager.post( request, json );
  connect( reply, &QNetworkReply::finished, this, [ = ]() { this->registrationFinished( username, password ); } );
  CoreUtils::log( "auth", QStringLiteral( "Requesting registration: " ) + url.toString() );
}

void MerginApi::postRegisterUser( const QString &marketingChannel, const QString &industry, bool wantsNewsletter )
{
  // Some very basic checks, so we do not validate everything
  if ( marketingChannel.isEmpty() || industry.isEmpty() )
  {
    QString msg = tr( "Marketing source cannot be empty" );
    emit postRegistrationFailed( msg );
    return;
  }

  // Some very basic checks, so we do not validate everything
  if ( industry.isEmpty() )
  {
    QString msg = tr( "Industry cannot be empty" );
    emit postRegistrationFailed( msg );
    return;
  }
  // request
  QNetworkRequest request = getDefaultRequest( false );
  QString urlString = mApiRoot + QStringLiteral( "v1/post-register" );
  QUrl url( urlString );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/json" );

  QJsonDocument jsonDoc;
  QJsonObject jsonObject;
  jsonObject.insert( QStringLiteral( "industry" ), industry );
  jsonObject.insert( QStringLiteral( "marketing_channel" ), marketingChannel );
  jsonObject.insert( QStringLiteral( "subscribe" ), wantsNewsletter );
  jsonDoc.setObject( jsonObject );
  QByteArray json = jsonDoc.toJson( QJsonDocument::Compact );
  QNetworkReply *reply = mManager.post( request, json );
  connect( reply, &QNetworkReply::finished, this, [ = ]() { this->postRegistrationFinished(); } );
  CoreUtils::log( "auth", QStringLiteral( "Requesting post-registration: " ) + url.toString() );
}

void MerginApi::getUserInfo()
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QString urlString;
  if ( mServerType == MerginServerType::OLD )
  {
    urlString = mApiRoot + QStringLiteral( "v1/user/%1" ).arg( mUserAuth->username() );
  }
  else
  {
    urlString = mApiRoot + QStringLiteral( "v1/user/profile" );
  }

  QNetworkRequest request = getDefaultRequest();
  QUrl url( urlString );
  request.setUrl( url );

  QNetworkReply *reply = mManager.get( request );
  CoreUtils::log( "user info", QStringLiteral( "Requesting user info: " ) + url.toString() );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::getUserInfoFinished );
}

void MerginApi::getWorkspaceInfo()
{
  if ( mServerType == MerginServerType::OLD )
  {
    return;
  }

  if ( mUserInfo->activeWorkspaceId() == -1 )
  {
    return;
  }

  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QString urlString = mApiRoot + QStringLiteral( "v1/workspace/%1" ).arg( mUserInfo->activeWorkspaceId() );
  QNetworkRequest request = getDefaultRequest();
  QUrl url( urlString );
  request.setUrl( url );

  QNetworkReply *reply = mManager.get( request );
  CoreUtils::log( "workspace info", QStringLiteral( "Requesting workspace info: " ) + url.toString() );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::getWorkspaceInfoReplyFinished );
}

void MerginApi::getServiceInfo()
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QString urlString;

  if ( mServerType == MerginServerType::SAAS )
  {
    urlString = mApiRoot + QStringLiteral( "v1/workspace/%1/service" ).arg( mUserInfo->activeWorkspaceId() );
  }
  else if ( mServerType == MerginServerType::OLD )
  {
    urlString = mApiRoot + QStringLiteral( "v1/user/service" );
  }
  else
  {
    return;
  }

  QNetworkRequest request = getDefaultRequest( true );
  QUrl url( urlString );
  request.setUrl( url );

  QNetworkReply *reply = mManager.get( request );

  connect( reply, &QNetworkReply::finished, this, &MerginApi::getServiceInfoReplyFinished );

  CoreUtils::log( "Service info", QStringLiteral( "Requesting service info: " ) + url.toString() );
}

void MerginApi::getServiceInfoReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "Service info", QStringLiteral( "Success" ) );

    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    if ( doc.isObject() )
    {
      QJsonObject docObj = doc.object();
      mSubscriptionInfo->setFromJson( docObj );
    }
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "getServiceInfo" ), r->errorString(), serverMsg );
    CoreUtils::log( "Service info", QStringLiteral( "FAILED - %1" ).arg( message ) );

    mSubscriptionInfo->clear();

    int httpCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    if ( httpCode == 404 )
    {
      // no such API on the server, do not emit anything
    }
    else if ( httpCode == 403 )
    {
      // forbidden - I do not have enough rights to see this, do not emit anything
    }
    else
    {
      emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: getServiceInfo" ) );
    }
  }

  r->deleteLater();
}

void MerginApi::clearAuth()
{
  mUserAuth->clear();
  mUserInfo->clear();
  mUserInfo->clearCachedWorkspacesInfo();
  mWorkspaceInfo->clear();
  mSubscriptionInfo->clear();
}

void MerginApi::resetApiRoot()
{
  QSettings settings;
  settings.beginGroup( QStringLiteral( "Input/" ) );
  setApiRoot( defaultApiRoot() );
  settings.endGroup();
}

QString MerginApi::resetPasswordUrl()
{
  if ( !mApiRoot.isEmpty() )
  {
    QUrl base( mApiRoot );
    return base.resolved( QUrl( "login/reset" ) ).toString();
  }
  return QString();
}

bool MerginApi::createProject( const QString &projectNamespace, const QString &projectName, bool isPublic )
{
  if ( !validateAuth() )
  {
    emit missingAuthorizationError( projectName );
    return false;
  }

  if ( mApiVersionStatus != MerginApiStatus::OK )
  {
    return false;
  }

  QString projectFullName = getFullProjectName( projectNamespace, projectName );

  QNetworkRequest request = getDefaultRequest();
  QUrl url( mApiRoot + QString( "/v1/project/%1" ).arg( projectNamespace ) );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/json" );
  request.setRawHeader( "Accept", "application/json" );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ), projectFullName );

  QJsonDocument jsonDoc;
  QJsonObject jsonObject;
  jsonObject.insert( QStringLiteral( "name" ), projectName );
  jsonObject.insert( QStringLiteral( "public" ), isPublic );
  jsonDoc.setObject( jsonObject );
  QByteArray json = jsonDoc.toJson( QJsonDocument::Compact );

  QNetworkReply *reply = mManager.post( request, json );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::createProjectFinished );
  CoreUtils::log( "create " + projectFullName, QStringLiteral( "Requesting project creation: " ) + url.toString() );

  return true;
}

void MerginApi::deleteProject( const QString &projectNamespace, const QString &projectName, bool informUser )
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QString projectFullName = getFullProjectName( projectNamespace, projectName );

  QNetworkRequest request = getDefaultRequest();
  QUrl url( mApiRoot + QStringLiteral( "/v1/project/%1" ).arg( projectFullName ) );
  request.setUrl( url );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ), projectFullName );
  QNetworkReply *reply = mManager.deleteResource( request );
  connect( reply, &QNetworkReply::finished, this, [this, informUser]() { this->deleteProjectFinished( informUser );} );
  CoreUtils::log( "delete " + projectFullName, QStringLiteral( "Requesting project deletion: " ) + url.toString() );
}

void MerginApi::saveAuthData()
{
  QSettings settings;
  settings.beginGroup( "Input/" );
  settings.setValue( "apiRoot", mApiRoot );
  settings.endGroup();

  mUserAuth->saveAuthData();
  mUserInfo->clear();
}

void MerginApi::createProjectFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();

  QString projectNamespace, projectName;
  extractProjectName( projectFullName, projectNamespace, projectName );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "create " + projectFullName, QStringLiteral( "Success" ) );
    emit projectCreated( projectFullName, true );


    // Upload data if createProject has been called for a local project with empty namespace (case of migrating a project)
    for ( const LocalProject &info : mLocalProjects.projects() )
    {
      if ( info.projectName == projectName && info.projectNamespace.isEmpty() )
      {
        mLocalProjects.updateNamespace( info.projectDir, projectNamespace );
        emit projectAttachedToMergin( projectFullName, projectName );

        QDir projectDir( info.projectDir );
        if ( projectDir.exists() && !projectDir.isEmpty() )
        {
          pushProject( projectNamespace, projectName, true );
        }
      }
    }
  }
  else
  {
    QByteArray data = r->readAll();
    QString code = extractServerErrorCode( data );
    QString serverMsg = extractServerErrorMsg( data );
    QString message = QStringLiteral( "FAILED - %1: %2" ).arg( r->errorString(), serverMsg );
    bool showLimitReachedDialog = EnumHelper::isEqual( code, ErrorCode::ProjectsLimitHit );

    CoreUtils::log( "create " + projectFullName, message );

    emit projectCreated( projectFullName, false );

    if ( showLimitReachedDialog )
    {
      int maxProjects = 0;
      QVariant maxProjectVariant = extractServerErrorValue( data, "projects_quota" );
      if ( maxProjectVariant.isValid() )
        maxProjects = maxProjectVariant.toInt();
      emit projectLimitReached( maxProjects, serverMsg );
    }
    else
    {
      int httpCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
      emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: createProject" ), httpCode, projectName );
    }
  }
  r->deleteLater();
}

void MerginApi::deleteProjectFinished( bool informUser )
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "delete " + projectFullName, QStringLiteral( "Success" ) );

    if ( informUser )
      emit notifySuccess( QStringLiteral( "Project deleted" ) );

    emit serverProjectDeleted( projectFullName, true );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    CoreUtils::log( "delete " + projectFullName, QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );
    emit serverProjectDeleted( projectFullName, false );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: deleteProject" ) );
  }
  r->deleteLater();
}

void MerginApi::authorizeFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "auth", QStringLiteral( "Success" ) );
    const QByteArray data = r->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( data );
    if ( doc.isObject() )
    {
      QJsonObject docObj = doc.object();
      mUserAuth->setFromJson( docObj );
    }
    else
    {
      // keep username and password, but clear token
      // this is problem with internet connection or server
      // so do not force user to input login credentials again
      mUserAuth->clearTokenData();
      emit authFailed();
      CoreUtils::log( "Auth", QStringLiteral( "FAILED - invalid JSON response" ) );
      emit notifyError( "Internal server error during authorization" );
    }
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QVariant statusCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    int status = statusCode.toInt();
    CoreUtils::log( "Auth", QStringLiteral( "FAILED - %1. %2 (%3)" ).arg( r->errorString(), serverMsg, QString::number( status ) ) );

    if ( status == 401 )
    {
      // OK, we have INVALID username or password or
      // our user got blocked on the server by admin or owner
      // lets show error to user and let him try different credentials
      emit authFailed();
      emit notifyError( serverMsg );

      mUserAuth->blockSignals( true );
      mUserAuth->setUsername( QString() );
      mUserAuth->setPassword( QString() );
      mUserAuth->blockSignals( false );

    }
    else
    {
      // keep username and password
      // this is problem with internet connection or server
      // so do not force user to input login credentials again
      emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: authorize" ) );
    }

    // in case of any error, just clean token and request new one
    mUserAuth->clearTokenData();
  }

  if ( mAuthLoopEvent.isRunning() )
  {
    mAuthLoopEvent.exit();
  }
  r->deleteLater();
}

void MerginApi::registrationFinished( const QString &username, const QString &password )
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "register", QStringLiteral( "Success" ) );
    QString msg = tr( "Registration successful" );
    emit notifySuccess( msg );

    if ( !username.isEmpty() && !password.isEmpty() ) // log in immediately
      authorize( username, password );

    emit registrationSucceeded();
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    CoreUtils::log( "register", QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );
    QVariant statusCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    int status = statusCode.toInt();
    if ( status == 401 || status == 400 )
    {
      emit registrationFailed( serverMsg, RegistrationError::RegistrationErrorType::OTHER );
      emit notifyError( serverMsg );
    }
    else if ( status == 404 )
    {
      // the self-registration is not allowed on the server
      QString msg = tr( "New registrations are not allowed on the selected server. Please check with your administrator." );
      emit registrationFailed( msg, RegistrationError::RegistrationErrorType::OTHER );
      emit notifyError( msg );
    }
    else
    {
      QString msg = QStringLiteral( "Mergin API error: register" );
      emit registrationFailed( msg, RegistrationError::RegistrationErrorType::OTHER );
      emit networkErrorOccurred( serverMsg, msg );
    }
  }
  r->deleteLater();
}

void MerginApi::postRegistrationFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "post-register", QStringLiteral( "Success" ) );
    emit postRegistrationSucceeded();
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    CoreUtils::log( "post-register", QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );
    QVariant statusCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    int status = statusCode.toInt();
    emit postRegistrationFailed( QStringLiteral( "Post-registation failed %1" ).arg( serverMsg ) );
  }
  emit notifySuccess( tr( "Workspace created" ) );
  r->deleteLater();
}

void MerginApi::pingMerginReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );
  QString apiVersion;
  QString serverMsg;
  bool serverSupportsSubscriptions = false;

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "ping", QStringLiteral( "Success" ) );
    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    if ( doc.isObject() )
    {
      QJsonObject obj = doc.object();
      apiVersion = obj.value( QStringLiteral( "version" ) ).toString();
      serverSupportsSubscriptions = obj.value( QStringLiteral( "subscriptions_enabled" ) ).toBool();
    }
  }
  else
  {
    serverMsg = extractServerErrorMsg( r->readAll() );
    CoreUtils::log( "ping", QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );
  }
  r->deleteLater();
  emit pingMerginFinished( apiVersion, serverSupportsSubscriptions, serverMsg );
}

void MerginApi::onPlanProductIdChanged()
{
  if ( mUserAuth->hasAuthData() )
  {
    if ( mServerType == MerginServerType::OLD )
    {
      getUserInfo();
    }
    else
    {
      getWorkspaceInfo();
    }
  }
}

QNetworkReply *MerginApi::getProjectInfo( const QString &projectFullName, bool withAuth )
{
  if ( withAuth && !validateAuth() )
  {
    emit missingAuthorizationError( projectFullName );
    return nullptr;
  }

  if ( mApiVersionStatus != MerginApiStatus::OK )
  {
    return nullptr;
  }

  int sinceVersion = -1;
  LocalProject projectInfo = mLocalProjects.projectFromMerginName( projectFullName );
  if ( projectInfo.isValid() )
  {
    // let's also fetch the recent history of diffable files
    // (the "since" is inclusive, so if we are on v2, we want to use since=v3 which will include v2->v3, v3->v4, ...)
    sinceVersion = projectInfo.localVersion + 1;
  }

  QUrlQuery query;
  if ( sinceVersion != -1 )
    query.addQueryItem( QStringLiteral( "since" ), QStringLiteral( "v%1" ).arg( sinceVersion ) );

  QUrl url( mApiRoot + QStringLiteral( "/v1/project/%1" ).arg( projectFullName ) );
  url.setQuery( query );

  QNetworkRequest request = getDefaultRequest( withAuth );
  request.setUrl( url );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ), projectFullName );

  return mManager.get( request );
}

bool MerginApi::validateAuth()
{
  if ( !mUserAuth->hasAuthData() )
  {
    emit authRequested();
    return false;
  }

  if ( mUserAuth->authToken().isEmpty() || mUserAuth->tokenExpiration() < QDateTime().currentDateTime().toUTC() )
  {
    authorize( mUserAuth->username(), mUserAuth->password() );
    CoreUtils::log( QStringLiteral( "MerginApi" ), QStringLiteral( "Requesting authorization because of missing or expired token." ) );
    mAuthLoopEvent.exec();
  }
  return true;
}

void MerginApi::checkMerginVersion( QString apiVersion, bool serverSupportsSubscriptions, QString msg )
{
  setApiSupportsSubscriptions( serverSupportsSubscriptions );

  if ( msg.isEmpty() )
  {
    int major = -1;
    int minor = -1;

    bool validVersion = parseVersion( apiVersion, major, minor );

    if ( !validVersion )
    {
      setApiVersionStatus( MerginApiStatus::NOT_FOUND );
      return;
    }

    if ( ( MERGIN_API_VERSION_MAJOR == major && MERGIN_API_VERSION_MINOR <= minor ) || ( MERGIN_API_VERSION_MAJOR < major ) )
    {
      setApiVersionStatus( MerginApiStatus::OK );
    }
    else
    {
      setApiVersionStatus( MerginApiStatus::INCOMPATIBLE );
    }
  }
  else
  {
    setApiVersionStatus( MerginApiStatus::NOT_FOUND );
  }
}

bool MerginApi::extractProjectName( const QString &sourceString, QString &projectNamespace, QString &name )
{
  QStringList parts = sourceString.split( "/" );
  if ( parts.length() > 1 )
  {
    projectNamespace = parts.at( parts.length() - 2 );
    name = parts.last();
    return true;
  }
  else
  {
    name = sourceString;
    return false;
  }
}

QString MerginApi::extractServerErrorCode( const QByteArray &data )
{
  QVariant code = extractServerErrorValue( data, QStringLiteral( "code" ) );
  if ( code.isValid() )
    return code.toString();
  return QString();
}

QVariant MerginApi::extractServerErrorValue( const QByteArray &data, const QString &key )
{
  QJsonDocument doc = QJsonDocument::fromJson( data );
  if ( doc.isObject() )
  {
    QJsonObject obj = doc.object();
    if ( obj.contains( key ) )
    {
      QJsonValue val = obj.value( key );
      return val.toVariant();
    }
  }

  return QVariant();
}

QString MerginApi::extractServerErrorMsg( const QByteArray &data )
{
  QString serverMsg = "[can't parse server error]";
  QJsonDocument doc = QJsonDocument::fromJson( data );
  if ( doc.isObject() )
  {
    QJsonObject obj = doc.object();
    if ( obj.contains( QStringLiteral( "detail" ) ) )
    {
      QJsonValue vDetail = obj.value( "detail" );
      if ( vDetail.isString() )
      {
        serverMsg = vDetail.toString();
      }
      else if ( vDetail.isObject() )
      {
        serverMsg = QJsonDocument( vDetail.toObject() ).toJson();
      }
    }
    else if ( obj.contains( QStringLiteral( "name" ) ) )
    {
      QJsonValue val = obj.value( "name" );
      if ( val.isArray() )
      {
        QJsonArray errors = val.toArray();
        QStringList messages;
        for ( auto it = errors.constBegin(); it != errors.constEnd(); ++it )
        {
          messages << it->toString();
        }
        serverMsg = messages.join( " " );
      }
    }
    else
    {
      serverMsg = "[can't parse server error]";
    }
  }
  else
  {
    // take only first 1000 bytes of the message ~ there are situations when data is an unclosed string that would eat the whole log memory
    serverMsg = data.mid( 0, 1000 );
  }

  return serverMsg;
}


LocalProject MerginApi::getLocalProject( const QString &projectFullName )
{
  return mLocalProjects.projectFromMerginName( projectFullName );
}

ProjectDiff MerginApi::localProjectChanges( const QString &projectDir )
{
  MerginProjectMetadata projectMetadata = MerginProjectMetadata::fromCachedJson( projectDir + "/" + sMetadataFile );
  QList<MerginFile> localFiles = getLocalProjectFiles( projectDir + "/" );

  MerginConfig config = MerginConfig::fromFile( projectDir + "/" + sMerginConfigFile );

  return compareProjectFiles( projectMetadata.files, projectMetadata.files, localFiles, projectDir, config.isValid, config );
}

bool MerginApi::parseVersion( const QString &version, int &major, int &minor )
{
  if ( version.isNull() || version.isEmpty() )
    return false;

  QStringList versionParts = version.split( '.' );

  if ( versionParts.size() != 3 )
    return false;

  bool majorOk, minorOk;
  major = versionParts[0].toInt( &majorOk );
  minor = versionParts[1].toInt( &minorOk );

  if ( !majorOk || !minorOk )
    return false;

  return true;
}

bool MerginApi::hasLocalProjectChanges( const QString &projectDir )
{
  MerginProjectMetadata projectMetadata = MerginProjectMetadata::fromCachedJson( projectDir + "/" + sMetadataFile );
  QList<MerginFile> localFiles = getLocalProjectFiles( projectDir + "/" );

  MerginConfig config = MerginConfig::fromFile( projectDir + "/" + sMerginConfigFile );

  return hasLocalChanges( projectMetadata.files, localFiles, projectDir );
}

QString MerginApi::getTempProjectDir( const QString &projectFullName )
{
  return mDataDir + "/" + TEMP_FOLDER + projectFullName;
}

QString MerginApi::getFullProjectName( QString projectNamespace, QString projectName ) // TODO: move to inpututils?
{
  return QString( "%1/%2" ).arg( projectNamespace ).arg( projectName );
}

MerginApiStatus::VersionStatus MerginApi::apiVersionStatus() const
{
  return mApiVersionStatus;
}

void MerginApi::setApiVersionStatus( const MerginApiStatus::VersionStatus &apiVersionStatus )
{
  if ( mApiVersionStatus != apiVersionStatus )
  {
    mApiVersionStatus = apiVersionStatus;
    emit apiVersionStatusChanged();
  }
}

void MerginApi::pingMergin()
{
  if ( mApiVersionStatus == MerginApiStatus::OK ) return;

  setApiVersionStatus( MerginApiStatus::PENDING );

  QNetworkRequest request = getDefaultRequest( false );
  QUrl url( mApiRoot + QStringLiteral( "/ping" ) );
  request.setUrl( url );

  QNetworkReply *reply = mManager.get( request );
  CoreUtils::log( "ping", QStringLiteral( "Requesting: " ) + url.toString() );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::pingMerginReplyFinished );
}

void MerginApi::migrateProjectToMergin( const QString &projectName, const QString &projectNamespace )
{
  CoreUtils::log( "migrate project", projectName );
  if ( projectNamespace.isEmpty() )
  {
    createProject( mUserAuth->username(), projectName );
  }
  else
  {
    createProject( projectNamespace, projectName );
  }
}

void MerginApi::detachProjectFromMergin( const QString &projectNamespace, const QString &projectName, bool informUser )
{
  // Remove mergin folder
  QString projectFullName = getFullProjectName( projectNamespace, projectName );
  LocalProject projectInfo = mLocalProjects.projectFromMerginName( projectFullName );

  if ( projectInfo.isValid() )
  {
    CoreUtils::removeDir( projectInfo.projectDir + "/.mergin" );
  }

  // Update localProject
  mLocalProjects.updateNamespace( projectInfo.projectDir, "" );
  mLocalProjects.updateLocalVersion( projectInfo.projectDir, -1 );

  if ( informUser )
    emit notifySuccess( tr( "Project detached from the server" ) );

  emit projectDetached( projectFullName );
}

QString MerginApi::apiRoot() const
{
  return mApiRoot;
}

void MerginApi::setApiRoot( const QString &apiRoot )
{
  QString newApiRoot;
  if ( apiRoot.isEmpty() )
  {
    newApiRoot = defaultApiRoot();
  }
  else
  {
    newApiRoot = apiRoot;
  }

  if ( newApiRoot != mApiRoot )
  {
    mApiRoot = newApiRoot;

    QSettings settings;
    settings.setValue( QStringLiteral( "Input/apiRoot" ), mApiRoot );

    emit apiRootChanged();
  }
}

QList<MerginFile> MerginApi::getLocalProjectFiles( const QString &projectPath )
{
  QElapsedTimer timer;
  timer.start();

  QList<MerginFile> merginFiles;
  ProjectChecksumCache checksumCache( projectPath );

  QSet<QString> localFiles = listFiles( projectPath );
  for ( QString p : localFiles )
  {
    MerginFile file;
    file.checksum = checksumCache.get( p );
    file.path = p;
    QFileInfo info( projectPath + p );
    file.size = info.size();
    file.mtime = info.lastModified();
    merginFiles.append( file );
  }

  qint64 elapsed = timer.elapsed();
  if ( elapsed > 100 )
  {
    CoreUtils::log( "Local File", QStringLiteral( "It took %1 ms to create MerginFiles for %2 local files for %3." ).arg( elapsed ).arg( localFiles.count() ).arg( projectPath ) );
  }
  return merginFiles;
}

void MerginApi::listProjectsReplyFinished( QString requestId )
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  int projectCount = -1;
  int requestedPage = 1;
  MerginProjectsList projectList;

  if ( r->error() == QNetworkReply::NoError )
  {
    QUrlQuery query( r->request().url().query() );
    requestedPage = query.queryItemValue( "page" ).toInt();

    QByteArray data = r->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( data );

    if ( doc.isObject() )
    {
      projectCount = doc.object().value( "count" ).toInt();
      projectList = parseProjectsFromJson( doc );
    }

    CoreUtils::log( "list projects", QStringLiteral( "Success - got %1 projects" ).arg( projectList.count() ) );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "listProjects" ), r->errorString(), serverMsg );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: listProjects" ) );
    CoreUtils::log( "list projects", QStringLiteral( "FAILED - %1" ).arg( message ) );

    emit listProjectsFailed();
  }

  r->deleteLater();

  emit listProjectsFinished( projectList, projectCount, requestedPage, requestId );
}

void MerginApi::listProjectsByNameReplyFinished( QString requestId )
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  MerginProjectsList projectList;

  if ( r->error() == QNetworkReply::NoError )
  {
    QByteArray data = r->readAll();
    QJsonDocument json = QJsonDocument::fromJson( data );
    projectList = parseProjectsFromJson( json );
    CoreUtils::log( "list projects by name", QStringLiteral( "Success - got %1 projects" ).arg( projectList.count() ) );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "listProjectsByName" ), r->errorString(), serverMsg );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: listProjectsByName" ) );
    CoreUtils::log( "list projects by name", QStringLiteral( "FAILED - %1" ).arg( message ) );

    emit listProjectsFailed();
  }

  r->deleteLater();

  emit listProjectsByNameFinished( projectList, requestId );
}


void MerginApi::finalizeProjectPullCopy( const QString &projectFullName, const QString &projectDir, const QString &tempDir, const QString &filePath, const QList<DownloadQueueItem> &items )
{
  CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Copying new content of " ) + filePath );

  QString dest = projectDir + "/" + filePath;
  createPathIfNotExists( dest );

  QFile f( dest );
  if ( !f.open( QIODevice::WriteOnly ) )
  {
    CoreUtils::log( "pull " + projectFullName, "Failed to open file for writing " + dest );
    return;
  }

  // assemble file from tmp files
  for ( const auto &item : items )
  {
    QFile fTmp( tempDir + "/" + item.tempFileName );
    if ( !fTmp.open( QIODevice::ReadOnly ) )
    {
      CoreUtils::log( "pull " + projectFullName, "Failed to open temp file for reading " + item.tempFileName );
      return;
    }
    f.write( fTmp.readAll() );
  }

  f.close();

  // if diffable, copy to .mergin dir so we have a basefile
  if ( MerginApi::isFileDiffable( filePath ) )
  {
    QString basefile = projectDir + "/.mergin/" + filePath;
    createPathIfNotExists( basefile );

    if ( !QFile::remove( basefile ) )
    {
      CoreUtils::log( "pull " + projectFullName, "failed to remove old basefile for: " + filePath );
    }
    if ( !QFile::copy( dest, basefile ) )
    {
      CoreUtils::log( "pull " + projectFullName, "failed to copy new basefile for: " + filePath );
    }
  }
}


bool MerginApi::finalizeProjectPullApplyDiff( const QString &projectFullName, const QString &projectDir, const QString &tempDir, const QString &filePath, const QList<DownloadQueueItem> &items )
{
  CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Applying diff to " ) + filePath );

  // update diffable files that have been modified on the server
  // - if they were not modified locally, the server changes will be simply applied
  // - if they were modified locally, local changes will be rebased on top of server changes

  QString src = tempDir + "/" + CoreUtils::uuidWithoutBraces( QUuid::createUuid() );
  QString dest = projectDir + "/" + filePath;
  QString basefile = projectDir + "/.mergin/" + filePath;

  LocalProject info = mLocalProjects.projectFromMerginName( projectFullName );

  // add conflict files to project dir so they can be synced
  QString conflictfile = CoreUtils::findUniquePath( CoreUtils::generateEditConflictFileName( dest, mUserAuth->username(), info.localVersion ) );

  createPathIfNotExists( src );
  createPathIfNotExists( dest );
  createPathIfNotExists( basefile );

  QStringList diffFiles;
  for ( const auto &item : items )
    diffFiles << tempDir + "/" + item.tempFileName;

  //
  // let's first assemble server's file from our basefile + diffs
  //

  if ( !QFile::copy( basefile, src ) )
  {
    CoreUtils::log( "pull " + projectFullName, "assemble server file fail: copying failed " + basefile + " to " + src );

    // TODO: this is a critical failure - we should abort pull
  }

  if ( !GeodiffUtils::applyDiffs( src, diffFiles ) )
  {
    CoreUtils::log( "pull " + projectFullName, "server file assembly failed: " + filePath );

    // TODO: this is a critical failure - we should abort pull
    // TODO: we could try to delete the basefile and re-download it from scratch on next sync
  }
  else
  {
    CoreUtils::log( "pull " + projectFullName, "server file assembly successful: " + filePath );
  }

  //
  // now we are ready for the update of our local file
  //
  bool hasConflicts = false;

  bool res = GeodiffUtils::rebase( basefile,
                                   src,
                                   dest,
                                   conflictfile
                                 );
  if ( res )
  {
    CoreUtils::log( "pull " + projectFullName, "geodiff rebase successful: " + filePath );
  }
  else
  {
    CoreUtils::log( "pull " + projectFullName, "geodiff rebase failed! " + filePath );

    // not good... something went wrong in rebase - we need to save the local changes
    // let's put them into a conflict file and use the server version
    hasConflicts = true;
    LocalProject info = mLocalProjects.projectFromMerginName( projectFullName );
    QString newDest = CoreUtils::findUniquePath( CoreUtils::generateConflictedCopyFileName( dest, mUserAuth->username(), info.localVersion ) );
    if ( !QFile::rename( dest, newDest ) )
    {
      CoreUtils::log( "pull " + projectFullName, "failed rename of conflicting file after failed geodiff rebase: " + filePath );
    }
    if ( !QFile::copy( src, dest ) )
    {
      CoreUtils::log( "pull " + projectFullName, "failed to update local conflicting file after failed geodiff rebase: " + filePath );
    }
  }

  //
  // finally update our basefile
  //

  if ( !QFile::remove( basefile ) )
  {
    CoreUtils::log( "pull " + projectFullName, "failed removal of old basefile: " + filePath );

    // TODO: this is a critical failure - we should abort pull
  }
  if ( !QFile::rename( src, basefile ) )
  {
    CoreUtils::log( "pull " + projectFullName, "failed rename of basefile using new server content: " + filePath );

    // TODO: this is a critical failure - we should abort pull
  }
  return hasConflicts;
}

void MerginApi::finalizeProjectPull( const QString &projectFullName )
{
  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  QString projectDir = transaction.projectDir;
  QString tempProjectDir = getTempProjectDir( projectFullName );

  CoreUtils::log( "pull " + projectFullName, "Running update tasks" );

  for ( const PullTask &finalizationItem : transaction.pullTasks )
  {
    switch ( finalizationItem.method )
    {
      case PullTask::Copy:
      {
        finalizeProjectPullCopy( projectFullName, projectDir, tempProjectDir, finalizationItem.filePath, finalizationItem.data );
        break;
      }

      case PullTask::CopyConflict:
      {
        // move local file to conflict file
        QString origPath = projectDir + "/" + finalizationItem.filePath;
        LocalProject info = mLocalProjects.projectFromMerginName( projectFullName );
        QString newPath = CoreUtils::findUniquePath( CoreUtils::generateConflictedCopyFileName( origPath, mUserAuth->username(), info.localVersion ) );
        if ( !QFile::rename( origPath, newPath ) )
        {
          CoreUtils::log( "pull " + projectFullName, "failed rename of conflicting file: " + finalizationItem.filePath );
        }
        else
        {
          CoreUtils::log( "pull " + projectFullName, "Local file renamed due to conflict with server: " + finalizationItem.filePath );
        }
        finalizeProjectPullCopy( projectFullName, projectDir, tempProjectDir, finalizationItem.filePath, finalizationItem.data );
        break;
      }

      case PullTask::ApplyDiff:
      {
        // applying diff can result in conflicted copy too, in this case
        // we need to update gpkgSchemaChanged flag.
        bool res = finalizeProjectPullApplyDiff( projectFullName, projectDir, tempProjectDir, finalizationItem.filePath, finalizationItem.data );
        transaction.gpkgSchemaChanged = res;
        break;
      }

      case PullTask::Delete:
      {
        CoreUtils::log( "pull " + projectFullName, "Removing local file: " + finalizationItem.filePath );
        QFile file( projectDir + "/" + finalizationItem.filePath );
        file.remove();
        break;
      }
    }

    // remove tmp files associated with this item
    for ( const auto &downloadItem : finalizationItem.data )
    {
      if ( !QFile::remove( tempProjectDir + "/" + downloadItem.tempFileName ) )
        CoreUtils::log( "pull " + projectFullName, "Failed to remove temporary file " + downloadItem.tempFileName );
    }
  }

  // check there are no files left
  int tmpFilesLeft = QDir( tempProjectDir ).entryList( QDir::NoDotAndDotDot ).count();
  if ( tmpFilesLeft )
  {
    CoreUtils::log( "pull " + projectFullName, "Some temporary files were left - this should not happen..." );
  }

  QDir( tempProjectDir ).removeRecursively();

  // add the local project if not there yet
  if ( !mLocalProjects.projectFromMerginName( projectFullName ).isValid() )
  {
    QString projectNamespace, projectName;
    extractProjectName( projectFullName, projectNamespace, projectName );

    // remove download in progress file
    if ( !QFile::remove( CoreUtils::downloadInProgressFilePath( transaction.projectDir ) ) )
      CoreUtils::log( QStringLiteral( "sync %1" ).arg( projectFullName ), QStringLiteral( "Failed to remove download in progress file for project name %1" ).arg( projectName ) );

    mLocalProjects.addMerginProject( projectDir, projectNamespace, projectName );
  }

  finishProjectSync( projectFullName, true );
}


void MerginApi::pushStartReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();

  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];
  Q_ASSERT( r == transaction.replyPushStart );

  if ( r->error() == QNetworkReply::NoError )
  {
    QByteArray data = r->readAll();

    transaction.replyPushStart->deleteLater();
    transaction.replyPushStart = nullptr;

    QList<MerginFile> files = transaction.pushQueue;
    if ( !files.isEmpty() )
    {
      QString transactionUUID;
      QJsonDocument doc = QJsonDocument::fromJson( data );
      if ( doc.isObject() )
      {
        QJsonObject docObj = doc.object();
        transactionUUID = docObj.value( QStringLiteral( "transaction" ) ).toString();
        transaction.transactionUUID = transactionUUID;
      }

      if ( transaction.transactionUUID.isEmpty() )
      {
        CoreUtils::log( "push " + projectFullName, QStringLiteral( "Fail! Could not acquire transaction ID" ) );
        finishProjectSync( projectFullName, false );
      }

      CoreUtils::log( "push " + projectFullName, QStringLiteral( "Push request accepted. Transaction ID: " ) + transactionUUID );

      MerginFile file = files.first();
      pushFile( projectFullName, transactionUUID, file );
      emit pushFilesStarted();
    }
    else  // pushing only files to be removed
    {
      // we are done here - no upload of chunks, no request to "finish"
      // because server immediatelly creates a new version without starting a transaction to upload chunks

      CoreUtils::log( "push " + projectFullName, QStringLiteral( "Push request accepted and no files to upload" ) );

      transaction.projectMetadata = data;
      transaction.version = MerginProjectMetadata::fromJson( data ).version;

      finishProjectSync( projectFullName, true );
    }
  }
  else
  {
    QByteArray data = r->readAll();
    QString serverMsg = extractServerErrorMsg( data );
    if ( r->error() == QNetworkReply::OperationCanceledError )
      serverMsg = sSyncCanceledMessage;

    QString code = extractServerErrorCode( data );
    bool showLimitReachedDialog = EnumHelper::isEqual( code, ErrorCode::StorageLimitHit );

    CoreUtils::log( "push " + projectFullName, QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );

    transaction.replyPushStart->deleteLater();
    transaction.replyPushStart = nullptr;

    if ( showLimitReachedDialog )
    {
      const QList<MerginFile> files = transaction.pushQueue;
      qreal uploadSize = 0;
      for ( const MerginFile &f : files )
      {
        uploadSize += f.size;
      }
      emit storageLimitReached( uploadSize );

      // remove project if it was first time sync - migration
      if ( transaction.isInitialPush )
      {
        QString projectNamespace, projectName;
        extractProjectName( projectFullName, projectNamespace, projectName );

        detachProjectFromMergin( projectNamespace, projectName, false );
        deleteProject( projectNamespace, projectName, false );
      }
    }
    else
    {
      int httpCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
      emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: pushStartReply" ), httpCode, projectFullName );
    }
    finishProjectSync( projectFullName, false );
  }
}

void MerginApi::pushFileReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();

  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];
  Q_ASSERT( r == transaction.replyPushFile );

  QStringList params = ( r->url().toString().split( "/" ) );
  QString transactionUUID = params.at( params.length() - 2 );
  QString chunkID = params.at( params.length() - 1 );
  Q_ASSERT( transactionUUID == transaction.transactionUUID );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "Uploaded successfully: " ) + chunkID );

    transaction.replyPushFile->deleteLater();
    transaction.replyPushFile = nullptr;

    MerginFile currentFile = transaction.pushQueue.first();
    int chunkNo = currentFile.chunks.indexOf( chunkID );
    if ( chunkNo < currentFile.chunks.size() - 1 )
    {
      pushFile( projectFullName, transactionUUID, currentFile, chunkNo + 1 );
    }
    else
    {
      transaction.transferedSize += currentFile.size;

      emit syncProjectStatusChanged( projectFullName, transaction.transferedSize / transaction.totalSize );
      transaction.pushQueue.removeFirst();

      if ( !transaction.pushQueue.isEmpty() )
      {
        MerginFile nextFile = transaction.pushQueue.first();
        pushFile( projectFullName, transactionUUID, nextFile );
      }
      else
      {
        pushFinish( projectFullName, transactionUUID );
      }
    }
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    if ( r->error() == QNetworkReply::OperationCanceledError )
      serverMsg = sSyncCanceledMessage;

    CoreUtils::log( "push " + projectFullName, QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );

    int httpCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: pushFile" ), httpCode, projectFullName );

    transaction.replyPushFile->deleteLater();
    transaction.replyPushFile = nullptr;

    finishProjectSync( projectFullName, false );
  }
}

void MerginApi::pullInfoReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();

  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];
  Q_ASSERT( r == transaction.replyPullProjectInfo );

  if ( r->error() == QNetworkReply::NoError )
  {
    QByteArray data = r->readAll();
    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Downloaded project info." ) );

    transaction.replyPullProjectInfo->deleteLater();
    transaction.replyPullProjectInfo = nullptr;

    prepareProjectPull( projectFullName, data );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    if ( r->error() == QNetworkReply::OperationCanceledError )
      serverMsg = sSyncCanceledMessage;

    QString message = QStringLiteral( "Network API error: %1(): %2" ).arg( QStringLiteral( "projectInfo" ), r->errorString() );
    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "FAILED - %1" ).arg( message ) );

    int httpCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: pullInfo" ), httpCode, projectFullName );

    transaction.replyPullProjectInfo->deleteLater();
    transaction.replyPullProjectInfo = nullptr;

    finishProjectSync( projectFullName, false );
  }
}

void MerginApi::prepareProjectPull( const QString &projectFullName, const QByteArray &data )
{
  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  MerginProjectMetadata serverProject = MerginProjectMetadata::fromJson( data );

  transaction.projectMetadata = data;
  transaction.version = serverProject.version;

  LocalProject projectInfo = mLocalProjects.projectFromMerginName( projectFullName );
  if ( projectInfo.isValid() )
  {
    transaction.projectDir = projectInfo.projectDir;

    // do not continue if we are already on the latest version
    if ( projectInfo.localVersion != -1 && projectInfo.localVersion == serverProject.version )
    {
      emit projectAlreadyOnLatestVersion( projectFullName );
      CoreUtils::log( QStringLiteral( "Pull %1" ).arg( projectFullName ), QStringLiteral( "Project is already on the latest version: %1" ).arg( serverProject.version ) );

      return finishProjectSync( projectFullName, false );
    }
  }
  else
  {
    QString projectNamespace;
    QString projectName;
    extractProjectName( projectFullName, projectNamespace, projectName );

    // remove any leftover temp files that could be created from previous unsuccessful download
    removeProjectsTempFolder( projectNamespace, projectName );

    // project has not been downloaded yet - we need to create a directory for it
    transaction.projectDir = CoreUtils::createUniqueProjectDirectory( mDataDir, projectName );
    transaction.firstTimeDownload = true;

    // create file indicating first time download in progress
    QString downloadInProgressFilePath = CoreUtils::downloadInProgressFilePath( transaction.projectDir );
    createPathIfNotExists( downloadInProgressFilePath );
    if ( !CoreUtils::createEmptyFile( downloadInProgressFilePath ) )
      CoreUtils::log( QStringLiteral( "pull %1" ).arg( projectFullName ), "Unable to create temporary download in progress file" );

    CoreUtils::log( "pull " + projectFullName, QStringLiteral( "First time download - new directory: " ) + transaction.projectDir );
  }

  Q_ASSERT( !transaction.projectDir.isEmpty() );  // that would mean we do not have entry -> fail getting local files

  if ( transaction.configAllowed )
  {
    prepareDownloadConfig( projectFullName );
  }
  else
  {
    startProjectPull( projectFullName );
  }
}

void MerginApi::startProjectPull( const QString &projectFullName )
{
  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  QList<MerginFile> localFiles = getLocalProjectFiles( transaction.projectDir + "/" );
  MerginProjectMetadata serverProject = MerginProjectMetadata::fromJson( transaction.projectMetadata );
  MerginProjectMetadata oldServerProject = MerginProjectMetadata::fromCachedJson( transaction.projectDir + "/" + sMetadataFile );
  MerginConfig oldTransactionConfig = MerginConfig::fromFile( transaction.projectDir + "/" + sMerginConfigFile );

  CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Updating from version %1 to version %2" )
                  .arg( oldServerProject.version ).arg( serverProject.version ) );

  transaction.diff = compareProjectFiles(
                       oldServerProject.files,
                       serverProject.files,
                       localFiles,
                       transaction.projectDir,
                       transaction.configAllowed,
                       transaction.config,
                       oldTransactionConfig );

  CoreUtils::log( "pull " + projectFullName, transaction.diff.dump() );

  for ( QString filePath : transaction.diff.remoteAdded )
  {
    MerginFile file = serverProject.fileInfo( filePath );
    QList<DownloadQueueItem> items = itemsForFileChunks( file, transaction.version );
    transaction.pullTasks << PullTask( PullTask::Copy, filePath, items );
    transaction.gpkgSchemaChanged = true;
  }

  for ( QString filePath : transaction.diff.remoteUpdated )
  {
    MerginFile file = serverProject.fileInfo( filePath );

    // for diffable files - download and apply to the basefile (without rebase)
    if ( isFileDiffable( filePath ) && file.pullCanUseDiff )
    {
      QList<DownloadQueueItem> items = itemsForFileDiffs( file );
      transaction.pullTasks << PullTask( PullTask::ApplyDiff, filePath, items );
    }
    else
    {
      QList<DownloadQueueItem> items = itemsForFileChunks( file, transaction.version );
      transaction.pullTasks << PullTask( PullTask::Copy, filePath, items );
      transaction.gpkgSchemaChanged = true;
    }
  }

  // also download files which were changed both on the server and locally (the local version will be renamed as conflicting copy)
  for ( QString filePath : transaction.diff.conflictRemoteUpdatedLocalUpdated )
  {
    MerginFile file = serverProject.fileInfo( filePath );

    // for diffable files - download and apply to the basefile (will also do rebase)
    if ( isFileDiffable( filePath ) && file.pullCanUseDiff )
    {
      QList<DownloadQueueItem> items = itemsForFileDiffs( file );
      transaction.pullTasks << PullTask( PullTask::ApplyDiff, filePath, items );
    }
    else
    {
      QList<DownloadQueueItem> items = itemsForFileChunks( file, transaction.version );
      transaction.pullTasks << PullTask( PullTask::CopyConflict, filePath, items );
      transaction.gpkgSchemaChanged = true;
    }
  }

  // also download files which were added both on the server and locally (the local version will be renamed as conflicting copy)
  for ( QString filePath : transaction.diff.conflictRemoteAddedLocalAdded )
  {
    MerginFile file = serverProject.fileInfo( filePath );
    QList<DownloadQueueItem> items = itemsForFileChunks( file, transaction.version );
    transaction.pullTasks << PullTask( PullTask::CopyConflict, filePath, items );
    transaction.gpkgSchemaChanged = true;
  }

  // schedule removed files to be deleted
  for ( QString filePath : transaction.diff.remoteDeleted )
  {
    transaction.pullTasks << PullTask( PullTask::Delete, filePath, QList<DownloadQueueItem>() );
  }

  // prepare the download queue
  for ( const PullTask &item : transaction.pullTasks )
  {
    transaction.downloadQueue << item.data;
  }

  qint64 totalSize = 0;
  for ( const DownloadQueueItem &item : transaction.downloadQueue )
  {
    totalSize += item.size;
  }
  transaction.totalSize = totalSize;

  // order download queue from the largest to smallest chunks to better
  // work with parallel downloads
  std::sort(
    transaction.downloadQueue.begin(), transaction.downloadQueue.end(),
  []( const DownloadQueueItem & a, const DownloadQueueItem & b ) { return a.size > b.size; }
  );

  CoreUtils::log( "pull " + projectFullName, QStringLiteral( "%1 of available device storage, %2 of total device storage" )
                  .arg( CoreUtils::getAvailableDeviceStorage() )
                  .arg( CoreUtils::getTotalDeviceStorage() ) );

  CoreUtils::log( "pull " + projectFullName, QStringLiteral( "%1 update tasks, %2 items to download (total size %3 bytes)" )
                  .arg( transaction.pullTasks.count() )
                  .arg( transaction.downloadQueue.count() )
                  .arg( transaction.totalSize ) );

  emit pullFilesStarted();

  if ( transaction.downloadQueue.isEmpty() )
  {
    finalizeProjectPull( projectFullName );
  }
  else
  {
    while ( transaction.replyPullItems.count() < 5 && !transaction.downloadQueue.isEmpty() )
    {
      downloadNextItem( projectFullName );
    }
  }
}

void MerginApi::prepareDownloadConfig( const QString &projectFullName, bool downloaded )
{
  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  MerginProjectMetadata newServerVersion = MerginProjectMetadata::fromJson( transaction.projectMetadata );

  const auto res = std::find_if( newServerVersion.files.begin(), newServerVersion.files.end(), []( const MerginFile & file )
  {
    return file.path == sMerginConfigFile;
  } );
  bool serverContainsConfig = res != newServerVersion.files.end();

  if ( serverContainsConfig )
  {
    if ( !downloaded )
    {
      // we should have server config but we do not have it yet
      return requestServerConfig( projectFullName );
    }
  }

  MerginProjectMetadata oldServerVersion = MerginProjectMetadata::fromCachedJson( transaction.projectDir + "/" + sMetadataFile );

  const auto resOld = std::find_if( oldServerVersion.files.begin(), oldServerVersion.files.end(), []( const MerginFile & file )
  {
    return file.path == sMerginConfigFile;
  } );

  bool previousVersionContainedConfig = ( resOld != oldServerVersion.files.end() ) && !transaction.firstTimeDownload;

  if ( !transaction.config.isValid )
  {
    // if transaction is not valid (or missing), consider it as deleted
    transaction.config.downloadMissingFiles = true;
    CoreUtils::log( "MerginConfig", "No config detected" );
  }
  else if ( serverContainsConfig && previousVersionContainedConfig )
  {
    // config was there, check if there are changes
    QString newChk = newServerVersion.fileInfo( sMerginConfigFile ).checksum;
    QString oldChk = oldServerVersion.fileInfo( sMerginConfigFile ).checksum;

    if ( newChk == oldChk )
    {
      // config files are the same
    }
    else
    {
      // config was changed, but what changed?
      MerginConfig oldConfig = MerginConfig::fromFile( transaction.projectDir + "/" + MerginApi::sMerginConfigFile );

      if ( oldConfig.selectiveSyncEnabled != transaction.config.selectiveSyncEnabled )
      {
        // selective sync was enabled/disabled
        if ( transaction.config.selectiveSyncEnabled )
        {
          CoreUtils::log( "MerginConfig", "Selective sync has been enabled" );
        }
        else
        {
          CoreUtils::log( "MerginConfig", "Selective sync has been disabled, downloading missing files." );
          transaction.config.downloadMissingFiles = true;
        }
      }
      else if ( oldConfig.selectiveSyncDir != transaction.config.selectiveSyncDir )
      {
        CoreUtils::log( "MerginConfig", "Selective sync directory has changed, downloading missing files." );
        transaction.config.downloadMissingFiles = true;
      }
      else
      {
        CoreUtils::log( "MerginConfig", "Unknown change in config file, continuing with latest version." );
      }
    }
  }
  else if ( serverContainsConfig )
  {
    CoreUtils::log( "MerginConfig", "Detected new config file." );
  }
  else if ( previousVersionContainedConfig ) // and current does not
  {
    CoreUtils::log( "MerginConfig", "Config file was removed, downloading missing files." );
    transaction.config.downloadMissingFiles = true;
  }
  else // no config in last versions
  {
    // pull like without config
    transaction.configAllowed = false;
    transaction.config.isValid = false;

    // if it would be possible to add mergin-config locally, it needs to be checked here
  }

  startProjectPull( projectFullName );
}

void MerginApi::requestServerConfig( const QString &projectFullName )
{
  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  QUrl url( mApiRoot + QStringLiteral( "/v1/project/raw/" ) + projectFullName );
  QUrlQuery query;

  query.addQueryItem( "file", sMerginConfigFile.toUtf8().toPercentEncoding() );
  query.addQueryItem( "version", QStringLiteral( "v%1" ).arg( transaction.version ) );
  url.setQuery( query );

  QNetworkRequest request = getDefaultRequest();
  request.setUrl( url );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ), projectFullName );

  Q_ASSERT( !transaction.replyPullServerConfig );
  transaction.replyPullServerConfig = mManager.get( request );
  connect( transaction.replyPullServerConfig, &QNetworkReply::finished, this, &MerginApi::cacheServerConfig );

  CoreUtils::log( "pull " + projectFullName, QStringLiteral( "Requesting mergin config: " ) + url.toString() );
}

QList<DownloadQueueItem> MerginApi::itemsForFileChunks( const MerginFile &file, int version )
{
  QList<DownloadQueueItem> lst;
  qint64 from = 0;
  while ( from < file.size )
  {
    qint64 size = qMin( MerginApi::UPLOAD_CHUNK_SIZE, file.size - from );
    lst << DownloadQueueItem( file.path, size, version, from, from + size - 1 );
    from += size;
  }
  return lst;
}

QList<DownloadQueueItem> MerginApi::itemsForFileDiffs( const MerginFile &file )
{
  QList<DownloadQueueItem> items;
  // download diffs instead of full download of gpkg file from server
  for ( const auto &d : file.pullDiffFiles )
  {
    items << DownloadQueueItem( file.path, d.second, d.first, -1, -1, true );
  }
  return items;
}


static MerginFile findFile( const QString &filePath, const QList<MerginFile> &files )
{
  for ( const MerginFile &merginFile : files )
  {
    if ( merginFile.path == filePath )
      return merginFile;
  }
  CoreUtils::log( QStringLiteral( "MerginFile" ), QStringLiteral( "requested findFile() for non-existant file: %1" ).arg( filePath ) );
  return MerginFile();
}


void MerginApi::pushInfoReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();

  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];
  Q_ASSERT( r == transaction.replyPushProjectInfo );

  if ( r->error() == QNetworkReply::NoError )
  {
    QString url = r->url().toString();
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "Downloaded project info." ) );
    QByteArray data = r->readAll();

    transaction.replyPushProjectInfo->deleteLater();
    transaction.replyPushProjectInfo = nullptr;

    LocalProject projectInfo = mLocalProjects.projectFromMerginName( projectFullName );
    transaction.projectDir = projectInfo.projectDir;
    Q_ASSERT( !transaction.projectDir.isEmpty() );

    // get the latest server version from our reply (we do not update it in LocalProjectsManager though... I guess we don't need to)
    MerginProjectMetadata serverProject = MerginProjectMetadata::fromJson( data );

    // now let's figure a key question: are we on the most recent version of the project
    // if we're about to do upload? because if not, we need to do pull first
    if ( projectInfo.isValid() && projectInfo.localVersion != -1 && projectInfo.localVersion < serverProject.version )
    {
      CoreUtils::log( "push " + projectFullName, QStringLiteral( "Need pull first: local version %1 | server version %2" )
                      .arg( projectInfo.localVersion ).arg( serverProject.version ) );
      transaction.pullBeforePush = true;
      prepareProjectPull( projectFullName, data );
      return;
    }

    QList<MerginFile> localFiles = getLocalProjectFiles( transaction.projectDir + "/" );
    MerginProjectMetadata oldServerProject = MerginProjectMetadata::fromCachedJson( transaction.projectDir + "/" + sMetadataFile );

    // Cache mergin-config, since we are on the most recent version, it is sufficient to just read the local version
    if ( transaction.configAllowed )
    {
      transaction.config = MerginConfig::fromFile( transaction.projectDir + "/" + MerginApi::sMerginConfigFile );
    }

    transaction.diff = compareProjectFiles(
                         oldServerProject.files,
                         serverProject.files,
                         localFiles,
                         transaction.projectDir,
                         transaction.configAllowed,
                         transaction.config
                       );

    CoreUtils::log( "push " + projectFullName, transaction.diff.dump() );

    // TODO: make sure there are no remote files to add/update/remove nor conflicts

    QList<MerginFile> filesToUpload;
    QList<MerginFile> addedMerginFiles, updatedMerginFiles, deletedMerginFiles;
    QList<MerginFile> diffFiles;
    for ( QString filePath : transaction.diff.localAdded )
    {
      MerginFile merginFile = findFile( filePath, localFiles );
      merginFile.chunks = generateChunkIdsForSize( merginFile.size );
      addedMerginFiles.append( merginFile );
    }

    for ( QString filePath : transaction.diff.localUpdated )
    {
      MerginFile merginFile = findFile( filePath, localFiles );
      merginFile.chunks = generateChunkIdsForSize( merginFile.size );

      if ( MerginApi::isFileDiffable( filePath ) )
      {
        // try to create a diff
        QString diffName;
        int geodiffRes = GeodiffUtils::createChangeset( transaction.projectDir, filePath, diffName );
        QString diffPath = transaction.projectDir + "/.mergin/" + diffName;
        QString basePath = transaction.projectDir + "/.mergin/" + filePath;

        if ( geodiffRes == GEODIFF_SUCCESS )
        {
          QByteArray checksumDiff = CoreUtils::calculateChecksum( diffPath );

          // TODO: this is ugly. our basefile may not need to have the same checksum as the server's
          // basefile (because each of them have applied the diff independently) so we have to fake it
          QByteArray checksumBase = serverProject.fileInfo( filePath ).checksum.toLatin1();

          merginFile.diffName = diffName;
          merginFile.diffChecksum = QString::fromLatin1( checksumDiff.data(), checksumDiff.size() );
          merginFile.diffSize = QFileInfo( diffPath ).size();
          merginFile.chunks = generateChunkIdsForSize( merginFile.diffSize );
          merginFile.diffBaseChecksum = QString::fromLatin1( checksumBase.data(), checksumBase.size() );

          diffFiles.append( merginFile );

          CoreUtils::log( "push " + projectFullName, QString( "Geodiff create changeset on %1 successful: total size %2 bytes" ).arg( filePath ).arg( merginFile.diffSize ) );
        }
        else
        {
          // TODO: remove the diff file (if exists)
          CoreUtils::log( "push " + projectFullName, QString( "Geodiff create changeset on %1 FAILED with error %2 (will do full upload)" ).arg( filePath ).arg( geodiffRes ) );
        }
      }

      updatedMerginFiles.append( merginFile );
    }

    for ( QString filePath : transaction.diff.localDeleted )
    {
      MerginFile merginFile = findFile( filePath, serverProject.files );
      deletedMerginFiles.append( merginFile );
    }

    if ( addedMerginFiles.isEmpty() && updatedMerginFiles.isEmpty() && deletedMerginFiles.isEmpty() )
    {
      // if nothing has changed, there is no point to even start upload transaction
      transaction.projectMetadata = data;
      transaction.version = MerginProjectMetadata::fromJson( data ).version;

      finishProjectSync( projectFullName, true );
      return;
    }

    QJsonArray added = prepareUploadChangesJSON( addedMerginFiles );
    filesToUpload.append( addedMerginFiles );

    QJsonArray modified = prepareUploadChangesJSON( updatedMerginFiles );
    filesToUpload.append( updatedMerginFiles );

    QJsonArray removed = prepareUploadChangesJSON( deletedMerginFiles );
    // removed not in filesToUpload

    QJsonObject changes;
    changes.insert( "added", added );
    changes.insert( "removed", removed );
    changes.insert( "updated", modified );
    changes.insert( "renamed", QJsonArray() );

    qint64 totalSize = 0;
    for ( MerginFile file : filesToUpload )
    {
      if ( !file.diffName.isEmpty() )
        totalSize += file.diffSize;
      else
        totalSize += file.size;
    }

    CoreUtils::log( "push " + projectFullName, QStringLiteral( "%1 items to upload (total size %2 bytes)" )
                    .arg( filesToUpload.count() ).arg( totalSize ) );

    transaction.totalSize = totalSize;
    transaction.pushQueue = filesToUpload;
    transaction.pushDiffFiles = diffFiles;

    QJsonObject json;
    json.insert( QStringLiteral( "changes" ), changes );
    json.insert( QStringLiteral( "version" ), QString( "v%1" ).arg( serverProject.version ) );
    QJsonDocument jsonDoc;
    jsonDoc.setObject( json );

    pushStart( projectFullName, jsonDoc.toJson( QJsonDocument::Compact ) );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    if ( r->error() == QNetworkReply::OperationCanceledError )
      serverMsg = sSyncCanceledMessage;

    QString message = QStringLiteral( "Network API error: %1(): %2" ).arg( QStringLiteral( "projectInfo" ), r->errorString() );
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "FAILED - %1" ).arg( message ) );

    int httpCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: pushInfo" ), httpCode, projectFullName );

    transaction.replyPushProjectInfo->deleteLater();
    transaction.replyPushProjectInfo = nullptr;

    finishProjectSync( projectFullName, false );
  }
}

void MerginApi::pushFinishReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();

  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];
  Q_ASSERT( r == transaction.replyPushFinish );

  if ( r->error() == QNetworkReply::NoError )
  {
    Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
    QByteArray data = r->readAll();
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "Transaction finish accepted" ) );

    transaction.replyPushFinish->deleteLater();
    transaction.replyPushFinish = nullptr;

    transaction.projectMetadata = data;
    transaction.version = MerginProjectMetadata::fromJson( data ).version;

    //  a new diffable files suppose to have their basefile copies in .mergin
    for ( QString filePath : transaction.diff.localAdded )
    {
      if ( MerginApi::isFileDiffable( filePath ) )
      {
        QString basefile = transaction.projectDir + "/.mergin/" + filePath;
        createPathIfNotExists( basefile );

        QString sourcePath = transaction.projectDir + "/" + filePath;
        if ( !QFile::copy( sourcePath, basefile ) )
        {
          CoreUtils::log( "push " + projectFullName, "failed to copy new basefile for: " + filePath );
        }
      }
    }

    // clean up diff-related files
    const auto diffFiles = transaction.pushDiffFiles;
    for ( const MerginFile &merginFile : diffFiles )
    {
      QString diffPath = transaction.projectDir + "/.mergin/" + merginFile.diffName;

      // update basefile (unmodified file that should be equivalent to the server)
      QString basePath = transaction.projectDir + "/.mergin/" + merginFile.path;
      bool res = GeodiffUtils::applyChangeset( basePath, diffPath );
      if ( res )
      {
        CoreUtils::log( "push " + projectFullName, QString( "Applied %1 to base file of %2" ).arg( merginFile.diffName, merginFile.path ) );
      }
      else
      {
        CoreUtils::log( "push " + projectFullName, QString( "Failed to apply changeset %1 to basefile %2 - error %3" ).arg( diffPath ).arg( basePath ).arg( res ) );
      }

      // remove temporary diff files
      if ( !QFile::remove( diffPath ) )
        CoreUtils::log( "push " + projectFullName, "Failed to remove diff: " + diffPath );
    }

    finishProjectSync( projectFullName, true );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    if ( r->error() == QNetworkReply::OperationCanceledError )
      serverMsg = sSyncCanceledMessage;

    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "pushFinish" ), r->errorString(), serverMsg );
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "FAILED - %1" ).arg( message ) );

    int httpCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: pushFinish" ), httpCode, projectFullName );

    // remove temporary diff files
    const auto diffFiles = transaction.pushDiffFiles;
    for ( const MerginFile &merginFile : diffFiles )
    {
      QString diffPath = transaction.projectDir + "/.mergin/" + merginFile.diffName;
      if ( !QFile::remove( diffPath ) )
        CoreUtils::log( "push " + projectFullName, "Failed to remove diff: " + diffPath );
    }

    transaction.replyPushFinish->deleteLater();
    transaction.replyPushFinish = nullptr;

    finishProjectSync( projectFullName, false );
  }
}

void MerginApi::pushCancelReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "Transaction canceled" ) );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "uploadCancel" ), r->errorString(), serverMsg );
    CoreUtils::log( "push " + projectFullName, QStringLiteral( "FAILED - %1" ).arg( message ) );
  }

  emit pushCanceled( projectFullName, r->error() == QNetworkReply::NoError );

  r->deleteLater();
}

void MerginApi::getUserInfoFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "user info", QStringLiteral( "Success" ) );
    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    if ( doc.isObject() )
    {
      QJsonObject docObj = doc.object();
      mUserInfo->setFromJson( docObj );
      if ( mServerType == MerginServerType::OLD )
      {
        mWorkspaceInfo->setFromJson( docObj );
      }
    }
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "getUserInfo" ), r->errorString(), serverMsg );
    CoreUtils::log( "user info", QStringLiteral( "FAILED - %1" ).arg( message ) );
    mUserInfo->clear();

    // This is an ugly fix for #3261: if the user was logged in, but the token was already expired
    // (e.g. when starting the app the next day), the flow of network requests and handlers gets
    // confused because of mAuthLoopEvent involved when re-authenticating user to get new token.
    // We end up requesting user info even with expired token, which of course fails with HTTP code 401
    // and user gets "Authentication information is missing or invalid." notification - this code
    // prevents that. The correct solution is to get rid of the QEventLoop and to have more rigorous
    // flow of network requests.
    static bool firstTimeExpiredTokenAnd401 = true;
    if ( firstTimeExpiredTokenAnd401 && r->attribute( QNetworkRequest::HttpStatusCodeAttribute ) == 401 &&
         !mUserAuth->authToken().isEmpty() && mUserAuth->tokenExpiration() < QDateTime().currentDateTimeUtc() )
    {
      firstTimeExpiredTokenAnd401 = false;
    }
    else
    {
      emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: getUserInfo" ) );
    }
  }

  emit userInfoReplyFinished();

  r->deleteLater();
}

void MerginApi::getWorkspaceInfoReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "workspace info", QStringLiteral( "Success" ) );
    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    if ( doc.isObject() )
    {
      QJsonObject docObj = doc.object();
      mWorkspaceInfo->setFromJson( docObj );

      emit getWorkspaceInfoFinished();
    }
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "getWorkspaceInfo" ), r->errorString(), serverMsg );
    CoreUtils::log( "workspace info", QStringLiteral( "FAILED - %1" ).arg( message ) );
    mWorkspaceInfo->clear();
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: getWorkspaceInfo" ) );
  }

  r->deleteLater();
}

bool MerginApi::hasLocalChanges(
  const QList<MerginFile> &oldServerFiles,
  const QList<MerginFile> &localFiles,
  const QString &projectDir
)
{
  if ( localFiles.count() != oldServerFiles.count() )
  {
    return true;
  }

  QHash<QString, MerginFile> oldServerFilesMap;

  for ( const MerginFile &file : oldServerFiles )
  {
    oldServerFilesMap.insert( file.path, file );
  }

  for ( const MerginFile &localFile : localFiles )
  {
    QString filePath = localFile.path;
    bool hasOldServer = oldServerFilesMap.contains( localFile.path );

    if ( !hasOldServer )
    {
      // L-A
      return true;
    }
    else
    {
      const QString chkOld = oldServerFilesMap.value( localFile.path ).checksum;
      const QString chkLocal = localFile.checksum;

      if ( chkOld != chkLocal )
      {
        if ( isFileDiffable( filePath ) )
        {
          // we need to do a diff here to figure out whether the file is actually changed or not
          // because the real content may be the same although the checksums do not match
          // e.g. when GPKG is opened, its header is updated and therefore lastModified timestamp/checksum is updated as well.
          if ( GeodiffUtils::hasPendingChanges( projectDir, filePath ) )
          {
            // L-U
            return true;
          }
        }
        else
        {
          // L-U
          return true;
        }
      }
    }
  }

  // We know that the number of local files and old server is the same
  // And also that all local files has old file counterpart
  // So it is not possible that there is deleted local file at this point.
  return false;
}

ProjectDiff MerginApi::compareProjectFiles(
  const QList<MerginFile> &oldServerFiles,
  const QList<MerginFile> &newServerFiles,
  const QList<MerginFile> &localFiles,
  const QString &projectDir,
  bool allowConfig,
  const MerginConfig &config,
  const MerginConfig &lastSyncConfig
)
{
  ProjectDiff diff;
  QHash<QString, MerginFile> oldServerFilesMap, newServerFilesMap;

  for ( MerginFile file : newServerFiles )
  {
    newServerFilesMap.insert( file.path, file );
  }
  for ( MerginFile file : oldServerFiles )
  {
    oldServerFilesMap.insert( file.path, file );
  }

  for ( MerginFile localFile : localFiles )
  {
    QString filePath = localFile.path;
    bool hasOldServer = oldServerFilesMap.contains( localFile.path );
    bool hasNewServer = newServerFilesMap.contains( localFile.path );
    QString chkOld = oldServerFilesMap.value( localFile.path ).checksum;
    QString chkNew = newServerFilesMap.value( localFile.path ).checksum;
    QString chkLocal = localFile.checksum;

    if ( !hasOldServer && !hasNewServer )
    {
      // L-A
      diff.localAdded << filePath;
    }
    else if ( hasOldServer && !hasNewServer )
    {
      if ( chkOld == chkLocal )
      {
        // R-D
        diff.remoteDeleted << filePath;
      }
      else
      {
        // C/R-D/L-U
        diff.conflictRemoteDeletedLocalUpdated << filePath;
      }
    }
    else if ( !hasOldServer && hasNewServer )
    {
      if ( chkNew != chkLocal )
      {
        // C/R-A/L-A
        diff.conflictRemoteAddedLocalAdded << filePath;
      }
      else
      {
        // R-A/L-A
        // TODO: need to do anything?
      }
    }
    else if ( hasOldServer && hasNewServer )
    {
      // file has already existed
      if ( chkOld == chkNew )
      {
        if ( chkNew != chkLocal )
        {
          // L-U
          if ( isFileDiffable( filePath ) )
          {
            // we need to do a diff here to figure out whether the file is actually changed or not
            // because the real content may be the same although the checksums do not match
            if ( GeodiffUtils::hasPendingChanges( projectDir, filePath ) )
              diff.localUpdated << filePath;
          }
          else
            diff.localUpdated << filePath;
        }
        else
        {
          // no change :-)
        }
      }
      else   // v1 != v2
      {
        if ( chkNew != chkLocal && chkOld != chkLocal )
        {
          // C/R-U/L-U
          if ( isFileDiffable( filePath ) )
          {
            // we need to do a diff here to figure out whether the file is actually changed or not
            // because the real content may be the same although the checksums do not match
            if ( GeodiffUtils::hasPendingChanges( projectDir, filePath ) )
              diff.conflictRemoteUpdatedLocalUpdated << filePath;
            else
              diff.remoteUpdated << filePath;
          }
          else
            diff.conflictRemoteUpdatedLocalUpdated << filePath;
        }
        else if ( chkNew != chkLocal )  // && old == local
        {
          // R-U
          diff.remoteUpdated << filePath;
        }
        else if ( chkOld != chkLocal )  // && new == local
        {
          // R-U/L-U
          // TODO: need to do anything?
        }
        else
          Q_ASSERT( false );   // impossible - should be handled already
      }
    }

    if ( hasOldServer )
      oldServerFilesMap.remove( filePath );
    if ( hasNewServer )
      newServerFilesMap.remove( filePath );
  }

  // go through files listed on the server, but not available locally
  for ( MerginFile file : newServerFilesMap )
  {
    bool hasOldServer = oldServerFilesMap.contains( file.path );

    if ( hasOldServer )
    {
      if ( oldServerFilesMap.value( file.path ).checksum == file.checksum )
      {
        // L-D
        if ( allowConfig )
        {
          bool shouldBeExcludedFromSync = MerginApi::excludeFromSync( file.path, config );
          if ( shouldBeExcludedFromSync )
          {
            continue;
          }

          // check if we should download missing files that were previously ignored (e.g. selective sync has been disabled)
          bool previouslyIgnoredButShouldDownload = \
            config.downloadMissingFiles &&
            lastSyncConfig.isValid &&
            MerginApi::excludeFromSync( file.path, lastSyncConfig );

          if ( previouslyIgnoredButShouldDownload )
          {
            diff.remoteAdded << file.path;
            continue;
          }
        }
        diff.localDeleted << file.path;
      }
      else
      {
        // C/R-U/L-D
        diff.conflictRemoteUpdatedLocalDeleted << file.path;
      }
    }
    else
    {
      // R-A
      if ( allowConfig )
      {
        if ( MerginApi::excludeFromSync( file.path, config ) )
        {
          continue;
        }
      }
      diff.remoteAdded << file.path;
    }

    if ( hasOldServer )
      oldServerFilesMap.remove( file.path );
  }

  /*
  for ( MerginFile file : oldServerFilesMap )
  {
    // R-D/L-D
    // TODO: need to do anything?
  }
  */

  return diff;
}

MerginProject MerginApi::parseProjectMetadata( const QJsonObject &proj )
{
  MerginProject project;

  if ( proj.isEmpty() )
  {
    return project;
  }

  if ( proj.contains( QStringLiteral( "error" ) ) )
  {
    // handle project error (user might be logged out / do not have write rights / project is on different server / project is orphaned)
    project.remoteError = QString::number( proj.value( QStringLiteral( "error" ) ).toInt( 0 ) ); // error code
    return project;
  }

  project.projectName = proj.value( QStringLiteral( "name" ) ).toString();
  project.projectNamespace = proj.value( QStringLiteral( "namespace" ) ).toString();

  QString versionStr = proj.value( QStringLiteral( "version" ) ).toString();
  if ( versionStr.isEmpty() )
  {
    project.serverVersion = 0;
  }
  else if ( versionStr.startsWith( "v" ) ) // cut off 'v' part from v123
  {
    versionStr = versionStr.mid( 1 );
    project.serverVersion = versionStr.toInt();
  }

  QDateTime updated = QDateTime::fromString( proj.value( QStringLiteral( "updated" ) ).toString(), Qt::ISODateWithMs ).toUTC();
  if ( !updated.isValid() )
  {
    project.serverUpdated = QDateTime::fromString( proj.value( QStringLiteral( "created" ) ).toString(), Qt::ISODateWithMs ).toUTC();
  }
  else
  {
    project.serverUpdated = updated;
  }
  return project;
}


MerginProjectsList MerginApi::parseProjectsFromJson( const QJsonDocument &doc )
{
  if ( !doc.isObject() )
    return MerginProjectsList();

  QJsonObject object = doc.object();
  MerginProjectsList result;

  if ( object.contains( "projects" ) && object.value( "projects" ).isArray() ) // listProjects API
  {
    QJsonArray vArray = object.value( "projects" ).toArray();

    for ( auto it = vArray.constBegin(); it != vArray.constEnd(); ++it )
    {
      result << parseProjectMetadata( it->toObject() );
    }
  }
  else if ( !object.isEmpty() ) // listProjectsbyName API returns projects as separate objects not in array
  {
    for ( auto it = object.begin(); it != object.end(); ++it )
    {
      MerginProject project = parseProjectMetadata( it->toObject() );
      if ( !project.remoteError.isEmpty() )
      {
        // add project namespace/name from object name in case of error
        MerginApi::extractProjectName( it.key(), project.projectNamespace, project.projectName );
      }
      result << project;
    }
  }
  return result;
}

void MerginApi::refreshAuthToken()
{
  if ( !mUserAuth->hasAuthData() ||
       mUserAuth->authToken().isEmpty() )
  {
    CoreUtils::log( QStringLiteral( "Auth" ), QStringLiteral( "Can not refresh token, missing credentials" ) );
    return;
  }

  if ( mUserAuth->tokenExpiration() < QDateTime::currentDateTimeUtc() )
  {
    CoreUtils::log( QStringLiteral( "Auth" ), QStringLiteral( "Token has expired, requesting new one" ) );
    authorize( mUserAuth->username(), mUserAuth->password() );
    mAuthLoopEvent.exec();
  }
}

QStringList MerginApi::generateChunkIdsForSize( qint64 fileSize )
{
  qreal rawNoOfChunks = qreal( fileSize ) / UPLOAD_CHUNK_SIZE;
  int noOfChunks = qCeil( rawNoOfChunks );

  // edge case when file is empty, filesize equals zero
  // manually set one chunk so that file will be synced
  if ( fileSize <= 0 )
    noOfChunks = 1;

  QStringList chunks;
  for ( int i = 0; i < noOfChunks; i++ )
  {
    QString chunkID = CoreUtils::uuidWithoutBraces( QUuid::createUuid() );
    chunks.append( chunkID );
  }
  return chunks;
}

QJsonArray MerginApi::prepareUploadChangesJSON( const QList<MerginFile> &files )
{
  QJsonArray jsonArray;

  for ( MerginFile file : files )
  {
    QJsonObject fileObject;
    fileObject.insert( "path", file.path );

    fileObject.insert( "size", file.size );
    fileObject.insert( "mtime", file.mtime.toString( Qt::ISODateWithMs ) );

    if ( !file.diffName.isEmpty() )
    {
      // doing diff-based upload
      QJsonObject diffObject;
      diffObject.insert( "path", file.diffName );
      diffObject.insert( "checksum", file.diffChecksum );
      diffObject.insert( "size", file.diffSize );

      fileObject.insert( "diff", diffObject );
      fileObject.insert( "checksum", file.diffBaseChecksum );
    }
    else
    {
      fileObject.insert( "checksum", file.checksum );
    }

    QJsonArray chunksJson;
    for ( QString id : file.chunks )
    {
      chunksJson.append( id );
    }
    fileObject.insert( "chunks", chunksJson );
    jsonArray.append( fileObject );
  }
  return jsonArray;
}

void MerginApi::finishProjectSync( const QString &projectFullName, bool syncSuccessful )
{
  Q_ASSERT( mTransactionalStatus.contains( projectFullName ) );
  TransactionStatus &transaction = mTransactionalStatus[projectFullName];

  emit syncProjectStatusChanged( projectFullName, -1 );   // -1 means there's no sync going on

  if ( syncSuccessful )
  {
    // update the local metadata file
    writeData( transaction.projectMetadata, transaction.projectDir + "/" + MerginApi::sMetadataFile );

    // update info of local projects
    mLocalProjects.updateLocalVersion( transaction.projectDir, transaction.version );

    CoreUtils::log( "sync " + projectFullName, QStringLiteral( "### Finished ###  New project version: %1\n" ).arg( transaction.version ) );
  }
  else
  {
    CoreUtils::log( "sync " + projectFullName, QStringLiteral( "### FAILED ###\n" ) );
  }

  bool pullBeforePush = transaction.pullBeforePush;
  QString projectDir = transaction.projectDir;  // keep it before the transaction gets removed
  ProjectDiff diff = transaction.diff;
  int newVersion = syncSuccessful ? transaction.version : -1;

  if ( transaction.gpkgSchemaChanged || projectFileHasBeenUpdated( diff ) )
  {
    emit projectReloadNeededAfterSync( projectFullName );
  }

  mTransactionalStatus.remove( projectFullName );

  if ( pullBeforePush )
  {
    CoreUtils::log( "sync " + projectFullName, QStringLiteral( "Continue with push after pull" ) );
    // we're done only with the download part before the actual upload - so let's continue with upload
    QString projectNamespace, projectName;
    extractProjectName( projectFullName, projectNamespace, projectName );
    pushProject( projectNamespace, projectName );
  }
  else
  {
    emit syncProjectFinished( projectFullName, syncSuccessful, newVersion );

    if ( syncSuccessful )
    {
      emit projectDataChanged( projectFullName );
    }
  }
}

bool MerginApi::writeData( const QByteArray &data, const QString &path )
{
  QFile file( path );
  createPathIfNotExists( path );
  if ( !file.open( QIODevice::WriteOnly ) )
  {
    return false;
  }

  file.write( data );
  file.close();

  return true;
}

bool MerginApi::updateCachedProjectRole( const QString &projectFullName, const QString &newRole )
{
  LocalProject project = mLocalProjects.projectFromMerginName( projectFullName );
  if ( !project.isValid() )
  {
    return false;
  }

  QString metadataPath = project.projectDir + "/" + sMetadataFile;

  QFile file( metadataPath );
  if ( !file.open( QIODevice::ReadOnly ) )
  {
    return false;
  }

  QByteArray data = file.readAll();
  file.close();

  QJsonDocument doc = QJsonDocument::fromJson( data );
  if ( !doc.isObject() )
  {
    return false;
  }

  QJsonObject obj = doc.object();
  obj["role"] = newRole;
  doc.setObject( obj );

  if ( !file.open( QIODevice::WriteOnly ) )
  {
    return false;
  }

  bool success = ( file.write( doc.toJson() ) != -1 );
  file.close();

  return success;
}

void MerginApi::createPathIfNotExists( const QString &filePath )
{
  QDir dir;
  if ( !dir.exists( mDataDir ) )
    dir.mkpath( mDataDir );

  QFileInfo newFile( filePath );
  if ( !newFile.absoluteDir().exists() )
  {
    if ( !dir.mkpath( newFile.absolutePath() ) )
    {
      CoreUtils::log( "create path", QString( "Creating a folder failed for path: %1" ).arg( filePath ) );
    }
  }
}

bool MerginApi::isInIgnore( const QFileInfo &info )
{
  return sIgnoreExtensions.contains( info.suffix() ) || sIgnoreFiles.contains( info.fileName() ) || info.filePath().contains( sMetadataFolder + "/" );
}

bool MerginApi::excludeFromSync( const QString &filePath, const MerginConfig &config )
{
  if ( config.isValid && config.selectiveSyncEnabled )
  {
    QFileInfo info( filePath );

    bool isExcludedFormat = sIgnoreImageExtensions.contains( info.suffix().toLower() );

    if ( !isExcludedFormat )
      return false;

    if ( config.selectiveSyncDir.isEmpty() )
    {
      return true; // we are ignoring photos in the entire project
    }
    else if ( filePath.startsWith( config.selectiveSyncDir ) )
    {
      return true; // we are ignoring photo in subfolder
    }
  }
  return false;
}

QSet<QString> MerginApi::listFiles( const QString &path )
{
  QSet<QString> files;
  QDirIterator it( path, QStringList() << QStringLiteral( "*" ), QDir::Files, QDirIterator::Subdirectories );
  while ( it.hasNext() )
  {
    it.next();
    if ( !isInIgnore( it.fileInfo() ) )
    {
      files << it.filePath().replace( path, "" );
    }
  }
  return files;
}

void MerginApi::deleteAccount()
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QNetworkRequest request = getDefaultRequest();
  QUrl url( mApiRoot + QStringLiteral( "/v1/user" ) );
  request.setUrl( url );
  QNetworkReply *reply = mManager.deleteResource( request );
  connect( reply, &QNetworkReply::finished, this, [this]() { this->deleteAccountFinished();} );
  CoreUtils::log( "delete account " + mUserAuth->username(), QStringLiteral( "Requesting account deletion: " ) + url.toString() );
}

void MerginApi::deleteAccountFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "delete account " + mUserAuth->username(), QStringLiteral( "Success" ) );

    // remove all local projects from the device
    LocalProjectsList projects = mLocalProjects.projects();
    for ( const LocalProject &info : projects )
    {
      mLocalProjects.removeLocalProject( info.id() );
    }

    clearAuth();

    emit accountDeleted( true );
  }
  else
  {
    int statusCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    CoreUtils::log( "delete account " + mUserAuth->username(), QStringLiteral( "FAILED - %1 %2. %3" ).arg( statusCode ).arg( r->errorString() ).arg( serverMsg ) );
    if ( statusCode == 422 )
    {
      emit userIsAnOrgOwnerError();
    }
    else
    {
      emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: deleteAccount" ) );
    }

    emit accountDeleted( false );
  }

  r->deleteLater();
}

void MerginApi::getServerConfig()
{
  QNetworkRequest request = getDefaultRequest();
  QString urlString = mApiRoot + QStringLiteral( "/config" );
  QUrl url( urlString );
  request.setUrl( url );

  QNetworkReply *reply = mManager.get( request );

  connect( reply, &QNetworkReply::finished, this, &MerginApi::getServerConfigReplyFinished );
  CoreUtils::log( "Config", QStringLiteral( "Requesting server configuration: " ) + url.toString() );
}

void MerginApi::getServerConfigReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "Config", QStringLiteral( "Success" ) );
    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    if ( doc.isObject() )
    {
      QString serverType = doc.object().value( QStringLiteral( "server_type" ) ).toString();
      QString apiVersion = doc.object().value( QStringLiteral( "version" ) ).toString();
      int major = -1;
      int minor = -1;
      bool validVersion = parseVersion( apiVersion, major, minor );

      if ( !validVersion )
      {
        CoreUtils::log( QStringLiteral( "Server version" ), QStringLiteral( "Cannot parse server version" ) );
      }

      if ( serverType == QStringLiteral( "ee" ) )
      {
        setServerType( MerginServerType::EE );
        if ( validVersion )
        {
          CoreUtils::log( QStringLiteral( "Server version" ), QStringLiteral( "%1.%2 EE" ).arg( major ).arg( minor ) );
        }
      }
      else if ( serverType == QStringLiteral( "ce" ) )
      {
        setServerType( MerginServerType::CE );
        if ( validVersion )
        {
          CoreUtils::log( QStringLiteral( "Server version" ), QStringLiteral( "%1.%2 CE" ).arg( major ).arg( minor ) );
        }
      }
      else if ( serverType == QStringLiteral( "saas" ) )
      {
        setServerType( MerginServerType::SAAS );
        if ( validVersion )
        {
          CoreUtils::log( QStringLiteral( "Server version" ), QStringLiteral( "%1.%2 SAAS" ).arg( major ).arg( minor ) );
        }
      }

      // will be dropped support for old servers (mostly CE servers without workspaces)
      if ( ( MINIMUM_SERVER_VERSION_MAJOR == major && MINIMUM_SERVER_VERSION_MINOR > minor ) || ( MINIMUM_SERVER_VERSION_MAJOR > major ) )
      {
        emit migrationRequested( QString( "%1.%2" ).arg( major ).arg( minor ) );
      }
    }
  }
  else
  {
    int statusCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    if ( statusCode == 404 ) // legacy (old) server
    {
      setServerType( MerginServerType::OLD );
      emit migrationRequested( "legacy" );
    }
    else
    {
      QString serverMsg = extractServerErrorMsg( r->readAll() );
      QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "getServerType" ), r->errorString(), serverMsg );
      emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: getServerType" ) );
      CoreUtils::log( "server type", QStringLiteral( "FAILED - %1" ).arg( message ) );
    }
  }

  r->deleteLater();
}

MerginServerType::ServerType MerginApi::serverType() const
{
  return mServerType;
}

void MerginApi::setServerType( const MerginServerType::ServerType &serverType )
{
  if ( mServerType != serverType )
  {
    if ( mServerType == MerginServerType::OLD && serverType == MerginServerType::SAAS )
    {
      emit serverWasUpgraded();
    }

    mServerType = serverType;
    QSettings settings;
    settings.beginGroup( QStringLiteral( "Input/" ) );
    settings.setValue( QStringLiteral( "serverType" ), mServerType );
    settings.endGroup();
    emit serverTypeChanged();
    emit apiSupportsWorkspacesChanged();
  }
}

void MerginApi::listWorkspaces()
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    emit listWorkspacesFailed();
    return;
  }

  QUrl url( mApiRoot + QStringLiteral( "/v1/workspaces" ) );
  QNetworkRequest request = getDefaultRequest( mUserAuth->hasAuthData() );
  request.setUrl( url );

  QNetworkReply *reply = mManager.get( request );
  CoreUtils::log( "list workspaces", QStringLiteral( "Requesting: " ) + url.toString() );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::listWorkspacesReplyFinished );
}

void MerginApi::listWorkspacesReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "list workspaces", QStringLiteral( "Success" ) );
    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    if ( doc.isArray() )
    {
      QMap<int, QString> workspaces;
      QJsonArray array = doc.array();
      for ( auto it = array.constBegin(); it != array.constEnd(); ++it )
      {
        QJsonObject ws = it->toObject();
        workspaces.insert( ws.value( QStringLiteral( "id" ) ).toInt(), ws.value( QStringLiteral( "name" ) ).toString() );
      }

      mUserInfo->setWorkspaces( workspaces );
      emit listWorkspacesFinished( workspaces );
    }
    else
    {
      emit listWorkspacesFailed();
    }
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "listWorkspaces" ), r->errorString(), serverMsg );
    CoreUtils::log( "list workspaces", QStringLiteral( "FAILED - %1" ).arg( message ) );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: listWorkspaces" ) );
    emit listWorkspacesFailed();
  }

  r->deleteLater();
}

void MerginApi::listInvitations()
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    emit listInvitationsFailed();
    return;
  }

  QUrl url( mApiRoot + QStringLiteral( "/v1/workspace/invitations" ) );
  QNetworkRequest request = getDefaultRequest( mUserAuth->hasAuthData() );
  request.setUrl( url );

  QNetworkReply *reply = mManager.get( request );
  CoreUtils::log( "list invitations", QStringLiteral( "Requesting: " ) + url.toString() );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::listInvitationsReplyFinished );
}

void MerginApi::listInvitationsReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "list invitations", QStringLiteral( "Success" ) );
    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    if ( doc.isArray() )
    {
      QList<MerginInvitation> invitations;
      QJsonArray array = doc.array();
      for ( auto it = array.constBegin(); it != array.constEnd(); ++it )
      {
        MerginInvitation invite = MerginInvitation::fromJsonObject( it->toObject() );
        invitations.append( invite );
      }

      emit listInvitationsFinished( invitations );
    }
    else
    {
      emit listInvitationsFailed();
    }
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "listInvitations" ), r->errorString(), serverMsg );
    CoreUtils::log( "list invitations", QStringLiteral( "FAILED - %1" ).arg( message ) );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: listInvitations" ) );
    emit listInvitationsFailed();
  }

  r->deleteLater();
}

void MerginApi::processInvitation( const QString &uuid, bool accept )
{
  if ( !validateAuth() || mApiVersionStatus != MerginApiStatus::OK )
  {
    emit processInvitationFailed();
    return;
  }

  QNetworkRequest request = getDefaultRequest( true );
  QString urlString = mApiRoot + QStringLiteral( "v1/workspace/invitation/%1" ).arg( uuid );
  QUrl url( urlString );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/json" );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrAcceptFlag ), accept );

  QJsonDocument jsonDoc;
  QJsonObject jsonObject;
  jsonObject.insert( QStringLiteral( "accept" ), accept );
  jsonDoc.setObject( jsonObject );
  QByteArray json = jsonDoc.toJson( QJsonDocument::Compact );
  QNetworkReply *reply = mManager.post( request, json );
  CoreUtils::log( "process invitation", QStringLiteral( "Requesting: " ) + url.toString() );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::processInvitationReplyFinished );
}

void MerginApi::processInvitationReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  bool accept = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrAcceptFlag ) ).toBool();

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "process invitation", QStringLiteral( "Success" ) );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "processInvitation" ), r->errorString(), serverMsg );
    CoreUtils::log( "process invitation", QStringLiteral( "FAILED - %1" ).arg( message ) );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: processInvitation" ) );
    emit processInvitationFailed();
  }

  emit processInvitationFinished( accept );

  r->deleteLater();
}

bool MerginApi::createWorkspace( const QString &workspaceName )
{
  if ( !validateAuth() )
  {
    emit missingAuthorizationError( workspaceName );
    return false;
  }

  if ( mApiVersionStatus != MerginApiStatus::OK )
  {
    return false;
  }

  if ( !CoreUtils::isValidName( workspaceName ) )
  {
    emit notifyError( tr( "Workspace name contains invalid characters" ) );
    return false;
  }

  QNetworkRequest request = getDefaultRequest();
  QUrl url( mApiRoot + QString( "/v1/workspace" ) );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/json" );
  request.setRawHeader( "Accept", "application/json" );
  request.setAttribute( static_cast<QNetworkRequest::Attribute>( AttrWorkspaceName ), workspaceName );

  QJsonDocument jsonDoc;
  QJsonObject jsonObject;
  jsonObject.insert( QStringLiteral( "name" ), workspaceName );
  jsonDoc.setObject( jsonObject );
  QByteArray json = jsonDoc.toJson( QJsonDocument::Compact );

  QNetworkReply *reply = mManager.post( request, json );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::createWorkspaceReplyFinished );
  CoreUtils::log( "create " + workspaceName, QStringLiteral( "Requesting workspace creation: " ) + url.toString() );

  return true;
}

void MerginApi::signOut()
{
  clearAuth();
}

void MerginApi::refreshUserData()
{
  getUserInfo();

  if ( apiSupportsWorkspaces() )
  {
    getWorkspaceInfo();
    // getServiceInfo is called automatically when workspace info finishes
  }
  else if ( mServerType == MerginServerType::OLD )
  {
    getServiceInfo();
  }
}

void MerginApi::createWorkspaceReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString workspaceName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrWorkspaceName ) ).toString();

  if ( r->error() == QNetworkReply::NoError )
  {
    CoreUtils::log( "create " + workspaceName, QStringLiteral( "Success" ) );
    emit workspaceCreated( workspaceName );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "FAILED - %1: %2" ).arg( r->errorString(), serverMsg );
    CoreUtils::log( "create " + workspaceName, message );

    int httpCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();

    if ( httpCode == 409 )
    {
      emit networkErrorOccurred( tr( "Workspace %1 already exists" ).arg( workspaceName ), QStringLiteral( "Mergin API error: createWorkspace" ), httpCode, workspaceName );
    }
    else
    {
      emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: createWorkspace" ), httpCode, workspaceName );
    }
  }
  r->deleteLater();
}

bool MerginApi::apiSupportsWorkspaces()
{
  if ( mServerType == MerginServerType::SAAS || mServerType == MerginServerType::EE )
  {
    return true;
  }
  else
  {
    return false;
  }
}

DownloadQueueItem::DownloadQueueItem( const QString &fp, qint64 s, int v, qint64 rf, qint64 rt, bool diff )
  : filePath( fp ), size( s ), version( v ), rangeFrom( rf ), rangeTo( rt ), downloadDiff( diff )
{
  tempFileName = CoreUtils::uuidWithoutBraces( QUuid::createUuid() );
}

void MerginApi::updateProjectMetadataRole( const QString &projectFullName )
{
  if ( projectFullName.isEmpty() )
  {
    return;
  }

  QString cachedRole = MerginApi::getCachedProjectRole( projectFullName );

  QNetworkReply *reply = getProjectInfo( projectFullName );
  if ( !reply )
  {
    emit projectMetadataRoleUpdated( projectFullName, cachedRole );
    return;
  }

  reply->request().setAttribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ), projectFullName );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::updateProjectMetadataRoleReplyFinished );
}

void MerginApi::updateProjectMetadataRoleReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = r->request().attribute( static_cast<QNetworkRequest::Attribute>( AttrProjectFullName ) ).toString();
  QString cachedRole = MerginApi::getCachedProjectRole( projectFullName );

  if ( r->error() == QNetworkReply::NoError )
  {
    QByteArray data = r->readAll();
    MerginProjectMetadata serverProject = MerginProjectMetadata::fromJson( data );
    QString role = serverProject.role;

    if ( role != cachedRole )
    {
      if ( updateCachedProjectRole( projectFullName, role ) )
      {
        emit projectMetadataRoleUpdated( projectFullName, role );
      }
      else
      {
        CoreUtils::log( "metadata", QString( "Failed to update cached role for project %1" ).arg( projectFullName ) );
      }
    }
  }
  else
  {
    emit projectMetadataRoleUpdated( projectFullName, cachedRole );
  }

  r->deleteLater();
}

QString MerginApi::getCachedProjectRole( const QString &projectFullName ) const
{
  if ( projectFullName.isEmpty() )
    return QString();

  QString projectDir = mLocalProjects.projectFromMerginName( projectFullName ).projectDir;

  if ( projectDir.isEmpty() )
    return QString();

  MerginProjectMetadata cachedProjectMetadata = MerginProjectMetadata::fromCachedJson( projectDir + "/" + sMetadataFile );

  return cachedProjectMetadata.role;
}
