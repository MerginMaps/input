/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QJsonArray>
#include <QSettings>

#include "merginworkspaceinfo.h"
#include "coreutils.h"

MerginWorkspaceInfo::MerginWorkspaceInfo( QObject *parent )
  : QObject( parent )
{
  clear();
}

void MerginWorkspaceInfo::clear()
{
  mDiskUsage = 0;
  mStorageLimit = 0;

  emit workspaceInfoChanged();
}

void MerginWorkspaceInfo::setFromJson( QJsonObject docObj )
{
  // parse storage data
  mDiskUsage = docObj.value( QStringLiteral( "disk_usage" ) ).toDouble();
  mStorageLimit = docObj.value( QStringLiteral( "storage" ) ).toDouble();

  emit workspaceInfoChanged();
}

double MerginWorkspaceInfo::diskUsage() const
{
  return mDiskUsage;
}

double MerginWorkspaceInfo::storageLimit() const
{
  return mStorageLimit;
}

void MerginWorkspaceInfo::onStorageChanged( double storage )
{
  mStorageLimit = storage;
  emit workspaceInfoChanged();
}
