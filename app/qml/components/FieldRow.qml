import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import QtQuick.Dialogs

import lc 1.0
import "./.." // import InputStyle singleton

Item {
  id: fieldDelegate
  signal removeClicked(var index)

  property real rowHeight: InputStyle.rowHeightHeader
  property real iconSize: rowHeight * 0.4
  property color color: InputStyle.fontColor
  property var widgetList: []

  RowLayout {
    id: row
    height: fieldDelegate.rowHeight
    width: fieldDelegate.width
    spacing: InputStyle.panelSpacing
    property real itemSize: (parent.width - imageBtn.width - (2* row.spacing)) / 2

    InputTextField {
      id: textField
      color: fieldDelegate.color
      Layout.fillHeight: true
      Layout.fillWidth: true
      Layout.preferredWidth: row.itemSize

      Component.onCompleted: text = AttributeName
      onTextChanged: function(text) {
        AttributeName = text
      }
    }

    ComboBox {
      id: comboBox
      height: row.height
      Layout.fillHeight: true
      Layout.fillWidth: true
      Layout.preferredWidth: row.itemSize
      model: widgetList
      textRole: "display"
      valueRole: "widget"

      Component.onCompleted: {
        comboBox.currentIndex = comboBox.indexOfValue(WidgetType);
      }

      MouseArea {
        anchors.fill: parent
        propagateComposedEvents: true

        onClicked: function() {
          mouse.accepted = false
        }
        onPressed: function() {
          forceActiveFocus(); mouse.accepted = false;
        }
        onReleased: function() {
          mouse.accepted = false;
        }
        onDoubleClicked: function() {
          mouse.accepted = false;
        }
        onPositionChanged: function() {
          mouse.accepted = false;
        }
        onPressAndHold: function() {
          mouse.accepted = false;
        }
      }

      delegate: ItemDelegate {
        width: comboBox.width
        height: comboBox.height * 0.8
        text: model.display.replace('&', "&&") // issue ampersand character showing up as underscore
        font.weight: comboBox.currentIndex === index ? Font.DemiBold : Font.Normal
        font.pixelSize: InputStyle.fontPixelSizeNormal
        highlighted: comboBox.highlightedIndex === index
        leftPadding: textField.leftPadding
        onClicked: function() {
          WidgetType = model.widget
          comboBox.currentIndex = index
        }
      }

      contentItem: Text {
        height: comboBox.height * 0.8
        text: comboBox.displayText
        font.pixelSize: InputStyle.fontPixelSizeNormal
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        leftPadding: textField.leftPadding
        color: InputStyle.fontColor
      }

      background: Item {
        implicitHeight: comboBox.height * 0.8

        Rectangle {
          anchors.fill: parent
          id: backgroundRect
          border.color: comboBox.pressed ? InputStyle.fontColor : InputStyle.panelBackgroundLight
          border.width: comboBox.visualFocus ? 2 : 1
          color: InputStyle.panelBackgroundLight
          radius: InputStyle.cornerRadius
        }
      }

      indicator: Item {
        height: parent.height
        anchors.right: parent.right

        Image {
          id: comboboxIndicatorIcon
          source: InputStyle.comboboxIcon
          height: fieldDelegate.iconSize
          width: height / 2
          anchors.right: parent.right
          anchors.rightMargin: InputStyle.innerFieldMargin
          anchors.verticalCenter: parent.verticalCenter
          fillMode: Image.PreserveAspectFit
          autoTransform: true
          visible: false
        }

        ColorOverlay {
          anchors.fill: comboboxIndicatorIcon
          source: comboboxIndicatorIcon
          color: InputStyle.activeButtonColor
        }
      }
    }

    Symbol {
      id: imageBtn
      height: fieldDelegate.height/2
      Layout.fillHeight: true
      source: InputStyle.noIcon
      iconSize: fieldDelegate.iconSize

      MouseArea {
        anchors.fill: parent
        onClicked: function() {
          fieldDelegate.removeClicked(index)
        }
      }
    }
  }
}
