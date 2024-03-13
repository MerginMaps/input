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
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import QtQuick.Dialogs

import mm 1.0 as MM

import "./components"
import "../components"
import "../inputs"

Item {
  id: projectWizardPanel

  signal backClicked

  property real rowHeight: 50 * __dp

  property ListModel widgetsModel: ListModel {}

  //! Inits widgetsModel data just after its created, but before Component.complete is emitted (for both model or components where its used)
  property bool isWidgetModelReady: {
    var types = fieldsModel.supportedTypes()
    for (var prop in types) {
      projectWizardPanel.widgetsModel.append({ "AttributeName": types[prop], "WidgetType": prop })
    }

    true
  }

  MM.FieldsModel {
    id: fieldsModel
    onNotifyError: function( message ) {
      __notificationModel.addError( message )
    }
    Component.onCompleted: fieldsModel.initModel()
  }

  // background
  Rectangle {
    width: parent.width
    height: parent.height
    color: __style.lightGreenColor
  }

  MMPageHeader {
    id: header
    width: projectWizardPanel.width
    color: __style.lightGreenColor
    title: qsTr("Create Project")

    onBackClicked: {
      projectWizardPanel.backClicked()
    }
  }

  Item {
    height: projectWizardPanel.height - header.height - toolbar.height
    width: projectWizardPanel.width
    y: header.height

    ColumnLayout {
      id: contentLayout
      spacing: 10 * __dp
      anchors.fill: parent
      anchors.leftMargin: __style.pageMargins
      anchors.rightMargin: __style.pageMargins

      MMTextInput {
        id: projectNameField
        title: qsTr("Project name")
        height: projectWizardPanel.rowheight
        width: parent.width
        Layout.fillWidth: true
        Layout.preferredHeight: projectWizardPanel.rowHeight
      }

      Label {
        id: attributesLabel
        height: projectWizardPanel.rowheight
        width: parent.width
        text: qsTr("Fields")
        color: __style.nightColor
        font: __style.p6
        Layout.preferredHeight: projectWizardPanel.rowHeight
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
      }

      ListView {
        id: fieldList
        model: fieldsModel
        width: parent.width
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 10 * __dp

        delegate: MMProjectWizardDelegate {
          height: projectWizardPanel.rowHeight
          width: contentLayout.width
          widgetList: projectWizardPanel.widgetsModel
          onRemoveClicked: function( index ) {
            fieldsModel.removeField(index)
          }
        }

        footer: MMButton {
              text: qsTr( "Add field" )

              type: MMButton.Types.Tertiary

              iconSource: __style.addIcon

              onClicked: {
                fieldsModel.addField("", "TextEdit")
                if (fieldList.visible) {
                  fieldList.positionViewAtEnd()
                }
              }
        }
      }
    }
  }

  // footer toolbar
  MMToolbar {
    id: toolbar
    anchors.bottom: parent.bottom

    model: ObjectModel {
      MMToolbarLongButton {
        text: qsTr("Create project");
        iconSource: __style.doneCircleIcon;
        iconColor: toolbar.color
        onClicked: {
          if (!projectNameField.text) {
            __notificationModel.addWarning( qsTr("Empty project name") )
          } else {
            __projectWizard.createProject(projectNameField.text, fieldsModel )
          }
        }
      }
    }
  }
}
