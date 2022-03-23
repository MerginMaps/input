/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SYNCHRONIZATIONMANAGER_H
#define SYNCHRONIZATIONMANAGER_H

#include <QObject>

#include "project.h"
#include "merginapi.h"

struct SyncProcess
{
  qreal progress;
  bool pending;
  // In future: current state (push/pull)
};

class SynchronizationManager : public QObject
{
    Q_OBJECT

  public:

    explicit SynchronizationManager( MerginApi *merginApi, QObject *parent = nullptr );

    virtual ~SynchronizationManager();

    //! Stops a running sync process if there is one for project specified by projectFullname
    void stopProjectSync( const QString &projectFullName );

    void migrateProjectToMergin( const QString &projectName );

    //! Returns sync progress of specified project in range <0, 1>. Returns -1 if this project is not being synchronized.
    qreal syncProgress( const QString &projectFullName ) const;

    //! Returns true if specified project is being synchronized, false otherwise.
    bool hasPendingSync( const QString &projectFullName ) const;

    QList<QString> pendingProjects() const;

  signals:

    // Synchronization signals
    void syncStarted( const QString &projectFullName );
    void syncCancelled( const QString &projectFullName );
    void syncProgressChanged( const QString &projectFullName, qreal progress );
    void syncFinished( const QString &projectFullName, bool success, int newVersion );

  public slots:

    /**
     * \brief syncProject Starts synchronization of a project if there are local/server changes to be applied
     *
     * \param project Project struct instance
     * \param withAut Bears an information whether authorization should be included in sync requests.
     *                Authorization can be omitted for pull of public projects
     */
    void syncProject( const LocalProject &project, bool withAuth = true );

    //! Overloaded method, allows to sync with Project instance. Can be used in case of first download of remote project (it has invalid LocalProject info).
    void syncProject( const Project &project, bool withAuth = true );

    // Handling of synchronization changes from MerginApi
    void onProjectSyncCanceled( const QString &projectFullName, bool hasError );
    void onProjectSyncFinished( const QString &projectDir, const QString &projectFullName, bool successfully, int version );
    void onProjectSyncProgressChanged( const QString &projectFullName, qreal progress );

  private:

    // Hashmap of currently running synchronizations, key: project full name
    QHash<QString, SyncProcess> mSyncProcesses;

    MerginApi *mMerginApi = nullptr; // not owned
};

#endif // SYNCHRONIZATIONMANAGER_H
