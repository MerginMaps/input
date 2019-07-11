#include "merginapi.h"

#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDate>
#include <QByteArray>
#include <QSet>
#include <QMessageBox>
#include <QUuid>
#include <QtMath>

#include "inpututils.h"

const QString MerginApi::sMetadataFile = QStringLiteral( "/.mergin/mergin.json" );

MerginApi::MerginApi( const QString &dataDir, QObject *parent )
  : QObject( parent )
  , mDataDir( dataDir )
{
  QObject::connect( this, &MerginApi::syncProjectFinished, this, &MerginApi::updateProjectMetadata );
  QObject::connect( this, &MerginApi::authChanged, this, &MerginApi::saveAuthData );
  QObject::connect( this, &MerginApi::serverProjectDeleted, this, &MerginApi::projectDeleted );
  QObject::connect( this, &MerginApi::apiRootChanged, this, &MerginApi::pingMergin );
  QObject::connect( this, &MerginApi::pingMerginFinished, this, &MerginApi::checkMerginVersion );
  QObject::connect( this, &MerginApi::downloadFileFinished, this, &MerginApi::continueDownloadFiles );

  loadAuthData();
  mMerginProjects = parseAllProjectsMetadata();
}

void MerginApi::listProjects( const QString &searchExpression, const QString &user,
                              const QString &flag, const QString &filterTag )
{
  if ( !validateAuthAndContinute() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QNetworkRequest request;
  // projects filtered by tag "input_use"
  QString urlString = mApiRoot + QStringLiteral( "/v1/project" );
  if ( !filterTag.isEmpty() )
  {
    urlString += QStringLiteral( "?tags=" ) + filterTag;
  }
  if ( !searchExpression.isEmpty() )
  {
    urlString += QStringLiteral( "&q=" ) + searchExpression;
  }
  if ( !flag.isEmpty() )
  {
    urlString += QStringLiteral( "&flag=%1&user=%2" ).arg( flag ).arg( user );
  }
  QUrl url( urlString );
  request.setUrl( url );
  request.setRawHeader( "Authorization", QByteArray( "Bearer " + mAuthToken ) );

  QNetworkReply *reply = mManager.get( request );
  InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::listProjectsReplyFinished );
}

void MerginApi::downloadFile( const QString &projectFullName, const QString &filename, const QString &version, int chunkNo )
{
  if ( !validateAuthAndContinute() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QNetworkRequest request;
  QUrl url( mApiRoot + QStringLiteral( "/v1/project/raw/%1?file=%2&version=%3" ).arg( projectFullName ).arg( filename ).arg( version ) );
  request.setUrl( url );
  request.setRawHeader( "Authorization", QByteArray( "Bearer " + mAuthToken ) );
  request.setAttribute( QNetworkRequest::User, QVariant( chunkNo ) );

  QString range;
  int from = UPLOAD_CHUNK_SIZE * chunkNo;
  int to = UPLOAD_CHUNK_SIZE * ( chunkNo + 1 ) - 1;
  range = QStringLiteral( "bytes=%1-%2" ).arg( from ).arg( to );
  request.setRawHeader( "Range", range.toUtf8() );

  mPendingRequests.insert( url, projectFullName );
  QNetworkReply *reply = mManager.get( request );
  mTransactionalStatus[projectFullName].openReply = reply;
  InputUtils::log( url.toString() + " Range: " + range, QStringLiteral( "STARTED" ) );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::downloadFileReplyFinished );
}

void MerginApi::uploadFile( const QString &projectFullName, const QString &transactionUUID, MerginFile file, int chunkNo )
{
  if ( !validateAuthAndContinute() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QString projectNamespace;
  QString projectName;
  extractProjectName( projectFullName, projectNamespace, projectName );
  QString projectDir = getProjectDir( projectNamespace, projectName );
  QString chunkID = file.chunks.at( chunkNo );

  QFile f( projectDir + "/" + file.path );
  QByteArray data;

  if ( f.open( QIODevice::ReadOnly ) )
  {
    f.seek( chunkNo * UPLOAD_CHUNK_SIZE );
    data = f.read( UPLOAD_CHUNK_SIZE );
  }

  QNetworkRequest request;
  QUrl url( mApiRoot + QStringLiteral( "/v1/project/push/chunk/%1/%2" ).arg( transactionUUID ).arg( chunkID ) );
  request.setUrl( url );
  request.setRawHeader( "Authorization", QByteArray( "Bearer " + mAuthToken ) );
  request.setRawHeader( "Content-Type", "application/octet-stream" );

  QNetworkReply *reply = mManager.post( request, data );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::uploadFileReplyFinished );
  mTransactionalStatus[projectFullName].openReply = reply;
  mPendingRequests.insert( url, projectFullName );
  InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
}

void MerginApi::uploadStart( const QString &projectFullName, const QByteArray &json )
{
  if ( !validateAuthAndContinute() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  // Has been canceled
  if ( mTransactionalStatus[projectFullName].transactionUUID.isEmpty() )
  {
    InputUtils::log( "uploadStarted", QStringLiteral( "ABORT" ) );
    return;
  }

  QNetworkRequest request;
  QUrl url( mApiRoot + QStringLiteral( "v1/project/push/%1" ).arg( projectFullName ) );
  request.setUrl( url );
  request.setRawHeader( "Authorization", QByteArray( "Bearer " + mAuthToken ) );
  request.setRawHeader( "Content-Type", "application/json" );

  QNetworkReply *reply = mManager.post( request, json );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::uploadStartReplyFinished );
  mPendingRequests.insert( url, projectFullName );
  mTransactionalStatus[projectFullName].openReply = reply;
  InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
}

