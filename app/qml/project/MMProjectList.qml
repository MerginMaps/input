﻿/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import mm 1.0 as MM

import "./components"
import "../components"
import "../dialogs"

Item {
  id: root

  property int projectModelType: MM.ProjectsModel.EmptyProjectsModel
  property string activeProjectId: ""
  property string searchText: ""
  property int spacing: 0
  property bool hideActiveProject: false
  property alias projectsProxyModel: viewModel
  property alias projectsModel: controllerModel
  property alias listHeader: listview.header
  property alias listFooter: listview.footer

  signal openProjectRequested( string projectFilePath )
  signal showLocalChangesRequested( string projectId )
  signal activeProjectDeleted()

  onSearchTextChanged: {
    if ( projectModelType !== MM.ProjectsModel.LocalProjectsModel ) {
      controllerModel.listProjects( root.searchText )
    }
    else viewModel.searchExpression = root.searchText
  }

  function refreshProjectList() {
    controllerModel.listProjects( searchText )
  }

  ListView {
    id: listview

    Component.onCompleted: {
      // set proper footer (add project / fetch more)
      if ( root.projectModelType === MM.ProjectsModel.LocalProjectsModel ) {
        listview.footer = addProjectComponent
      }
      else
      {
        listview.footer = loadingSpinnerComponent
      }
    }

    onAtYEndChanged: {
      if ( atYEnd ) { // user reached end of the list
        if ( controllerModel.hasMoreProjects && !controllerModel.isLoading ) {
          controllerModel.fetchAnotherPage( viewModel.searchExpression )
        }
      }
    }

    anchors.fill: parent
    clip: true
    spacing: root.spacing

    maximumFlickVelocity: __androidUtils.isAndroid ? __style.scrollVelocityAndroid : maximumFlickVelocity

    // Proxy model with source projects model
    model: MM.ProjectsProxyModel {
      id: viewModel

      projectSourceModel: MM.ProjectsModel {
        id: controllerModel

        merginApi: __merginApi
        localProjectsManager: __localProjectsManager
        syncManager: __syncManager
        modelType: root.projectModelType
      }
    }

    // Project delegate
    delegate: MMProjectDelegate {
      id: projectDelegate

      width: ListView.view.width
      height: visible ? implicitHeight : 0

      visible: root.hideActiveProject ? !projectIsOpened : true

      projectDisplayName: root.projectModelType === MM.ProjectsModel.CreatedProjectsModel ? model.ProjectName : model.ProjectFullName
      projectId: model.ProjectId ? model.ProjectId : ""
      projectDescription: model.ProjectDescription ? model.ProjectDescription : ""
      projectIsInSync: model.ProjectSyncPending ? model.ProjectSyncPending : false
      projectSyncProgress: model.ProjectSyncProgress ? model.ProjectSyncProgress : -1
      projectIsOpened: model.ProjectId === root.activeProjectId

      state: {
        let status = model.ProjectStatus ? model.ProjectStatus : MM.ProjectStatus.NoVersion

        if ( status === MM.ProjectStatus.NeedsSync ) {
          return "NeedsSync"
        }
        else if ( model.ProjectIsMergin && model.ProjectIsLocal )
        {
          return "UpToDate"
        }
        else if ( model.ProjectIsMergin && !model.ProjectIsLocal )
        {
          return "OnServer"
        }
        else if ( !model.ProjectIsMergin && !model.ProjectIsLocal )
        {
          return "NeedsSync" // TODO: what to do here? Locally created project!
        }
        else if ( !model.ProjectIsValid )
        {
          return "Error"
        }

        return "UpToDate" // fallback, should never happen
      }

      projectActionButtons: {
        if ( model.ProjectIsMergin && model.ProjectIsLocal )
        {
          if ( ( ( model.ProjectStatus ? model.ProjectStatus : MM.ProjectStatus.NoVersion ) === MM.ProjectStatus.NeedsSync ) ) {
            return ["sync", "changes", "remove"]
          }
          return ["changes", "remove"]
        }
        else if ( !model.ProjectIsMergin && model.ProjectIsLocal ) {
          return ["upload", "remove"]
        }
        return ["download"]
      }

      onOpenRequested: {
        if ( model.ProjectIsLocal )
          root.openProjectRequested( model.ProjectFilePath )
        else if ( !model.ProjectIsLocal && model.ProjectIsMergin && !model.ProjectSyncPending) {
          downloadProjectDialog.relatedProjectId = model.ProjectId
          downloadProjectDialog.open()
        }
      }

      onSyncRequested: {
        if ( model.ProjectRemoteError ) {
          __notificationModel.addError( qsTr( "Could not synchronize project, please make sure you are logged in and have sufficient rights." ) )
        }
        else if ( !model.ProjectIsMergin ) {
          controllerModel.migrateProject( projectId )
        }
        else {
          controllerModel.syncProject( projectId )
        }
      }

      onMigrateRequested: controllerModel.migrateProject( projectId )
      onRemoveRequested: {
        removeDialog.relatedProjectId = projectId
        removeDialog.open()
      }
      onStopSyncRequested: controllerModel.stopProjectSync( projectId )
      onShowChangesRequested: root.showLocalChangesRequested( projectId )
    }
  }

  Component {
    id: loadingSpinnerComponent

    MMBusyIndicator {
      x: parent.width / 2 - width / 2
      running: controllerModel.isLoading
    }
  }

  Component {
    id: addProjectComponent

    Column {

      topPadding: noLocalProjectsMessageContainer.visible ? noLocalProjectsMessageContainer.height + __style.margin40 : 0

      width: ListView.view.width

      Item {
        width: parent.width
        height: root.spacing
      }

      MMButton {
        width: parent.width
        text: qsTr("Create project")
        onClicked: stackView.push(projectWizardComp)
      }

      Item {
        width: parent.width
        height: root.spacing
      }
    }
  }

  MMMessage {
    id: noLocalProjectsMessageContainer

    visible: listview.count === 0 && // this check is getting longer and longer, would be good to replace with states
             projectModelType === MM.ProjectsModel.LocalProjectsModel &&
             root.searchText === "" &&
             !controllerModel.isLoading

    anchors {
      left: parent.left
      right: parent.right
    }

    image: __style.positiveMMSymbolImage
    title: qsTr( "No downloaded projects found")
    description: "<style>a:link { color: " + __style.forestColor + "; }</style>" +
                 qsTr( "Learn %1how to create projects%2 and %3download them%2 onto your device. You can also create new project by clicking button below." )
    .arg("<a href='"+ __inputHelp.howToCreateNewProjectLink +"'>")
    .arg("</a>")
    .arg("<a href='"+ __inputHelp.howToDownloadProjectLink +"'>")
  }

  MMMessage {
    id: noMerginProjectsTexts

    anchors.centerIn: parent
    visible: reloadList.visible || !controllerModel.isLoading && ( projectModelType !== MM.ProjectsModel.LocalProjectsModel && listview.count === 0 )
    title: reloadList.visible ? qsTr("Unable to get the list of projects.") : qsTr("No projects found")
    image: reloadList.visible ? __style.noWifiImage : __style.negativeMMSymbolImage
  }

  Item {
    id: reloadList

    width: parent.width
    height: __style.row63
    visible: false
    y: root.height/3 * 2

    Connections {
      target: __merginApi

      function onListProjectsFailed() {
        reloadList.visible = root.projectModelType !== MM.ProjectsModel.LocalProjectsModel // show reload list to all models except local
      }

      function onListProjectsFinished( merginProjects, projectCount, page, requestId ) {
        if ( projectCount > -1 )
          reloadList.visible = false
      }
    }

    MMButton {
      id: reloadBtn

      width: reloadList.width - 2* __style.pageMargins
      height: reloadList.height
      text: qsTr("Retry")

      anchors.horizontalCenter: parent.horizontalCenter
      onClicked: {
        // filters suppose to not change
        controllerModel.listProjects( root.searchText )
      }
    }
  }

  MMRemoveProjectDialog {
    id: removeDialog

    onRemoveClicked: {
      if (relatedProjectId === "") {
        return
      }

      if ( root.activeProjectId === relatedProjectId )
        root.activeProjectDeleted()

      __inputUtils.log(
            "Delete project",
            "Project " + __localProjectsManager.projectName( relatedProjectId ) + " deleted by " +
            ( __merginApi.userAuth ? __merginApi.userAuth.username : "unknown" ) + " (" + __localProjectsManager.projectChanges( relatedProjectId ) + ")" )

      controllerModel.removeLocalProject( relatedProjectId )

      removeDialog.relatedProjectId = ""
    }
  }

  MMDownloadProjectDialog {
    id: downloadProjectDialog

    onDownloadClicked: {
      controllerModel.syncProject( relatedProjectId )
      downloadProjectDialog.relatedProjectId = ""
    }
  }
}
