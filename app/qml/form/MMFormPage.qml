/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import mm 1.0 as MM
import qgs 1.0 as QGS

import "../components" as MMComponents
import "./components" as MMFormComponents
import "../dialogs"


Page {
  id: root

  /**
   * When feature in the form is saved.
   */
  signal saved

  /**
   * When the form is about to be closed by closeButton or deleting a feature.
   */
  signal canceled

  /**
   * Signal emited when relation editor requests to open child feature form
   */
  signal openLinkedFeature( var linkedFeature )

  /**
   * Signal emited when relation editor requests to create child feature and open its form
   */
  signal createLinkedFeature( var parentController, var relation )

  signal editGeometryRequested( var layerPair )

  /**
   * Active project.
   */
  property QGS.Project project

  /**
   * Controller
   */
  property MM.AttributeController controller

  implicitWidth: ApplicationWindow.window?.width ?? 0
  implicitHeight: ApplicationWindow.window?.height ?? 0

  states: [
    State {
      name: "readOnly"
    },
    State {
      name: "edit"
    },
    State {
      name: "add"
    }
  ]

  Keys.onReleased: function( event ) {
    if ( event.key === Qt.Key_Back || event.key === Qt.Key_Escape ) {
      if ( root.controller.hasAnyChanges )  {
        saveChangesDialog.open()
      }
      else {
        root.rollbackAndClose()
      }
      event.accepted = true;
    }
  }

  property bool layerIsReadOnly: true
  property bool layerIsSpatial: true

  background: Rectangle {
    color: __style.lightGreenColor
  }

  header: MMComponents.MMPageHeader {

    onBackClicked: {
      if ( root.controller.hasAnyChanges )  {
        saveChangesDialog.open()
      }
      else {
        root.rollbackAndClose()
      }
    }


    title: {
      if ( root.state === "add" ) return qsTr( "New feature" )
      else if ( root.state === "edit" ) return qsTr( "Edit feature" )
      return __inputUtils.featureTitle( root.controller.featureLayerPair, __activeProject.qgsProject )
    }

    rightItemContent: MMComponents.MMRoundButton {

      anchors.verticalCenter: parent.verticalCenter

      visible: root.state === "add" || root.state === "edit"

      iconSource: __style.checkmarkIcon
      iconColor: controller.hasValidationErrors ? __style.grapeColor : __style.forestColor
      bgndColor: controller.hasValidationErrors ? __style.negativeColor : __style.grassColor

      onClicked: root.save()
    }
  }

  ColumnLayout {
    anchors.fill: parent

    MMFormComponents.MMFormTabBar {
      id: tabBar

      Layout.topMargin: __style.margin10
      Layout.bottomMargin: __style.margin10
      Layout.alignment: Qt.AlignHCenter
      Layout.fillWidth: true
      Layout.maximumWidth: Math.min(__style.maxPageWidth, root.width)

      model: root.controller.attributeTabProxyModel

      visible: root.controller.hasTabs

      onCurrentIndexChanged: formSwipe.setCurrentIndex( tabBar.currentIndex )
    }

    SwipeView {
      id: formSwipe

      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.alignment: Qt.AlignHCenter
      Layout.maximumWidth: __style.maxPageWidth

      clip: true

      onCurrentIndexChanged: tabBar.currentIndex = formSwipe.currentIndex

      Repeater {
        id: swipeViewRepeater

        model: root.controller.attributeTabProxyModel

        Item {
          id: pageDelegate

          property int tabIndex: model.TabIndex // from the repeater

          MMComponents.MMListView {

            anchors {
              fill: parent
              leftMargin: __style.pageMargins
              rightMargin: __style.pageMargins
            }

            model: swipeViewRepeater.model.attributeFormProxyModel( pageDelegate.tabIndex )

            clip: true
            spacing: internal.formSpacing

            header: Rectangle {
              opacity: 1 // invisible
              height: 20 * __dp
            }

            section {
              property: "Group"
              delegate: sectionDelegate
              labelPositioning: ViewSection.InlineLabels
            }

            delegate: editorDelegate

            footer: Rectangle {
              opacity: 1 // invisible
              height: 20 * __dp
            }
          }
        }
      }
    }
  }

  footer: MMComponents.MMToolbar {

    visible: !root.layerIsReadOnly && __activeProject.projectRole !== "reader"

    ObjectModel {
      id: readStateButtons

      MMComponents.MMToolbarButton {
        text: qsTr( "Edit feature" );

        iconSource: __style.editCircleIcon
        iconColor: __style.forestColor

        onClicked: root.state = "edit"
      }
    }

    ObjectModel {
      id: addStateButtons // edit buttons are the same

      MMComponents.MMToolbarButton {
        text: qsTr( "Delete" )
        iconSource: __style.deleteIcon
        iconColor: root.layerIsSpatial ? __style.polarColor : __style.grapeColor
        bgColor: root.layerIsSpatial ? __style.grassColor : __style.negativeColor
        onClicked: deleteDialog.open()
      }

      MMComponents.MMToolbarButton {
        id: editGeometry
        text: qsTr( "Edit geometry" )
        iconSource: __style.editIcon
        visible: root.layerIsSpatial && __activeProject.projectRole !== "reader"
        onClicked: root.editGeometryRequested( root.controller.featureLayerPair )
      }
    }


    model: root.state === "readOnly" ? readStateButtons : addStateButtons
  }

  Component {
    id: sectionDelegate

    Rectangle {

      height: section ? childrenRect.height : 0
      width: ListView.view.width

      color: __style.lightGreenColor

      MMComponents.MMText {
        text: section
        font: __style.h3
        color: __style.forestColor

        width: parent.width
        wrapMode: Text.Wrap
        elide: Text.ElideRight

        topPadding: internal.formSpacing
        bottomPadding: internal.formSpacing

        maximumLineCount: 3
      }
    }
  }

  Component {
    id: editorDelegate

    Item {

      width: ListView.view.width
      implicitHeight: childrenRect.height

      // In future, better to filter such fields in the field proxy model instead
      visible: Type !== MM.FormItem.Invalid && Type !== MM.FormItem.Container

      Loader {
        id: formEditorsLoader

        //
        // Maybe one day we could use DelegateChooser instead of this hack-ish approach, see:
        // https://doc.qt.io/qt-6/qml-qt-labs-qmlmodels-delegatechooser.html
        //

        width: parent.width

        property var fieldValue: model.RawValue
        property bool fieldValueIsNull: model.RawValueIsNull ?? true

        property var field: model.Field
        property var fieldIndex: model.FieldIndex
        property var fieldWidget: model.EditorWidget
        property var fieldConfig: model.EditorWidgetConfig

        property bool fieldFormIsReadOnly: root.state === "readOnly"
        property bool fieldIsEditable: AttributeEditable

        property bool fieldShouldShowTitle: model.ShowName

        property string fieldTitle: model.Name
        property string fieldErrorMessage: model.ValidationStatus === MM.FieldValidator.Error ? model.ValidationMessage : ""
        property string fieldWarningMessage: model.ValidationStatus === MM.FieldValidator.Warning ? model.ValidationMessage : ""

        property var fieldActiveProject: root.project
        property var fieldAssociatedRelation: model.Relation
        property var fieldFeatureLayerPair: root.controller.featureLayerPair
        property string fieldHomePath: root.project ? root.project.homePath : "" // for photo editor

        property bool fieldRememberValueSupported: root.controller.rememberAttributesController.rememberValuesAllowed && root.state === "add" && model.EditorWidget !== "Hidden" && Type === MM.FormItem.Field
        property bool fieldRememberValueState: model.RememberValue ? true : false

        active: fieldWidget !== 'Hidden'

        source: {
          if ( model.EditorWidget !== undefined ) {
            return __inputUtils.getFormEditorType( model.EditorWidget, model.EditorWidgetConfig, model.Field, model.Relation, model.Name )
          }

          return ''
        }
      }

      Connections {
        target: formEditorsLoader.item
        ignoreUnknownSignals: true

        function onEditorValueChanged( newVal, isNull ) {
          model.AttributeValue = isNull ? undefined : newVal
        }

        function onRememberValueBoxClicked( state ) {
          model.RememberValue = state
        }

        function onCreateLinkedFeature( parentFeature, relation ) {
          let parentHasValidId = __inputUtils.isFeatureIdValid( parentFeature.feature.id )

          if ( parentHasValidId ) {
            // parent feature in this case already have valid id, so we can open new form
            root.createLinkedFeature( root.controller, relation )
          }
          else {
            // parent feature does not have a valid ID yet, we need to save it and acquire ID
            root.controller.acquireId()
            root.createLinkedFeature( root.controller, relation )
          }
        }

        function onOpenLinkedFeature( linkedFeature ) {
          root.openLinkedFeature( linkedFeature )
        }
      }

      Connections {
        target: root.controller

        // Important for relation form editors // <--- TODO: remove me if all works, unused
        function onFeatureLayerPairChanged() {
          if ( formEditorsLoader.item && formEditorsLoader.item.featureLayerPairChanged )
          {
            formEditorsLoader.item.featureLayerPairChanged()
          }
        }

        // Important for value relation form editors
        function onFormRecalculated() {
          if ( formEditorsLoader.item && formEditorsLoader.item.reload )
          {
            formEditorsLoader.item.reload()
          }
        }
      }

      Connections {
        target: root
        ignoreUnknownSignals: true

        function onSaved() {
          if ( formEditorsLoader.item && typeof formEditorsLoader.item.callbackOnSave === "function" ) {
            formEditorsLoader.item.callbackOnFormSaved()
          }
        }

        function onCanceled() {
          if ( formEditorsLoader.item && typeof formEditorsLoader.item.callbackOnCancel === "function" ) {
            formEditorsLoader.item.callbackOnFormCanceled()
          }
        }
      }
    }
  }

  MMFormSaveChangesDialog {
    id: saveChangesDialog

    onSaveChanges: root.save()
    onDiscardChanges: root.rollbackAndClose()
  }

  MMFormDeleteFeatureDialog {
    id: deleteDialog

    onDeleteFeature: {
      root.controller.deleteFeature()
      root.canceled()
    }
  }

  MMFormEditFailedDialog {
    id: editingFailedDialog
  }

  Connections {
    target: Qt.inputMethod

    function onVisibleChanged() {
      Qt.inputMethod.commit()
    }
  }

  Connections {
    target: root.controller

    function onChangesCommited() {
      root.saved()
    }

    function onCommitFailed() {
      editingFailedDialog.open()
    }
  }

  function reset() {
    master.reset()
  }

  function save() {
    if ( controller.hasValidationErrors )
    {
      __notificationModel.addError( qsTr( 'Feature could not be saved, please check all required fields' ) )

      // In future we could navigate user to a field that contains validation error
      return
    }

    parent.focus = true
    controller.save()
  }

  function rollbackAndClose() {
    // remove feature if we are in "add" mode and it already has valid ID
    // it was saved to prefill relation reference field in child layer
    let featureId = root.controller.featureLayerPair.feature.id
    let shouldRemoveFeature = root.state === "add" && __inputUtils.isFeatureIdValid( featureId )

    if ( shouldRemoveFeature ) {
      root.controller.deleteFeature()
    }

    parent.focus = true

    // rollback all changes if the layer is still editable
    root.controller.rollback()

    root.canceled()
  }

  /**
   * This is a relay to forward private signals to internal components.
   */
  QtObject {
    id: master

    /**
     * This signal is emitted whenever the state of Flickables and TabBars should
     * be restored.
     */
    signal reset
  }

  QtObject {
    id: internal

    property real formSpacing: 20 * __dp
  }
}