void MerginApi::uploadCancel( const QString &projectFullName )
{
  if ( !validateAuthAndContinute() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  if ( !mTransactionalStatus.contains( projectFullName ) )
    return;

  QNetworkReply *reply = mTransactionalStatus[projectFullName].openReply;
  // There is an open transaction, abort it followed by calling cancelUpload again.
  if ( reply )
  {
    InputUtils::log( reply->url().toString(), QStringLiteral( "ABORT" ) );
    reply->abort();
  }
  // There is no connections, can send cancel request or call syncProjectFinished straight away
  else
  {
    mTransactionalStatus[projectFullName].files.clear();
    QString transactionUUID = mTransactionalStatus[projectFullName].transactionUUID;

    // Any transaction has not started yet
    if ( transactionUUID == projectFullName || transactionUUID.isEmpty() )
    {
      emit syncProjectFinished( QStringLiteral(), projectFullName, false );
    }
    else
    {
      QNetworkRequest request;
      QUrl url( mApiRoot + QStringLiteral( "v1/project/push/cancel/%1" ).arg( transactionUUID ) );
      request.setUrl( url );
      request.setRawHeader( "Authorization", QByteArray( "Bearer " + mAuthToken ) );
      request.setRawHeader( "Content-Type", "application/json" );
      mPendingRequests.insert( url, projectFullName );

      // To not include cancel reply to transactionStatus
      QNetworkReply *reply = mManager.post( request, QByteArray() );
      connect( reply, &QNetworkReply::finished, this, &MerginApi::uploadCancelReplyFinished );
      InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
    }
  }
}

void MerginApi::updateCancel( const QString &projectFullName )
{
  if ( !mTransactionalStatus.contains( projectFullName ) ) return;

  QNetworkReply *reply = mTransactionalStatus[projectFullName].openReply;
  if ( reply )
  {
    InputUtils::log( reply->url().toString(), QStringLiteral( "ABORT" ) );
    reply->abort();
  }
  else
  {
    MerginProject p = mTempMerginProjects.value( projectFullName );
    QDir projectDir( p.projectDir );
    if ( projectDir.exists() && projectDir.isEmpty() )
    {
      projectDir.removeRecursively();
    }
    QDir( getTempProjectDir( projectFullName ) ).removeRecursively();
    mTempMerginProjects.remove( projectFullName );
    emit syncProjectFinished( QStringLiteral(), projectFullName, false );
  }
}

void MerginApi::uploadFinish( const QString &projectFullName, const QString &transactionUUID )
{
  if ( !validateAuthAndContinute() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  if ( mTransactionalStatus[projectFullName].transactionUUID.isEmpty() )
  {
    return;
  }

  QNetworkRequest request;
  QUrl url( mApiRoot + QStringLiteral( "v1/project/push/finish/%1" ).arg( transactionUUID ) );
  request.setUrl( url );
  request.setRawHeader( "Authorization", QByteArray( "Bearer " + mAuthToken ) );
  request.setRawHeader( "Content-Type", "application/json" );

  QNetworkReply *reply = mManager.post( request, QByteArray() );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::uploadFinishReplyFinished );
  mPendingRequests.insert( url, projectFullName );
  mTransactionalStatus[projectFullName].openReply = reply;
  InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
}

void MerginApi::updateProject( const QString &projectNamespace, const QString &projectName )
{
  QString projectFullName = getFullProjectName( projectNamespace, projectName );
  QNetworkReply *reply = getProjectInfo( projectFullName );
  if ( reply )
  {
    mTransactionalStatus[projectFullName].openReply = reply;
    connect( reply, &QNetworkReply::finished, this, &MerginApi::updateInfoReplyFinished );
  }
}

void MerginApi::uploadProject( const QString &projectNamespace, const QString &projectName )
{
  bool onlyUpload = true;
  QString projectFullName = getFullProjectName( projectNamespace, projectName );
  QString projectDir = getProjectDir( projectNamespace, projectName );

  // Waiting for transaction UUID, just added projectFullName as a tag its pending
  TransactionStatus syncStatus;
  syncStatus.transactionUUID = projectFullName;
  mTransactionalStatus.insert( projectFullName, syncStatus );

  for ( std::shared_ptr<MerginProject> project : mMerginProjects )
  {
    if ( getFullProjectName( project->projectNamespace, project->name ) == projectFullName )
    {
      if ( project->clientUpdated < project->serverUpdated && project->serverUpdated > project->lastSyncClient.toUTC() )
      {
        onlyUpload = false;
      }
    }
  }

  if ( onlyUpload )
  {
    continueWithUpload( projectDir, projectFullName, true );
  }
  else
  {
    QMessageBox msgBox;
    msgBox.setText( QStringLiteral( "The project has been updated on the server in the meantime. Your files will be updated before upload." ) );
    msgBox.setInformativeText( "Do you want to continue?" );
    msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
    msgBox.setDefaultButton( QMessageBox::Cancel );

    if ( msgBox.exec() == QMessageBox::Cancel )
    {
      updateCancel( projectFullName ); // suppose to call syncProjectFinished
      return;
    }

    mWaitingForUpload.insert( projectFullName );
    updateProject( projectNamespace, projectName );
    connect( this, &MerginApi::syncProjectFinished, this, &MerginApi::continueWithUpload );
  }
}

void MerginApi::authorize( const QString &username, const QString &password )
{
  if ( username.contains( "@" ) )
  {
    mUsername = username.split( "@" ).first();
  }
  else
  {
    mUsername = username;
  }
  mPassword = password;

  QNetworkRequest request;
  QString urlString = mApiRoot + QStringLiteral( "v1/auth/login" );
  QUrl url( urlString );
  request.setUrl( url );
  request.setRawHeader( "Content-Type", "application/json" );

  QJsonDocument jsonDoc;
  QJsonObject jsonObject;
  jsonObject.insert( QStringLiteral( "login" ), mUsername );
  jsonObject.insert( QStringLiteral( "password" ), mPassword );
  jsonDoc.setObject( jsonObject );
  QByteArray json = jsonDoc.toJson( QJsonDocument::Compact );

  QNetworkReply *reply = mManager.post( request, json );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::authorizeFinished );
  InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
}

void MerginApi::getUserInfo( const QString &username )
{
  if ( !validateAuthAndContinute() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QNetworkRequest request;
  QString urlString = mApiRoot + QStringLiteral( "v1/user/" ) + username;
  QUrl url( urlString );
  request.setUrl( url );
  request.setRawHeader( "Authorization", QByteArray( "Bearer " + mAuthToken ) );

  QNetworkReply *reply = mManager.get( request );
  InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::getUserInfoFinished );
}

void MerginApi::clearAuth()
{
  mUsername = "";
  mPassword = "";
  mAuthToken.clear();
  mTokenExpiration.setTime( QTime() );
  mUserId = -1;
  mDiskUsage = 0;
  mStorageLimit = 0;
  emit authChanged();
}

void MerginApi::resetApiRoot()
{
  QSettings settings;
  settings.beginGroup( QStringLiteral( "Input/" ) );
  setApiRoot( defaultApiRoot() );
  settings.endGroup();
}

bool MerginApi::hasAuthData()
{
  return !mUsername.isEmpty() && !mPassword.isEmpty();
}

void MerginApi::createProject( const QString &projectNamespace, const QString &projectName )
{
  if ( !validateAuthAndContinute() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QNetworkRequest request;
  QUrl url( mApiRoot + QString( "/v1/project/%1" ).arg( projectNamespace ) );
  request.setUrl( url );
  request.setRawHeader( "Authorization", QByteArray( "Bearer " + mAuthToken ) );
  request.setRawHeader( "Content-Type", "application/json" );
  request.setRawHeader( "Accept", "application/json" );
  mPendingRequests.insert( url, getFullProjectName( projectNamespace, projectName ) );

  QJsonDocument jsonDoc;
  QJsonObject jsonObject;
  jsonObject.insert( QStringLiteral( "name" ), projectName );
  jsonObject.insert( QStringLiteral( "public" ), false );
  jsonDoc.setObject( jsonObject );
  QByteArray json = jsonDoc.toJson( QJsonDocument::Compact );

  QNetworkReply *reply = mManager.post( request, json );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::createProjectFinished );
  InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
}

