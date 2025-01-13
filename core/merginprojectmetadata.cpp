/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "merginprojectmetadata.h"
#include "coreutils.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <QFile>

MerginFile MerginFile::fromJsonObject( const QJsonObject &merginFileInfo )
{
  MerginFile merginFile;
  merginFile.checksum = merginFileInfo.value( QStringLiteral( "checksum" ) ).toString();
  merginFile.path = merginFileInfo.value( QStringLiteral( "path" ) ).toString();
  merginFile.size = merginFileInfo.value( QStringLiteral( "size" ) ).toInteger();
  merginFile.mtime =  QDateTime::fromString( merginFileInfo.value( QStringLiteral( "mtime" ) ).toString(), Qt::ISODateWithMs ).toUTC();
  merginFile.diffSize = 0;

  if ( merginFileInfo.contains( QStringLiteral( "history" ) ) )
  {
    // "history" is only present if we request project info with "since=vXYZ" argument in the query.
    // It is useful for diffable files (.gpkg) to figure out which diff files we need to request
    // instead of having to download the whole .gpkg file.
    //
    // Changes to a diffable file can be pushed either through a diff or by uploading the whole file.
    // The latter is something we don't like to see as we can't apply geodiff processing - the clients
    // only should use it as the last resort - e.g. when table structures got modified.
    // If the whole history between our current version and the newest version is available with diffs,
    // then we can use them - otherwise we will have to pull the full .gpkg from server.

    QJsonObject history = merginFileInfo.value( QStringLiteral( "history" ) ).toObject();
    QList<int> versions;
    for ( QString key : history.keys() )
      versions << key.mid( 1 ).toInt();
    std::sort( versions.begin(), versions.end() );

    if ( versions.count() > 0 )
    {
      merginFile.pullCanUseDiff = true;
      for ( int key : versions )
      {
        QJsonObject obj = history.value( QString( "v%1" ).arg( key ) ).toObject();
        if ( obj.contains( "diff" ) )
        {
          QJsonObject diffObj = obj["diff"].toObject();
          qint64 fileSize = diffObj["size"].toInteger();
          merginFile.pullDiffFiles << qMakePair( key, fileSize );
        }
        else
        {
          // bad luck - a full file upload was done - we can't apply diffs
          merginFile.pullDiffFiles.clear();
          merginFile.pullCanUseDiff = false;
          break;
        }
      }
    }
    else
    {
      // this is not a diffable file if the history is empty ("history": {})
    }
  }

  return merginFile;
}


MerginProjectMetadata MerginProjectMetadata::fromJson( const QByteArray &data )
{
  MerginProjectMetadata project;

  QJsonDocument doc = QJsonDocument::fromJson( data );
  if ( !doc.isObject() )
  {
    qDebug() << "MerginProjectMetadata::fromJson: invalid content!";
    return project;
  }

  QJsonObject docObj = doc.object();

  // read metadata about project files
  QJsonValue vFiles = docObj.value( QStringLiteral( "files" ) );
  Q_ASSERT( vFiles.isArray() );
  QJsonArray vFilesArray = vFiles.toArray();
  for ( auto it = vFilesArray.constBegin(); it != vFilesArray.constEnd(); ++it )
  {
    project.files << MerginFile::fromJsonObject( it->toObject() );
  }

  project.name = docObj.value( QStringLiteral( "name" ) ).toString();
  project.projectNamespace = docObj.value( QStringLiteral( "namespace" ) ).toString();
  project.role = docObj.value( QStringLiteral( "role" ) ).toString();

  QString versionStr = docObj.value( QStringLiteral( "version" ) ).toString();
  if ( versionStr.isEmpty() )
  {
    project.version = 0;
  }
  else if ( versionStr.startsWith( "v" ) ) // cut off 'v' part from v123
  {
    versionStr = versionStr.mid( 1 );
    project.version = versionStr.toInt();
  }

  if ( docObj.contains( QStringLiteral( "id" ) ) )
  {
    project.projectId = docObj.value( QStringLiteral( "id" ) ).toString();
  }
  else
  {
    project.projectId.clear();
  }

  return project;
}

MerginProjectMetadata MerginProjectMetadata::fromCachedJson( const QString &metadataFilePath )
{
  QFile file( metadataFilePath );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    return fromJson( file.readAll() );
  }
  return MerginProjectMetadata();
}

MerginFile MerginProjectMetadata::fileInfo( const QString &filePath ) const
{
  for ( const MerginFile &merginFile : files )
  {
    if ( merginFile.path == filePath )
      return merginFile;
  }
  qDebug() << "requested fileInfo() for non-existant file! " << filePath;
  return MerginFile();
}

MerginConfig MerginConfig::fromJson( const QByteArray &data )
{
  QJsonDocument doc = QJsonDocument::fromJson( data );
  MerginConfig config;

  if ( doc.isObject() )
  {
    QJsonObject docObj = doc.object();
    config.selectiveSyncEnabled = docObj.value( QStringLiteral( "input-selective-sync" ) ).toBool( false );
    config.selectiveSyncDir = docObj.value( QStringLiteral( "input-selective-sync-dir" ) ).toString();
    config.isValid = true;
  }
  else
  {
    CoreUtils::log( QStringLiteral( "MerginConfig" ), QStringLiteral( "Invalid content of a config file!" ) );
  }

  return config;
}

MerginConfig MerginConfig::fromFile( const QString &filePath )
{
  MerginConfig config;
  QFile file( filePath );

  if ( file.open( QIODevice::ReadOnly ) )
  {
    QByteArray data = file.readAll();
    config = MerginConfig::fromJson( data );
  }

  return config;
}
