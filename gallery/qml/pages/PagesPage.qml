/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import "../../app/qml/components"
import "../../app/qml"
import "../../app/qml/project"
import "../../app/qml/settings"
import "../../app/qml/gps"

Page {
  id: root

  StackView {
    id: stackview

    anchors.fill: parent

    initialItem: Page {
      id: pane

      Column {
        width: parent.width
        spacing: 10
        padding: 10

        Label {
          text: "Pages"
        }

        MMButton {
          text: "MMProjectLoadingPage"
          onClicked: {
            stackview.push(loadingPageComponent)
          }
        }

        MMButton {
          text: "MMLogPanel"
          onClicked: {
            stackview.push(logPanelComponent)
          }
        }

        MMButton {
          text: "MMAboutPanel"
          onClicked: {
            stackview.push(aboutPanelComponent)
          }
        }

        MMButton {
          text: "MMChangelogPanel"
          onClicked: {
            stackview.push(changelogPanelComponent)
          }
        }

        MMButton {
          text: "MMSettingsPanel"
          onClicked: {
            stackview.push(settingsPanelComponent)
          }
        }

        MMButton {
          text: "MMProjectIssuesPage"
          onClicked: {
            stackview.push(projectIssuesPageComponent)
          }
        }
      }
    }
  }

  Component {
    id: loadingPageComponent

    MMProjectLoadingPage {
      id: loadingScreen

      width: root.width
      height: root.height

      MouseArea {
        width: parent.width
        height: parent.height

        onClicked: {
          stackview.pop()
        }
      }
    }
  }

  Component {
    id: projectIssuesPageComponent

    MMProjectIssuesPage {

      id: projectIssuesPage

      width: root.width
      height: root.height

      projectIssuesModel: ListModel {
          ListElement { title: "Apple" ; message: "2.45" }
          ListElement { title: "ipsum" ; message: "Lorem ipsum dolor sit amet, consectetur adipiscing elit" }
          ListElement { title: "Lorem" ; message: "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exerci" }
          ListElement { title: "sit" ; message: "dunt ut labore et dolore magna aliqua." }
          ListElement { title: "amet" ; message: "adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo" }
          ListElement { title: "consectetur adipiscing" ; message: "amet" }
          ListElement { title: "Apple" ; message: "2.45" }
          ListElement { title: "ipsum" ; message: "Lorem ipsum dolor sit amet, consectetur adipiscing elit" }
          ListElement { title: "Lorem" ; message: "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exerci" }
          ListElement { title: "sit" ; message: "dunt ut labore et dolore magna aliqua." }
          ListElement { title: "amet" ; message: "adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo" }
          ListElement { title: "consectetur adipiscing" ; message: "amet" }
      }

      projectLoadingLog: "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."

      MouseArea {
        width: parent.width
        height: parent.height

        onClicked: {
          stackview.pop()
        }
      }
    }
  }

  Component {
    id: logPanelComponent

    MMLogPanel {
      id: logPanel
      width: root.width
      height: root.height
      submitReportPending: false
      text: __logText
      onSubmitReport: submitReportPending = !submitReportPending
      onClose: stackview.pop()
    }
  }

  Component {
    id: aboutPanelComponent
    MMAboutPanel {
      id: aboutPanel
      width: root.width
      height: root.height
      onVisitWebsiteClicked: Qt.openUrlExternally( "https://merginmaps.com" )
      onClose: stackview.pop()
    }
  }

  Component {
    id: changelogPanelComponent
    MMChangelogPanel {
      id: changelogPanel
      width: root.width
      height: root.height
      onClose: stackview.pop()

      property date today: new Date()

      model: ListModel {
        ListElement {
          date: "Mon, 21 August 2023"
          description: "I am pleased to announce that position tracking has been released today as part of Mobile version 2.3.0 and Plugin version 2023.3"
          title: "Position tracking is now available"
          link: "https://wishlist.merginmaps.com/changelog"
        }

        ListElement {
          date: "Mon, 21 August 2023"
          description: "Mergin Maps QGIS plugin is now capable of setting a custom QGIS."
          title: "Ability to set custom name for photos taken in Mergin Maps"
          link: "https://wishlist.merginmaps.com/changelog"
        }

        function seeChangelogs() {
          console.log("see changelogs requested")
        }
      }
    }
  }

  Component {
    id: settingsPanelComponent
    MMSettingsPanel {
      id: settingsPanel
      width: root.width
      height: root.height
      onClose: stackview.pop()
      onManageGpsClicked: console.log("onManageGpsClicked clicked")
      onAboutClicked: console.log("onAboutClicked clicked")
      onChangelogClicked: console.log("onChangelogClicked clicked")
      onHelpClicked: console.log("onHelpClicked clicked")
      onPrivacyPolicyClicked: console.log("onPrivacyPolicyClicked clicked")
      onTermsOfServiceClicked: console.log("onTermsOfServiceClicked clicked")
      onDiagnosticLogClicked: console.log("onDiagnosticLogClicked clicked")
    }
  }
}