void MerginApi::deleteProject( const QString &projectNamespace, const QString &projectName )
{
  if ( !validateAuthAndContinute() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return;
  }

  QNetworkRequest request;
  QUrl url( mApiRoot + QStringLiteral( "/v1/project/%1/%2" ).arg( projectNamespace ).arg( projectName ) );
  request.setUrl( url );
  request.setRawHeader( "Authorization", QByteArray( "Bearer " + mAuthToken ) );
  mPendingRequests.insert( url, getFullProjectName( projectNamespace, projectName ) );
  QNetworkReply *reply = mManager.deleteResource( request );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::deleteProjectFinished );
  InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
}

void MerginApi::clearTokenData()
{
  mTokenExpiration = QDateTime().currentDateTime().addDays( -42 ); // to make it expired arbitrary days ago
  mAuthToken.clear();
}

void MerginApi::addProject( std::shared_ptr<MerginProject> project )
{
  mMerginProjects.append( project );
}

ProjectList MerginApi::updateMerginProjectList( const ProjectList &serverProjects )
{
  QHash<QString, std::shared_ptr<MerginProject>> downloadedProjects;
  for ( std::shared_ptr<MerginProject> project : mMerginProjects )
  {
    if ( !project->projectDir.isEmpty() )
    {
      downloadedProjects.insert( getFullProjectName( project->projectNamespace, project->name ), project );
    }
  }

  if ( downloadedProjects.isEmpty() ) return serverProjects;

  for ( std::shared_ptr<MerginProject> project : serverProjects )
  {
    QString fullProjectName = getFullProjectName( project->projectNamespace, project->name );
    if ( downloadedProjects.contains( fullProjectName ) )
    {
      project->projectDir = downloadedProjects.value( fullProjectName ).get()->projectDir;
      QDateTime localUpdate = downloadedProjects.value( fullProjectName ).get()->clientUpdated.toUTC();
      project->lastSyncClient = downloadedProjects.value( fullProjectName ).get()->lastSyncClient.toUTC();
      QDateTime lastModified = getLastModifiedFileDateTime( project->projectDir );
      project->clientUpdated = localUpdate;
      project->status = getProjectStatus( project, lastModified );
    }
  }
  return serverProjects;
}

void MerginApi::deleteObsoleteFiles( const QString &projectPath )
{
  if ( !mObsoleteFiles.value( projectPath ).isEmpty() )
  {
    for ( QString filename : mObsoleteFiles.value( projectPath ) )
    {
      QFile file( projectPath + filename );
      file.remove();
    }
    mObsoleteFiles.remove( projectPath );
  }
}

void MerginApi::saveAuthData()
{
  QSettings settings;
  settings.beginGroup( "Input/" );
  settings.setValue( "username", mUsername );
  settings.setValue( "password", mPassword );
  settings.setValue( "userId", mUserId );
  settings.setValue( "token", mAuthToken );
  settings.setValue( "expire", mTokenExpiration );
  settings.setValue( "apiRoot", mApiRoot );
  settings.endGroup();
}

void MerginApi::createProjectFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    QString projectFullName = mPendingRequests.value( r->url() );
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    emit notify( QStringLiteral( "Project created" ) );
    emit projectCreated( projectFullName );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "FAILED - %1: %2" ).arg( r->errorString(), serverMsg );
    InputUtils::log( r->url().toString(), message );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: createProject" ) );
  }
  mPendingRequests.remove( r->url() );
  r->deleteLater();
}

void MerginApi::deleteProjectFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    QString projectFullName = mPendingRequests.value( r->url() );
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    emit notify( QStringLiteral( "Project deleted" ) );
    emit serverProjectDeleted( projectFullName );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: deleteProject" ) );
  }
  mPendingRequests.remove( r->url() );
  r->deleteLater();
}

void MerginApi::authorizeFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    if ( doc.isObject() )
    {
      QJsonObject docObj = doc.object();
      QJsonObject session = docObj.value( QStringLiteral( "session" ) ).toObject();
      mAuthToken = session.value( QStringLiteral( "token" ) ).toString().toUtf8();
      mTokenExpiration = QDateTime::fromString( session.value( QStringLiteral( "expire" ) ).toString(), Qt::ISODateWithMs ).toUTC();
      mUserId = docObj.value( QStringLiteral( "id" ) ).toInt();
      mDiskUsage = docObj.value( QStringLiteral( "disk_usage" ) ).toInt();
      mStorageLimit = docObj.value( QStringLiteral( "storage_limit" ) ).toInt();
    }
    emit authChanged();
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );
    QVariant statusCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    int status = statusCode.toInt();
    if ( status == 401 || status == 400 )
    {
      emit authFailed();
      emit notify( serverMsg );
    }
    else
    {
      emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: authorize" ) );
    }
    mUsername.clear();
    mPassword.clear();
    clearTokenData();
  }
  if ( mAuthLoopEvent.isRunning() )
  {
    mAuthLoopEvent.exit();
  }
  r->deleteLater();
}

void MerginApi::pingMerginReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );
  QString apiVersion;
  QString serverMsg;

  if ( r->error() == QNetworkReply::NoError )
  {
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    if ( doc.isObject() )
    {
      QJsonObject obj = doc.object();
      apiVersion = obj.value( QStringLiteral( "version" ) ).toString();
    }
  }
  else
  {
    serverMsg = extractServerErrorMsg( r->readAll() );
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );
  }
  r->deleteLater();
  emit pingMerginFinished( apiVersion, serverMsg );
}

ProjectList MerginApi::parseAllProjectsMetadata()
{
  QStringList entryList = QDir( mDataDir ).entryList( QDir::NoDotAndDotDot | QDir::Dirs );
  ProjectList projects;

  for ( QString folderName : entryList )
  {
    QFileInfo info( mDataDir + folderName + "/" + sMetadataFile );
    if ( info.exists() )
    {
      std::shared_ptr<MerginProject> project = readProjectMetadataFromPath( mDataDir + folderName );
      if ( project )
      {
        projects << project;
      }
    }
  }

  return projects;
}

void MerginApi::clearProject( std::shared_ptr<MerginProject> project )
{
  project->status = ProjectStatus::NoVersion;
  project->lastSyncClient = QDateTime();
  project->clientUpdated = QDateTime();
  project->serverUpdated = QDateTime();
  project->projectDir.clear();
  emit merginProjectsChanged();
}

QNetworkReply *MerginApi::getProjectInfo( const QString &projectFullName )
{
  if ( !validateAuthAndContinute() || mApiVersionStatus != MerginApiStatus::OK )
  {
    return nullptr;
  }

  QNetworkRequest request;
  QUrl url( mApiRoot + QStringLiteral( "/v1/project/%1" ).arg( projectFullName ) );

  request.setUrl( url );
  request.setRawHeader( "Authorization", QByteArray( "Bearer " + mAuthToken ) );

  mPendingRequests.insert( url, projectFullName );
  InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
  return mManager.get( request );
}

void MerginApi::projectDeleted( const QString &projecFullName )
{
  std::shared_ptr<MerginProject> project = getProject( projecFullName );
  if ( project )
    clearProject( project );
}

