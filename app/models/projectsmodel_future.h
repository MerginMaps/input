/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROJECTSMODEL_FUTURE_H
#define PROJECTSMODEL_FUTURE_H

#include <QAbstractListModel>
#include <memory>

#include "localprojectsmanager.h"
#include "project_future.h"
#include "merginapi.h"

/**
 * \brief The ProjectModelTypes enum
 */
enum ProjectModelTypes
{
  LocalProjectsModel = 0,
  MyProjectsModel,
  SharedProjectsModel,
  ExploreProjectsModel,
  RecentProjectsModel
};

/**
 * \brief The ProjectsModel_future class
 */
class ProjectsModel_future : public QAbstractListModel
{
    Q_OBJECT

  public:

    enum Roles
    {
      Project = Qt::UserRole + 1
    };
    Q_ENUMS( Roles )

    ProjectsModel_future( MerginApi *merginApi, ProjectModelTypes modelType, LocalProjectsManager &localProjectsManager, QObject *parent = nullptr );
    ~ProjectsModel_future() override {};

    // Needed methods from QAbstractListModel
//    Q_INVOKABLE QVariant data( const QModelIndex &index, int role ) const override;
//    Q_INVOKABLE QModelIndex index( int row, int column = 0, const QModelIndex &parent = QModelIndex() ) const override;
//    QHash<int, QByteArray> roleNames() const override;
//    int rowCount( const QModelIndex &parent = QModelIndex() ) const override;

    //! Called to list projects, either fetch more or get first
    Q_INVOKABLE void listProjects();

    //! Method detecting local project for remote projects
    void mergeProjects();

  public slots:
    void listProjectsFinished();
    void projectSyncFinished();
    void projectSyncProgressChanged();

  private:

    QString modelTypeToFlag() const;

    MerginApi *mBackend;
    LocalProjectsManager &mLocalProjectsManager;
    QList<Project_future> mProjects;

    ProjectModelTypes mModelType;

    //! For pagination
    int mPopulatedPage = -1;

    //! For processing only my requests
    QString mLastRequestId;
};

#endif // PROJECTSMODEL_FUTURE_H
