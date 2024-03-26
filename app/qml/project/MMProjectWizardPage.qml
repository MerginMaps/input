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

import mm 1.0 as MM

import "./components" as MMProjectComponents
import "../components" as MMComponents
import "../inputs" as MMInputs

MMComponents.MMPage {
  id: root

  pageHeader.title: qsTr("Create Project")

  pageContent: Item {

    width: parent.width
    height: parent.height

    Column {
      id: contentLayout

      width: parent.width
      height: parent.height

      spacing: 0

      MMComponents.MMListSpacer { height: __style.margin20 }

      MMInputs.MMTextInput {
        id: projectNameField

        title: qsTr("Project name")
        width: parent.width
      }

      MMComponents.MMListSpacer { height: __style.margin20 }

      MMComponents.MMText {
        id: attributesLabel

        height: root.rowheight
        width: parent.width
        text: qsTr("Fields")
        color: __style.nightColor
        font: __style.p6
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
      }

      ListView {
        id: fieldList

        model: fieldsModel
        width: parent.width
        height: parent.height - projectNameField.height - __style.margin20 - projectNameField.height
        clip: true
        spacing: __style.margin20

        delegate: MMProjectComponents.MMProjectWizardDelegate {
          id: fieldDelegate

          width: ListView.view.width

          // find current index in the model
          comboboxField.comboboxModel: typesmodel

          comboboxField.onCurrentIndexChanged: {
            console.log(comboboxField.currentIndex, typesmodel.get(comboboxField.currentIndex), typesmodel.get(comboboxField.currentIndex)?.type ?? "")
            WidgetType = typesmodel.get(comboboxField.currentIndex)?.type ?? ""
          }

          onAttrNameChanged: ( attrname ) => AttributeName = attrname
          onRemoveClicked: () => fieldsModel.removeField( index )

          Component.onCompleted: {
            // assign initial values without binding
            attrname = AttributeName
            comboboxField.currentIndex = root.indexFromWidgetType( WidgetType )
          }
        }

        footer: MMComponents.MMButton {
          id: addButton

          width: ListView.view.width
          height: root.rowHeight

          text: qsTr( "Add field" )

          type: MMComponents.MMButton.Tertiary

          iconSourceRight: __style.addIcon
          topPadding: __style.margin20

          onClicked: {
            fieldsModel.addField("", "TextEdit")

            if ( fieldList.visible ) {
              fieldList.positionViewAtEnd()
            }
          }
        }
      }
    }
  }

  footer: MMComponents.MMToolbar {
    id: toolbar

    model: ObjectModel {
      MMComponents.MMToolbarButton {
        text: qsTr("Create project");
        iconSource: __style.doneCircleIcon
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

  MM.FieldsModel {
    id: fieldsModel

    onNotifyError: function( message ) {
      __notificationModel.addError( message )
    }
  }

  ListModel {
    id: typesmodel

    ListElement { text: "Text"; type: "TextEdit" }
    ListElement { text: "Date&time"; type: "DateTime" }
    ListElement { text: "Number"; type: "Range" }
    ListElement { text: "Checkbox"; type: "CheckBox" }
    ListElement { text: "Photo"; type: "ExternalResource" }
  }

  function indexFromWidgetType( widgetType ) {
    for ( let i = 0; i < typesmodel.count; i++ ) {
      let item = typesmodel.get(i)
      if ( widgetType === item.type ) {
        return i
      }
    }
    return -1
  }
}