void MerginApi::projectDeletedOnPath( const QString &projectDir )
{
  for ( std::shared_ptr<MerginProject> project : mMerginProjects )
  {
    if ( project->projectDir == mDataDir + projectDir )
    {
      clearProject( project );
    }
  }
}

void MerginApi::loadAuthData()
{
  QSettings settings;
  settings.beginGroup( QStringLiteral( "Input/" ) );
  setApiRoot( settings.value( QStringLiteral( "apiRoot" ) ).toString() );
  mUsername = settings.value( QStringLiteral( "username" ) ).toString();
  mPassword = settings.value( QStringLiteral( "password" ) ).toString();
  mUserId = settings.value( QStringLiteral( "userId" ) ).toInt();
  mTokenExpiration = settings.value( QStringLiteral( "expire" ) ).toDateTime();
  mAuthToken = settings.value( QStringLiteral( "token" ) ).toByteArray();
}

bool MerginApi::validateAuthAndContinute()
{
  if ( !hasAuthData() )
  {
    emit authRequested();
    return false;
  }

  if ( mAuthToken.isEmpty() || mTokenExpiration < QDateTime().currentDateTime().toUTC() )
  {
    authorize( mUsername, mPassword );

    mAuthLoopEvent.exec();
  }
  return true;
}

void MerginApi::checkMerginVersion( QString apiVersion, QString msg )
{
  if ( msg.isEmpty() )
  {
    int major = -1;
    int minor = -1;
    QRegularExpression re;
    re.setPattern( QStringLiteral( "(?<major>\\d+)[.](?<minor>\\d+)" ) );
    QRegularExpressionMatch match = re.match( apiVersion );
    if ( match.hasMatch() )
    {
      major = match.captured( "major" ).toInt();
      minor = match.captured( "minor" ).toInt();
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

  // TODO remove, only for te4eting
  setApiVersionStatus( MerginApiStatus::OK );
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

QString MerginApi::extractServerErrorMsg( const QByteArray &data )
{
  QString serverMsg;
  QJsonDocument doc = QJsonDocument::fromJson( data );
  if ( doc.isObject() )
  {
    QJsonObject obj = doc.object();
    serverMsg = obj.value( "detail" ).toString();
  }
  else
  {
    serverMsg = data;
  }

  return serverMsg;
}

std::shared_ptr<MerginProject> MerginApi::getProject( const QString &projectFullName )
{
  for ( std::shared_ptr<MerginProject> project : mMerginProjects )
  {
    if ( projectFullName == getFullProjectName( project->projectNamespace, project->name ) )
    {
      return project;
    }
  }

  return std::shared_ptr<MerginProject>();
}

QString MerginApi::findUniqueProjectDirectoryName( QString path )
{
  QDir projectDir( path );
  if ( projectDir.exists() )
  {
    int i = 0;
    QFileInfo info( path + QString::number( i ) );
    while ( info.exists() && info.isDir() )
    {
      ++i;
      info.setFile( path + QString::number( i ) );
    }
    return path + QString::number( i );
  }
  else
  {
    return path;
  }
}

QString MerginApi::getProjectDir( const QString &projectNamespace, const QString &projectName )
{
  QString projectFullName = getFullProjectName( projectNamespace, projectName );
  std::shared_ptr<MerginProject> project = getProject( projectFullName );
  if ( project )
  {
    if ( project->projectDir.isEmpty() )
    {
      QString projectDirPath = findUniqueProjectDirectoryName( mDataDir + projectName );
      QDir projectDir( projectDirPath );
      if ( !projectDir.exists() )
      {
        QDir dir( "" );
        dir.mkdir( projectDirPath );
      }
      project->projectDir = projectDirPath;
      return projectDir.path();
    }
    else
    {
      return project->projectDir;
    }
  }
  return QStringLiteral();
}

QString MerginApi::getTempProjectDir( const QString &projectFullName )
{
  return mDataDir + TEMP_FOLDER + projectFullName;
}

QString MerginApi::getFullProjectName( QString projectNamespace, QString projectName )
{
  return QString( "%1/%2" ).arg( projectNamespace ).arg( projectName );
}

MerginApiStatus::VersionStatus MerginApi::apiVersionStatus() const
{
  return mApiVersionStatus;
}

void MerginApi::setApiVersionStatus( const MerginApiStatus::VersionStatus &apiVersionStatus )
{
  mApiVersionStatus = apiVersionStatus;
  emit apiVersionStatusChanged();
}

int MerginApi::userId() const
{
  return mUserId;
}

void MerginApi::setUserId( int userId )
{
  mUserId = userId;
}

int MerginApi::storageLimit() const
{
  return mStorageLimit;
}

int MerginApi::diskUsage() const
{
  return mDiskUsage;
}

void MerginApi::pingMergin()
{
  if ( mApiVersionStatus == MerginApiStatus::OK ) return;

  setApiVersionStatus( MerginApiStatus::PENDING );

  QNetworkRequest request;
  QUrl url( mApiRoot + QStringLiteral( "/ping" ) );
  request.setUrl( url );

  QNetworkReply *reply = mManager.get( request );
  InputUtils::log( url.toString(), QStringLiteral( "STARTED" ) );
  connect( reply, &QNetworkReply::finished, this, &MerginApi::pingMerginReplyFinished );
}

QString MerginApi::apiRoot() const
{
  return mApiRoot;
}

void MerginApi::setApiRoot( const QString &apiRoot )
{
  QSettings settings;
  settings.beginGroup( QStringLiteral( "Input/" ) );
  if ( apiRoot.isEmpty() )
  {
    mApiRoot = defaultApiRoot();
  }
  else
  {
    mApiRoot = apiRoot;
  }
  settings.setValue( QStringLiteral( "apiRoot" ), mApiRoot );
  settings.endGroup();
  setApiVersionStatus( MerginApiStatus::UNKNOWN );
  emit apiRootChanged();
}

QString MerginApi::username() const
{
  return mUsername;
}

ProjectList MerginApi::projects()
{
  return mMerginProjects;
}

QList<MerginFile> MerginApi::getLocalProjectFiles( const QString &projectPath )
{
  QList<MerginFile> merginFiles;
  QSet<QString> localFiles = listFiles( projectPath );
  for ( QString p : localFiles )
  {

    MerginFile file;
    QByteArray localChecksumBytes = getChecksum( projectPath + p );
    QString localChecksum = QString::fromLatin1( localChecksumBytes.data(), localChecksumBytes.size() );
    file.checksum = localChecksum;
    file.path = p;
    QFileInfo info( projectPath + p );
    file.size = info.size();
    file.mtime = info.lastModified();
    merginFiles.append( file );
  }
  return merginFiles;
}

void MerginApi::listProjectsReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    if ( mMerginProjects.isEmpty() )
    {
      mMerginProjects = parseAllProjectsMetadata();
    }

    QByteArray data = r->readAll();
    ProjectList serverProjects = parseListProjectsMetadata( data );
    mMerginProjects = updateMerginProjectList( serverProjects );
    emit merginProjectsChanged();
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "listProjects" ), r->errorString(), serverMsg );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: listProjects" ) );
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1" ).arg( message ) );
    mMerginProjects.clear();

    emit listProjectsFailed();
  }

  r->deleteLater();
  emit listProjectsFinished( mMerginProjects );
}

