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
import QtQuick.Dialogs

import lc 1.0
import ".."
import "../components"

Item {
  id: root

  property var project
  property var featureLayerPair

  property var linkedRelation
  property var parentController

  property string formState

  signal close()
  signal editGeometryClicked( var pair )
  signal splitGeometryClicked()
  signal redrawGeometryClicked( var pair )
  signal openLinkedFeature( var linkedFeature )
  signal createLinkedFeature( var parentController, var relation )

  StackView {
    id: formStackView

    /**
     * StackView handling navigation in one FeatureForm
     * Initial page is the form itself and any other extra
     * needed pages (like value relation page, relations page, ..)
     * should be pushed to this view.
     *
     * View is attached to Feature Form,
     * so editors can push their components to it
     */

    anchors.fill: parent

    initialItem: formPageComponent
    focus: true

    onCurrentItemChanged: {
      currentItem.forceActiveFocus()
    }
  }

  Component {
    id: formPageComponent

    Page {
      id: formPage

      property alias form: featureForm

      header: PanelHeader {
        id: header


        height: InputStyle.rowHeightHeader
        rowHeight: InputStyle.rowHeightHeader
        color: InputStyle.clrPanelMain
        fontBtnColor: InputStyle.highlightColor

        titleText: featureForm.state === "edit" ? qsTr("Edit Feature") : qsTr("Feature")

        backIconVisible: !saveButtonText.visible
        backTextVisible: saveButtonText.visible

        onBack: featureForm.cancel()

        Text {
          id: saveButtonText

          text: qsTr("Save")

          height: header.rowHeight
          visible: featureForm.state === "edit" || featureForm.state === "add"

          color: featureForm.controller.hasValidationErrors ? InputStyle.invalidButtonColor : InputStyle.highlightColor
          font.pixelSize: InputStyle.fontPixelSizeNormal

          verticalAlignment: Text.AlignVCenter
          horizontalAlignment: Text.AlignLeft

          anchors.right: parent.right
          anchors.bottom: parent.bottom
          anchors.top: parent.top
          anchors.rightMargin: InputStyle.panelMargin // same as back button

          MouseArea {
            anchors.fill: parent
            onClicked: featureForm.save()
          }
        }
      }

      Item {
        id: backHandler
        focus: true
        Keys.onReleased: function( event ) {
          if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
            if ( featureForm.controller.hasAnyChanges )  {
              saveChangesDialog.open()
            }
            else {
              featureForm.cancel()
            }
            event.accepted = true;
          }
        }

        onVisibleChanged: function( visible ) {
          if ( visible )
            backHandler.forceActiveFocus()
        }
      }

      // content
      FeatureForm {
        id: featureForm

        anchors.fill: parent

        project: root.project

        controller: AttributeController {
          /*required*/ variablesManager: __variablesManager
          rememberAttributesController: RememberAttributesController {
            rememberValuesAllowed: __appSettings.reuseLastEnteredValues
          }
          // NOTE: order matters, we want to init variables manager before
          // assingning FeatureLayerPair, as VariablesManager required for
          // correct expression evaluation
          /*required*/ featureLayerPair: root.featureLayerPair
        }

        importDataHandler: codeReaderHandler.handler
        externalResourceHandler: externalResourceBundle.handler
        state: root.formState

        onSaved: root.close()
        onCanceled: root.close()
        onEditingFailed: editingFailedDialog.open()
        onOpenLinkedFeature: root.openLinkedFeature( linkedFeature )
        onCreateLinkedFeature: root.createLinkedFeature( parentController, relation )

        extraView: formPage.StackView.view

        Connections {
          target: root
          function onFormStateChanged() {
            featureForm.state = root.formState
          }
        }

        Component.onCompleted: {
          if ( root.parentController && root.linkedRelation ) {
            featureForm.controller.parentController = root.parentController
            featureForm.controller.linkedRelation = root.linkedRelation
          }
        }
      }

      footer: FeatureToolbar {
        id: toolbar

        height: InputStyle.rowHeightHeader

        state: featureForm.state

        visible: !root.readOnly
        isFeaturePoint: __inputUtils.isPointLayer( root.featureLayerPair.layer )
        isSpatialLayer: __inputUtils.isSpatialLayer( root.featureLayerPair.layer )

        onEditClicked: root.formState = "edit"
        onDeleteClicked: deleteDialog.visible = true
        onEditGeometryClicked: root.editGeometryClicked( featureForm.controller.featureLayerPair )
        onSplitGeometryClicked: root.splitGeometryClicked()
        onRedrawGeometryClicked: root.redrawGeometryClicked( featureForm.controller.featureLayerPair )
      }

      MessageDialog {
        id: deleteDialog

        visible: false
        title: qsTr( "Delete feature" )
        text: qsTr( "Are you sure you want to delete this feature?" )
        buttons: MessageDialog.Ok | MessageDialog.Cancel

        //! Using onButtonClicked instead of onAccepted,onRejected which have been called twice
        onButtonClicked: {
          if ( clickedButton === MessageDialog.Ok ) {
            featureForm.controller.deleteFeature()
            featureForm.canceled()
            root.close()
          }

          visible = false
        }
      }

      MessageDialog {
        id: saveChangesDialog

        visible: false
        title: qsTr( "Unsaved changes" )
        text: qsTr( "Do you want to save changes?" )
        buttons: MessageDialog.Yes | MessageDialog.No | MessageDialog.Cancel

        //! Using onButtonClicked instead of onAccepted,onRejected which have been called twice
        onButtonClicked: {
          if (clickedButton === MessageDialog.Yes) {
            featureForm.save()
          }
          else if (clickedButton === MessageDialog.No) {
            featureForm.canceled()
          }
          else if (clickedButton === MessageDialog.Cancel) {
            // Do nothing
          }
          visible = false
        }
      }

      MessageDialog {
        id: editingFailedDialog

        visible: false
        title: qsTr( "Saving failed" )
        text: qsTr( "Failed to save changes. This should not happen normally. Please restart the app and try again — if that does not help, please contact support." )
        buttons: MessageDialog.Close

        //! Using onButtonClicked instead of onAccepted,onRejected which have been called twice
        onButtonClicked: {
          visible = false
        }
      }

      ExternalResourceBundle {
        id: externalResourceBundle
      }

      CodeReaderHandler {
        id: codeReaderHandler
      }
    }
  }
}
