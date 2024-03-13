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

Button {
  id: root

  enum Types { Primary, Secondary, Tertiary }

  property int type: MMButton.Types.Primary

  property color fontColor: {
    if ( type === MMButton.Types.Primary ) return __style.forestColor
    if ( type === MMButton.Types.Secondary ) return __style.forestColor
    if ( type === MMButton.Types.Tertiary ) return __style.forestColor
  }

  property color iconColor: {
    if ( type === MMButton.Types.Primary ) return __style.forestColor
    if ( type === MMButton.Types.Secondary ) return __style.forestColor
    if ( type === MMButton.Types.Tertiary ) return __style.forestColor
  }

  property color bgndColor: {
    if ( type === MMButton.Types.Primary ) return __style.grassColor
    if ( type === MMButton.Types.Secondary ) return __style.forestColor
    if ( type === MMButton.Types.Tertiary ) return __style.transparentColor
  }

  property color fontColorHover: {
    if ( type === MMButton.Types.Primary ) return __style.grassColor
    if ( type === MMButton.Types.Secondary ) return __style.forestColor
    if ( type === MMButton.Types.Tertiary ) return __style.nightColor
  }

  property color iconColorHover: {
    if ( type === MMButton.Types.Primary ) return __style.grassColor
    if ( type === MMButton.Types.Secondary ) return __style.forestColor
    if ( type === MMButton.Types.Tertiary ) return __style.nightColor
  }

  property color bgndColorHover: {
    if ( type === MMButton.Types.Primary ) return __style.forestColor
    if ( type === MMButton.Types.Secondary ) return __style.grassColor
    if ( type === MMButton.Types.Tertiary ) return __style.transparentColor
  }

  property color fontColorDisabled: { // TODO: change with the new colors
    if ( type === MMButton.Types.Primary ) return __style.greyColor
    if ( type === MMButton.Types.Secondary ) return __style.lightGreenColor
    if ( type === MMButton.Types.Tertiary ) return __style.lightGreenColor
  }

  property color iconColorDisabled: { // TODO: change with the new colors
    if ( type === MMButton.Types.Primary ) return __style.greyColor
    if ( type === MMButton.Types.Secondary ) return __style.lightGreenColor
    if ( type === MMButton.Types.Tertiary ) return __style.lightGreenColor
  }

  property color bgndColorDisabled: { // TODO: change with the new colors
    if ( type === MMButton.Types.Primary ) return __style.lightGreenColor
    if ( type === MMButton.Types.Secondary ) return __style.lightGreenColor
    if ( type === MMButton.Types.Tertiary ) return __style.transparentColor
  }

  property bool disabled: false

  property string iconSourceRight
  property string iconSourceLeft

  states: [
    State {
      name: "default"
      when: !root.hovered && !root.disabled

      PropertyChanges {
        target: buttonContent
        color: root.fontColor
      }

      PropertyChanges {
        target: buttonBackground

        color: type === MMButton.Types.Secondary ? __style.transparentColor : root.bgndColor
        border.color: root.bgndColor
      }

      PropertyChanges {
        target: buttonIconRight
        color: root.iconColor
      }

      PropertyChanges {
        target: buttonIconLeft
        color: root.iconColor
      }
    },

    State {
      name: "hovered"
      when: root.hovered && !root.disabled

      PropertyChanges {
        target: buttonContent
        color: root.fontColorHover
      }

      PropertyChanges {
        target: buttonBackground

        color: root.bgndColorHover
        border.color: root.bgndColorHover
      }

      PropertyChanges {
        target: buttonIconRight
        color: root.iconColorHover
      }

      PropertyChanges {
        target: buttonIconLeft
        color: root.iconColorHover
      }
    },

    State {
      name: "disabled"
      when: root.disabled

      PropertyChanges {
        target: buttonContent
        color: root.fontColorDisabled
      }

      PropertyChanges {
        target: buttonBackground

        color: type === MMButton.Types.Secondary ? __style.transparentColor : root.bgndColorDisabled
        border.color: root.bgndColorDisabled
      }

      PropertyChanges {
        target: buttonIconRight
        color: root.iconColorDisabled
      }

      PropertyChanges {
        target: buttonIconLeft
        color: root.iconColorDisabled
      }
    }
  ]

  state: "default"
  enabled: !disabled

  implicitHeight: root.type === MMButton.Types.Tertiary ? buttonContent.height : buttonContent.height + topPadding + bottomPadding
  implicitWidth: row.paintedChildrenWidth + 2 * __style.margin32

  topPadding: root.type === MMButton.Types.Tertiary ? 0 : 11 * __dp
  bottomPadding: root.type === MMButton.Types.Tertiary ? 0 : 11 * __dp
  rightPadding: 0
  leftPadding: 0

  contentItem: Item {

    width: parent.width
    height: row.height

    Row {
      id: row

      property real paintedChildrenWidth: buttonIconLeft.paintedWidth + buttonContent.implicitWidth + buttonIconRight.paintedWidth + spacing
      property real maxWidth: parent.width - 2 * __style.margin32

      x: ( parent.width - width ) / 2

      width: Math.min( paintedChildrenWidth, maxWidth )
      height: Math.max( buttonContent.paintedHeight, buttonIconRight.height )

      spacing: buttonIconRight.visible || buttonIconLeft.visible ? __style.spacing12 : 0

      MMIcon {
        id: buttonIconLeft

        property real paintedWidth: visible ? ( width + parent.spacing ) : 0

        source: root.iconSourceLeft

        visible: root.iconSourceLeft
        width: visible ? __style.icon24 : 0
      }

      MMText {
        id: buttonContent

        width: parent.width - buttonIconLeft.paintedWidth - buttonIconRight.paintedWidth

        font: __style.t3
        text: root.text
      }

      MMIcon {
        id: buttonIconRight

        property real paintedWidth: visible ? ( width + parent.spacing ) : 0

        source: root.iconSourceRight

        visible: root.iconSourceRight
        width: visible ? __style.icon24 : 0
      }
    }

    // helper area to make the tertiary button easily clickable without enlarging its vertical size
    MouseArea {
      anchors.fill: parent
      anchors.topMargin: -__style.margin12
      anchors.bottomMargin: -__style.margin12

      enabled: root.type === MMButton.Types.Tertiary

      onClicked: function( mouse ) {
        mouse.accepted = true
        root.clicked()
      }
    }
  }

  background: Rectangle {
    id: buttonBackground

    radius: __style.radius30

    border.width: 2 * __dp
  }
}