void MerginApi::takeFirstAndDownload( const QString &projectFullName, const QString &version )
{
  MerginFile nextFile = mTransactionalStatus[projectFullName].files.first();
  if ( !nextFile.size )
  {
    createEmptyFile( getTempProjectDir( projectFullName ) + "/" + nextFile.path );
    emit continueDownloadFiles( projectFullName, version, 0, true );
  }
  else
  {
    downloadFile( projectFullName, nextFile.path, version, 0 );
  }
}

void MerginApi::continueDownloadFiles( const QString &projectFullName, const QString &version, int lastChunkNo, bool successfully )
{
  if ( !successfully )
  {
    updateCancel( projectFullName );
    return;
  }

  MerginFile currentFile = mTransactionalStatus[projectFullName].files.first();
  if ( lastChunkNo + 1 <= currentFile.chunks.size() - 1 )
  {
    downloadFile( projectFullName, currentFile.path, version, lastChunkNo + 1 );
  }
  else
  {
    mTransactionalStatus[projectFullName].files.removeFirst();
    if ( !mTransactionalStatus[projectFullName].files.isEmpty() )
    {
      takeFirstAndDownload( projectFullName, version );
    }
    else
    {
      QString projectNamespace;
      QString projectName;
      extractProjectName( projectFullName, projectNamespace, projectName );
      QString projectDir = getProjectDir( projectNamespace, projectName );
      copyTempFilesToProject( projectDir, projectFullName );

      emit syncProjectFinished( projectDir, projectFullName, true );
    }
  }
}

void MerginApi::deleteReply( QNetworkReply *r, const QString &projectFullName )
{
  Q_ASSERT( r == mTransactionalStatus[projectFullName].openReply );
  r->deleteLater();
  mTransactionalStatus[projectFullName].openReply = nullptr;
}

void MerginApi::downloadFileReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = mPendingRequests.value( r->url() );
  mPendingRequests.remove( r->url() );

  QUrlQuery query( r->url().query() );
  QString filename = query.queryItemValue( "file" );
  QString version = query.queryItemValue( "version" );
  int chunkNo = r->request().attribute( QNetworkRequest::User ).toInt();

  if ( r->error() == QNetworkReply::NoError )
  {
    QString projectNamespace;
    QString projectName;
    extractProjectName( projectFullName, projectNamespace, projectName );
    QString projectDir = getProjectDir( projectNamespace, projectName );

    bool overwrite = true; // chunkNo == 0
    bool closeFile = false;

    QList<MerginFile> files = mTransactionalStatus[projectFullName].files;
    if ( !files.isEmpty() )
    {
      MerginFile file = mTransactionalStatus[projectFullName].files.first();
      overwrite  = file.chunks.size() <= 1;

      if ( chunkNo == file.chunks.size() - 1 )
      {
        closeFile = true;
      }
    }

    QString tempFoler = getTempProjectDir( projectFullName );
    createPathIfNotExists( tempFoler );
    QByteArray data = r->readAll();
    handleOctetStream( data, tempFoler, filename, closeFile, overwrite );
    mTransactionalStatus[projectFullName].transferedSize += data.size();

    emit syncProgressUpdated( projectFullName, mTransactionalStatus[projectFullName].transferedSize / mTransactionalStatus[projectFullName].totalSize );
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    deleteReply( r, projectFullName );
    // Send another request afterwards
    emit downloadFileFinished( projectFullName, version, chunkNo, true );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    if ( serverMsg.isEmpty() )
    {
      serverMsg = r->errorString();
    }
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );
    deleteReply( r, projectFullName );
    emit downloadFileFinished( projectFullName, version, chunkNo, false );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: downloadFile" ) );
  }
}

void MerginApi::uploadStartReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = mPendingRequests.value( r->url() );
  mPendingRequests.remove( r->url() );

  if ( r->error() == QNetworkReply::NoError )
  {
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    deleteReply( r, projectFullName );
    QString transactionUUID;
    if ( doc.isObject() )
    {
      QJsonObject docObj = doc.object();
      transactionUUID = docObj.value( QStringLiteral( "transaction" ) ).toString();
      mTransactionalStatus[projectFullName].transactionUUID = transactionUUID;
    }

    QList<MerginFile> files = mTransactionalStatus[projectFullName].files;
    if ( !files.isEmpty() )
    {
      MerginFile file = files.first();
      uploadFile( projectFullName, transactionUUID, file );
    }
    // else pushing only files to be removed
  }
  else
  {
    QVariant statusCode = r->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    int status = statusCode.toInt();
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString errorMsg = r->errorString();
    bool showAsDialog = status == 400 && serverMsg == QStringLiteral( "You have reached a data limit" );

    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );
    deleteReply( r, projectFullName );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: uploadStartReply" ), showAsDialog );
    uploadCancel( projectFullName );
  }
}

void MerginApi::uploadFileReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QString projectFullName = mPendingRequests.value( r->url() );
  mPendingRequests.remove( r->url() );

  QStringList params = ( r->url().toString().split( "/" ) );
  QString transactionUUID = params.at( params.length() - 2 );
  QString chunkID = params.at( params.length() - 1 );

  if ( r->error() == QNetworkReply::NoError )
  {
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    deleteReply( r, projectFullName );

    MerginFile currentFile = mTransactionalStatus[projectFullName].files.first();
    int chunkNo = currentFile.chunks.indexOf( chunkID );
    if ( chunkNo < currentFile.chunks.size() - 1 )
    {
      uploadFile( projectFullName, transactionUUID, currentFile, chunkNo + 1 );
    }
    else
    {
      mTransactionalStatus[projectFullName].transferedSize += currentFile.size;
      emit syncProgressUpdated( projectFullName, mTransactionalStatus[projectFullName].transferedSize / mTransactionalStatus[projectFullName].totalSize );
      mTransactionalStatus[projectFullName].files.removeFirst();

      if ( !mTransactionalStatus[projectFullName].files.isEmpty() )
      {
        MerginFile nextFile = mTransactionalStatus[projectFullName].files.first();
        uploadFile( projectFullName, transactionUUID, nextFile );
      }
      else
      {
        uploadFinish( projectFullName, transactionUUID );
      }
    }
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1. %2" ).arg( r->errorString(), serverMsg ) );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: downloadFile" ) );

    deleteReply( r, projectFullName );
    uploadCancel( projectFullName );
  }
}

