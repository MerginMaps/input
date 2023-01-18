/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef MERGINUSERINFO_H
#define MERGINUSERINFO_H

#include <QObject>
#include <QString>
#include <QJsonObject>

#include "merginsubscriptionstatus.h"
#include "merginsubscriptiontype.h"


struct MerginInvitation
{
  QString uuid;
  QString workspace;
  QString role;
  QDateTime expiration;

  static MerginInvitation fromJsonObject( const QJsonObject &invitationInfo );
};


class MerginUserInfo: public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString name READ name NOTIFY userInfoChanged )
    Q_PROPERTY( QString email READ email NOTIFY userInfoChanged )
    Q_PROPERTY( QString activeWorkspaceName READ activeWorkspaceName NOTIFY userInfoChanged )
    Q_PROPERTY( int activeWorkspaceId READ activeWorkspaceId NOTIFY activeWorkspaceChanged )

  public:
    explicit MerginUserInfo( QObject *parent = nullptr );
    ~MerginUserInfo() = default;

    void clear();
    void setFromJson( QJsonObject docObj );

    QString name() const;
    QString email() const;
    QString activeWorkspaceName() const;
    int activeWorkspaceId() const;
    QMap<int, QString> workspaces() const;
    QList<MerginInvitation> invitations() const;

    void saveWorkspacesData();
    void loadWorkspacesData();
    void clearCachedWorkspacesInfo();

    int findActiveWorkspace( int preferredWorkspace = -1 );
    Q_INVOKABLE void setActiveWorkspace( int newWorkspace );
    void setWorkspaces( QMap<int, QString> workspaces );

  signals:
    void userInfoChanged();
    void activeWorkspaceChanged();

  private:
    QString mName;
    QString mEmail;
    QMap<int, QString> mWorkspaces;
    QList<MerginInvitation> mInvitations;
    int mActiveWorkspace = -1;
};

#endif // MERGINUSERINFO_H