void MerginApi::updateInfoReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QUrl url = r->url();
  QString projectNamespace;
  QString projectName;
  extractProjectName( url.path(), projectNamespace, projectName );
  QString projectFullName = getFullProjectName( projectNamespace, projectName );
  mPendingRequests.remove( url );
  QList<MerginFile> filesToDownload;

  if ( r->error() == QNetworkReply::NoError )
  {
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    QByteArray data = r->readAll();
    deleteReply( r, projectFullName );

    QString projectPath = getProjectDir( projectNamespace, projectName ) + "/";
    QList<MerginFile> localFiles = getLocalProjectFiles( projectPath );

    MerginProject serverProject = readProjectMetadata( data );
    serverProject.projectDir = projectPath;
    mTempMerginProjects.insert( projectNamespace + "/" + projectName, serverProject );

    ProjectDiff diff = compareProjectFiles( serverProject.files, localFiles );
    if ( !mWaitingForUpload.contains( projectFullName ) )
    {
      QSet<QString> obsoleteFiles;
      for ( MerginFile file : diff.removed )
      {
        obsoleteFiles.insert( file.path );
      }
      if ( !obsoleteFiles.isEmpty() )
      {
        QString projectPath = getProjectDir( projectNamespace, projectName );
        mObsoleteFiles.insert( projectPath, obsoleteFiles );
      }
    }

    TransactionStatus syncStatus;
    for ( MerginFile file : diff.added )
    {
      file.chunks = generateChunkIdsForSize( file.size ); // doesnt really matter whats there, only how many chunks are expected
      filesToDownload << file;
      syncStatus.totalSize += file.size;
    }

    for ( MerginFile file : diff.modified )
    {
      file.chunks = generateChunkIdsForSize( file.size ); // doesnt really matter whats there, only how many chunks are expected
      filesToDownload << file;
      syncStatus.totalSize += file.size;
    }

    if ( !filesToDownload.isEmpty() )
    {
      syncStatus.files = filesToDownload;
      mTransactionalStatus.insert( projectFullName, syncStatus );
      takeFirstAndDownload( projectFullName, serverProject.version );
      emit pullFilesStarted();
    }
  }
  else
  {
    QString message = QStringLiteral( "Network API error: %1(): %2" ).arg( QStringLiteral( "projectInfo" ), r->errorString() );
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1" ).arg( message ) );
    deleteReply( r, projectFullName );
    updateCancel( projectFullName );
  }
}

void MerginApi::uploadInfoReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QUrl url = r->url();
  QString projectNamespace;
  QString projectName;
  extractProjectName( url.path(), projectNamespace, projectName );
  QString projectFullName = getFullProjectName( projectNamespace, projectName );

  if ( r->error() == QNetworkReply::NoError )
  {
    QJsonObject changes;
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    QByteArray data = r->readAll();
    deleteReply( r, projectFullName );

    QString projectPath = getProjectDir( projectNamespace, projectName ) + "/";
    QList<MerginFile> localFiles = getLocalProjectFiles( projectPath );

    MerginProject serverProject = readProjectMetadata( data );
    ProjectDiff diff = compareProjectFiles( localFiles, serverProject.files );
    QList<MerginFile> filesToUpload;

    QJsonArray added = prepareUploadChangesJSON( diff.added );
    filesToUpload.append( diff.added );

    QJsonArray modified = prepareUploadChangesJSON( diff.modified );
    filesToUpload.append( diff.modified );

    QJsonArray removed = prepareUploadChangesJSON( diff.removed );
    // removed not in filesToUpload

    changes.insert( "added", added );
    changes.insert( "removed", removed );
    changes.insert( "updated", modified );
    changes.insert( "renamed", QJsonArray() );

    mPendingRequests.remove( url );
    for ( MerginFile file : filesToUpload )
    {
      mTransactionalStatus[projectFullName].totalSize += file.size;
    }
    mTransactionalStatus[projectFullName].files = filesToUpload;

    QJsonObject json;
    json.insert( QStringLiteral( "changes" ), changes );
    json.insert( QStringLiteral( "version" ), serverProject.version );
    QJsonDocument jsonDoc;
    jsonDoc.setObject( json );

    QString info = QString( "PUSH request - added: %1, updated: %2, removed: %3" )
                   .arg( InputUtils::filesToString( diff.added ) ).arg( InputUtils::filesToString( diff.modified ) )
                   .arg( InputUtils::filesToString( diff.removed ) );
    InputUtils::log( url.toString(), info );

    uploadStart( projectFullName, jsonDoc.toJson( QJsonDocument::Compact ) );
  }
  else
  {
    QString message = QStringLiteral( "Network API error: %1(): %2" ).arg( QStringLiteral( "projectInfo" ), r->errorString() );
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1" ).arg( message ) );
    deleteReply( r, projectFullName );
    uploadCancel( projectFullName );
  }

  mPendingRequests.remove( url );
  r->deleteLater();
}

void MerginApi::uploadFinishReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QUrl url = r->url();
  QString projectFullName = mPendingRequests.value( url );
  mPendingRequests.remove( url );

  if ( r->error() == QNetworkReply::NoError )
  {
    mTransactionalStatus[projectFullName].transactionUUID.clear();
    QByteArray data = r->readAll();
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    deleteReply( r, projectFullName );

    MerginProject project = readProjectMetadata( data );
    QString projectNamespace;
    QString projectName;
    extractProjectName( projectFullName, projectNamespace, projectName );
    QString projectDir = getProjectDir( projectNamespace, projectName ) + "/";
    mTempMerginProjects.insert( projectFullName, project );
    syncProjectFinished( projectDir, projectFullName, true );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "uploadFinish" ), r->errorString(), serverMsg );
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1" ).arg( message ) );
    deleteReply( r, projectFullName );
    uploadCancel( projectFullName );
  }
}

void MerginApi::uploadCancelReplyFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  QUrl url = r->url();
  QString projectFullName = mPendingRequests.value( url );
  mPendingRequests.remove( url );

  if ( r->error() == QNetworkReply::NoError )
  {
    mTransactionalStatus[projectFullName].transactionUUID.clear();
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "uploadCancel" ), r->errorString(), serverMsg );
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1" ).arg( message ) );
  }
  r->deleteLater();
  // if reply has error, uploadCancel send another push cancel request since true == !mTransactionalStatus[projectFullName].transactionUUID.isEmpty()
  uploadCancel( projectFullName );
}

void MerginApi::getUserInfoFinished()
{
  QNetworkReply *r = qobject_cast<QNetworkReply *>( sender() );
  Q_ASSERT( r );

  if ( r->error() == QNetworkReply::NoError )
  {
    InputUtils::log( r->url().toString(), QStringLiteral( "FINISHED" ) );
    QJsonDocument doc = QJsonDocument::fromJson( r->readAll() );
    if ( doc.isObject() )
    {
      QJsonObject docObj = doc.object();
      mDiskUsage = docObj.value( QStringLiteral( "disk_usage" ) ).toInt();
      mStorageLimit = docObj.value( QStringLiteral( "storage_limit" ) ).toInt();
    }
  }
  else
  {
    QString serverMsg = extractServerErrorMsg( r->readAll() );
    QString message = QStringLiteral( "Network API error: %1(): %2. %3" ).arg( QStringLiteral( "getUserInfo" ), r->errorString(), serverMsg );
    InputUtils::log( r->url().toString(), QStringLiteral( "FAILED - %1" ).arg( message ) );
    emit networkErrorOccurred( serverMsg, QStringLiteral( "Mergin API error: getUserInfo" ) );
  }

  r->deleteLater();
  emit userInfoChanged();
}

ProjectDiff MerginApi::compareProjectFiles( const QList<MerginFile> &newFiles, const QList<MerginFile> &currentFiles )
{
  ProjectDiff diff;
  QHash<QString, MerginFile> currentFilesMap;

  for ( MerginFile currentFile : currentFiles )
  {
    currentFilesMap.insert( currentFile.path, currentFile );
  }

  for ( MerginFile newFile : newFiles )
  {
    MerginFile currentFile = currentFilesMap.value( newFile.path );
    newFile.chunks = generateChunkIdsForSize( newFile.size );

    if ( currentFile.checksum.isEmpty() )
    {
      diff.added.append( newFile );
    }

    else if ( currentFile.checksum != newFile.checksum )
    {
      diff.modified.append( newFile );
    }

    currentFilesMap.remove( currentFile.path );
  }

  // Rest files are extra, therefore put to remove
  for ( MerginFile file : currentFilesMap )
  {
    diff.removed.append( file );
  }

  return diff;
}

ProjectList MerginApi::parseListProjectsMetadata( const QByteArray &data )
{
  ProjectList result;

  QJsonDocument doc = QJsonDocument::fromJson( data );
  if ( doc.isArray() )
  {
    QJsonArray vArray = doc.array();

    for ( auto it = vArray.constBegin(); it != vArray.constEnd(); ++it )
    {
      QJsonObject projectMap = it->toObject();
      MerginProject p;
      p.name = projectMap.value( QStringLiteral( "name" ) ).toString();
      p.projectNamespace = projectMap.value( QStringLiteral( "namespace" ) ).toString();
      p.creator = projectMap.value( QStringLiteral( "creator" ) ).toInt();

      QJsonValue meta = projectMap.value( QStringLiteral( "meta" ) );
      if ( meta.isObject() )
      {
        p.filesCount = meta.toObject().value( "files_count" ).toInt();
      }

      QJsonValue access = projectMap.value( QStringLiteral( "access" ) );
      if ( access.isObject() )
      {
        QJsonArray writers = access.toObject().value( "writers" ).toArray();
        for ( QJsonValueRef tag : writers )
        {
          p.writers.append( tag.toInt() );
        }
      }

      QJsonValue tags = projectMap.value( QStringLiteral( "tags" ) );
      if ( tags.isArray() )
      {
        for ( QJsonValueRef tag : tags.toArray() )
        {
          p.tags.append( tag.toString() );
        }
      }

      QDateTime updated = QDateTime::fromString( projectMap.value( QStringLiteral( "updated" ) ).toString(), Qt::ISODateWithMs ).toUTC();
      if ( !updated.isValid() )
      {
        p.serverUpdated = QDateTime::fromString( projectMap.value( QStringLiteral( "created" ) ).toString(), Qt::ISODateWithMs ).toUTC();
      }
      else
      {
        p.serverUpdated = updated;
      }

      result << std::make_shared<MerginProject>( p );
    }
  }
  return result;
}

std::shared_ptr<MerginProject> MerginApi::readProjectMetadataFromPath( const QString &projectPath, const QString &metadataFile )
{
  QFile file( QString( "%1/%2" ).arg( projectPath ).arg( metadataFile ) );
  if ( !file.exists() ) return std::shared_ptr<MerginProject>();

  QByteArray data;
  if ( file.open( QIODevice::ReadOnly ) )
  {
    data = file.readAll();
    file.close();
  }

  std::shared_ptr<MerginProject> p = std::make_shared<MerginProject>( readProjectMetadata( data ) );
  p->projectDir = projectPath;
  return p;
}

MerginProject MerginApi::readProjectMetadata( const QByteArray &data )
{
  MerginProject project;
  QList<MerginFile> projectFiles;

  QJsonDocument doc = QJsonDocument::fromJson( data );
  if ( doc.isObject() )
  {
    QJsonObject docObj = doc.object();
    auto it = docObj.constFind( QStringLiteral( "files" ) );
    QJsonValue v = *it;
    Q_ASSERT( v.isArray() );
    QJsonArray vArray = v.toArray();

    for ( auto it = vArray.constBegin(); it != vArray.constEnd(); ++it )
    {
      QJsonObject merginFileInfo = it->toObject();
      // Include metadata of file from server in temp project's file
      MerginFile merginFile;
      merginFile.checksum = merginFileInfo.value( QStringLiteral( "checksum" ) ).toString();
      merginFile.path = merginFileInfo.value( QStringLiteral( "path" ) ).toString();
      merginFile.size = merginFileInfo.value( QStringLiteral( "size" ) ).toInt();
      merginFile.mtime =  QDateTime::fromString( merginFileInfo.value( QStringLiteral( "mtime" ) ).toString(), Qt::ISODateWithMs ).toUTC();
      projectFiles << merginFile;
    }

    // Save data from server to update metadata after successful request
    project.name = docObj.value( QStringLiteral( "name" ) ).toString();
    project.projectNamespace = docObj.value( QStringLiteral( "namespace" ) ).toString();
    project.version = docObj.value( QStringLiteral( "version" ) ).toString();
    project.serverUpdated = QDateTime::fromString( docObj.value( QStringLiteral( "updated" ) ).toString(), Qt::ISODateWithMs ).toUTC();
    if ( project.version.isEmpty() )
    {
      project.version = QStringLiteral( "v1" );
    }
    // extra data to server
    project.clientUpdated = QDateTime::fromString( docObj.value( QStringLiteral( "clientUpdated" ) ).toString(), Qt::ISODateWithMs ).toUTC();
    project.lastSyncClient = QDateTime::fromString( docObj.value( QStringLiteral( "lastSync" ) ).toString(), Qt::ISODateWithMs ).toUTC();

    project.files = projectFiles;
  }
  return project;
}

QJsonDocument MerginApi::createProjectMetadataJson( std::shared_ptr<MerginProject> project )
{
  QJsonDocument doc;
  QJsonObject projectMap;
  projectMap.insert( QStringLiteral( "clientUpdated" ), project->clientUpdated.toString( Qt::ISODateWithMs ) );
  projectMap.insert( QStringLiteral( "lastSync" ), project->lastSyncClient.toString( Qt::ISODateWithMs ) );
  projectMap.insert( QStringLiteral( "name" ), project->name );
  projectMap.insert( QStringLiteral( "namespace" ), project->projectNamespace );
  projectMap.insert( QStringLiteral( "version" ), project->version );

  QJsonArray filesArray;
  for ( MerginFile file : project->files )
  {
    QJsonObject fileObject;
    fileObject.insert( "path", file.path );
    fileObject.insert( "checksum", file.checksum );
    fileObject.insert( "size", file.size );
    fileObject.insert( "mtime", file.mtime.toString( Qt::ISODateWithMs ) );
    filesArray.append( fileObject );
  }
  projectMap.insert( QStringLiteral( "files" ), filesArray );

  doc.setObject( projectMap );
  return doc;
}

QStringList MerginApi::generateChunkIdsForSize( qint64 fileSize )
{
  qreal rawNoOfChunks = qreal( fileSize ) / UPLOAD_CHUNK_SIZE;
  int noOfChunks = qCeil( rawNoOfChunks );
  QStringList chunks;
  for ( int i = 0; i < noOfChunks; i++ )
  {
    QString chunkID = QUuid::createUuid().toString( QUuid::WithoutBraces );
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

    fileObject.insert( "checksum", file.checksum );
    fileObject.insert( "size", file.size );
    fileObject.insert( "mtime", file.mtime.toString( Qt::ISODateWithMs ) );

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

void MerginApi::updateProjectMetadata( const QString &projectDir, const QString &projectFullName, bool syncSuccessful )
{
  mTransactionalStatus.remove( projectFullName );
  if ( !syncSuccessful )
  {
    return;
  }
  MerginProject tempProjectData = mTempMerginProjects.take( projectFullName );
  std::shared_ptr<MerginProject> project = getProject( projectFullName );
  if ( project )
  {
    project->clientUpdated = project->serverUpdated;
    if ( project->projectDir.isEmpty() )
      project->projectDir = projectDir;
    project->lastSyncClient = QDateTime::currentDateTime().toUTC();
    project->files = tempProjectData.files;
    project->version = tempProjectData.version;

    QJsonDocument doc = createProjectMetadataJson( project );
    writeData( doc.toJson(), projectDir + "/" + MerginApi::sMetadataFile );

    emit merginProjectsChanged();
  }
}

void MerginApi::copyTempFilesToProject( const QString &projectDir, const QString &projectFullName )
{
  QString tempProjectDir = getTempProjectDir( projectFullName );
  InputUtils::cpDir( tempProjectDir, projectDir );
  QDir( tempProjectDir ).removeRecursively();
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

void MerginApi::continueWithUpload( const QString &projectDir, const QString &projectFullName, bool successfully )
{
  Q_UNUSED( projectDir )

  disconnect( this, &MerginApi::syncProjectFinished, this, &MerginApi::continueWithUpload );
  mWaitingForUpload.remove( projectFullName );

  if ( !successfully )
  {
    return;
  }

  // Has been canceled
  if ( mTransactionalStatus[projectFullName].transactionUUID.isEmpty() )
  {
    InputUtils::log( "continueWithUpload", QStringLiteral( "ABORT" ) );
    return;
  }

  QNetworkReply *reply = getProjectInfo( projectFullName );
  if ( reply )
  {
    mTransactionalStatus[projectFullName].openReply = reply;
    connect( reply, &QNetworkReply::finished, this, &MerginApi::uploadInfoReplyFinished );
  }
}

void MerginApi::handleOctetStream( const QByteArray &data, const QString &projectDir, const QString &filename, bool closeFile, bool overwrite )
{
  QFile file;
  QString activeFilePath = projectDir + '/' + filename;
  file.setFileName( activeFilePath );
  createPathIfNotExists( activeFilePath );
  saveFile( data, file, closeFile, overwrite );
}

bool MerginApi::saveFile( const QByteArray &data, QFile &file, bool closeFile, bool overwrite )
{
  if ( !file.isOpen() )
  {
    if ( overwrite )
    {
      if ( !file.open( QIODevice::WriteOnly ) )
      {
        return false;
      }
    }
    else
    {
      if ( !file.open( QIODevice::Append ) )
      {
        return false;
      }
    }
  }

  file.write( data );
  if ( closeFile )
    file.close();

  return true;
}

void MerginApi::createPathIfNotExists( const QString &filePath )
{
  QDir dir;
  if ( !dir.exists( mDataDir ) )
    dir.mkpath( mDataDir );

  QFileInfo newFile( filePath );
  if ( !newFile.absoluteDir().exists() )
  {
    if ( !QDir( dir ).mkpath( newFile.absolutePath() ) )
    {
      InputUtils::log( QString( "Creating a folder failed for path: %1" ).arg( filePath ) );
    }
  }
}

void MerginApi::createEmptyFile( const QString &path )
{
  QDir dir;
  QFileInfo info( path );
  QString parentDir( info.dir().path() );
  if ( !dir.exists( parentDir ) )
    dir.mkpath( parentDir );

  QFile file( path );
  file.open( QIODevice::ReadWrite );
  file.close();
}

ProjectStatus MerginApi::getProjectStatus( std::shared_ptr<MerginProject> project, const QDateTime &lastModified )
{
  // There was no sync yet
  if ( !project->clientUpdated.isValid() )
  {
    return ProjectStatus::NoVersion;
  }

  // Something has locally changed after last sync with server
  int filesCount = getProjectFilesCount( project->projectDir );
  if ( project->lastSyncClient < lastModified || project->filesCount != filesCount )
  {
    return ProjectStatus::Modified;
  }

  // Version is lower than latest one, last sync also before updated
  if ( project->clientUpdated < project->serverUpdated && project->serverUpdated > project->lastSyncClient )
  {
    return ProjectStatus::OutOfDate;
  }

  return ProjectStatus::UpToDate;
}

QDateTime MerginApi::getLastModifiedFileDateTime( const QString &path )
{
  QDateTime lastModified;
  QDirIterator it( path, QStringList() << QStringLiteral( "*" ), QDir::Files, QDirIterator::Subdirectories );
  while ( it.hasNext() )
  {
    it.next();
    if ( !isInIgnore( it.fileInfo() ) )
    {
      if ( it.fileInfo().lastModified() > lastModified )
      {
        lastModified = it.fileInfo().lastModified();
      }
    }
  }
  return lastModified.toUTC();
}

int MerginApi::getProjectFilesCount( const QString &path )
{
  int count = 0;
  QDirIterator it( path, QStringList() << QStringLiteral( "*" ), QDir::Files, QDirIterator::Subdirectories );
  while ( it.hasNext() )
  {
    it.next();
    if ( !isInIgnore( it.fileInfo() ) )
    {
      count++;
    }
  }
  return count;
}

bool MerginApi::isInIgnore( const QFileInfo &info )
{
  return mIgnoreExtensions.contains( info.suffix() ) || mIgnoreFiles.contains( info.fileName() );
}

QByteArray MerginApi::getChecksum( const QString &filePath )
{
  QFile f( filePath );
  if ( f.open( QFile::ReadOnly ) )
  {
    QCryptographicHash hash( QCryptographicHash::Sha1 );
    QByteArray chunk = f.read( CHUNK_SIZE );
    while ( !chunk.isEmpty() )
    {
      hash.addData( chunk );
      chunk = f.read( CHUNK_SIZE );
    }
    f.close();
    return hash.result().toHex();
  }

  return QByteArray();
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
