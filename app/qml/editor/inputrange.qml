/***************************************************************************
 range.qml
  --------------------------------------
  Date                 : 2019
  Copyright            : (C) 2019 by Viktor Sklencar
  Email                : viktor.sklencar@lutraconsulting.co.uk
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QgsQuick 0.1 as QgsQuick

Item {
  signal valueChanged(var value, bool isNull)

  property string widgetStyle: config["Style"] ? config["Style"] : "SpinBox"
  property int precision: config["Precision"]
  property real from: config["Min"]
  property real to: config["Max"]
  property real step: config["Step"] ? config["Step"] : 1
  property var locale: Qt.locale()
  property string suffix: config["Suffix"] ? config["Suffix"] : ""

  id: fieldItem
  enabled: !readOnly
  height: customStyle.fields.height

  anchors {
    left: parent.left
    right: parent.right
  }

  // background
  Rectangle {
    anchors.fill: parent
    border.color: customStyle.fields.normalColor
    border.width: 1 * QgsQuick.Utils.dp
    color: customStyle.fields.backgroundColor
    radius: customStyle.fields.cornerRadius
  }

  Row {
    id: rowLayout
    anchors.fill: parent

    // SpinBox
    SpinBox {
      property int multiplier: fieldItem.precision === 0 ? 1 : Math.pow(10, spinbox.precision)
      property int precision: fieldItem.precision
      property int intValue: fieldItem.parent.value * multiplier

      id: spinbox
      locale: fieldItem.locale
      from: fieldItem.from
      value: intValue
      to: fieldItem.to * multiplier
      stepSize: fieldItem.step * multiplier
      width: parent.width
      height: parent.height
      editable: true
      visible: fieldItem.widgetStyle === "SpinBox"

      background: Rectangle {
        anchors.fill: parent
        border.color: customStyle.fields.normalColor
        border.width: 1 * QgsQuick.Utils.dp
        color: customStyle.fields.backgroundColor
        radius: customStyle.fields.cornerRadius
      }

      onValueChanged: {
        if (visible) {
          fieldItem.valueChanged(spinbox.value / multiplier, false)
        }
      }

      validator: DoubleValidator {
        bottom: Math.min(spinbox.from, spinbox.to)
        top:  Math.max(spinbox.from, spinbox.to)
        decimals: spinbox.precision
        locale: spinbox.locale.name
      }

      textFromValue: function(value, locale) {
        try {
          return Number(value / multiplier).toLocaleString(spinbox.locale, 'f', spinbox.precision) + fieldItem.suffix
        } catch(error) {
          return spinbox.textFromValue(spinbox.value, spinbox.locale)
        }
      }

      valueFromText: function(text, locale) {
        if (fieldItem.suffix) {
          text = text.replace(suffix, "")
        }
        try {
          return Number.fromLocaleString(spinbox.locale, text) * multiplier
        } catch (error) {
          return spinbox.value
        }
      }

      contentItem: TextInput {
        z: 2
        text: spinbox.textFromValue(spinbox.value, spinbox.locale)
        font.pointSize: customStyle.fields.fontPointSize
        color: customStyle.fields.fontColor
        selectionColor: customStyle.fields.fontColor
        selectedTextColor: "#ffffff"
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !spinbox.editable
        validator: spinbox.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
      }

      down.indicator: Rectangle {
              x: spinbox.mirrored ? parent.width - width: 0
              height: parent.height
              implicitWidth: parent.height
              implicitHeight: parent.height
              color: customStyle.fields.backgroundColor
              radius: customStyle.fields.cornerRadius

              Text {
                  text: "-"
                  height: parent.height
                  font.pixelSize: spinbox.font.pixelSize * 2
                  font.bold: true
                  fontSizeMode: Text.Fit
                  color: fieldItem.enabled ? customStyle.fields.fontColor : customStyle.toolbutton.backgroundColorInvalid
                  leftPadding: customStyle.fields.sideMargin
                  horizontalAlignment: Text.AlignLeft
                  verticalAlignment: Text.AlignVCenter
              }
          }

      up.indicator: Rectangle {
              x: spinbox.mirrored ? 0 : parent.width - width
              height: parent.height
              implicitWidth: parent.height
              implicitHeight: parent.height
              color: customStyle.fields.backgroundColor
              radius: customStyle.fields.cornerRadius

              Text {
                  text: "+"
                  height: parent.height
                  font.pixelSize: spinbox.font.pixelSize * 2
                  font.bold: true
                  fontSizeMode: Text.Fit
                  color: fieldItem.enabled ? customStyle.fields.fontColor : customStyle.toolbutton.backgroundColorInvalid
                  anchors.right: parent.right
                  rightPadding: customStyle.fields.sideMargin
                  horizontalAlignment: Text.AlignRight
                  verticalAlignment: Text.AlignVCenter
              }
          }
    }

    // Slider
    Text {
      id: valueLabel
      visible: fieldItem.widgetStyle === "Slider"
      width: rowLayout.width/3
      height: fieldItem.height
      elide: Text.ElideRight
      text: Number(slider.value).toFixed(precision).toLocaleString(fieldItem.locale) + fieldItem.suffix
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignLeft
      font.pointSize: customStyle.fields.fontPointSize
      color: customStyle.fields.fontColor
      padding: 10 * QgsQuick.Utils.dp
      leftPadding: customStyle.fields.sideMargin
    }

    Slider {
      id: slider
      visible: fieldItem.widgetStyle === "Slider"
      value: fieldItem.parent.value ? fieldItem.parent.value : 0
      width: parent.width - valueLabel.width
      height: fieldItem.height
      implicitWidth: width
      from: fieldItem.from
      to: fieldItem.to
      stepSize: fieldItem.step
      rightPadding: customStyle.fields.sideMargin

      onValueChanged: {
        if (visible) {
          fieldItem.valueChanged(slider.value, false)
        }
      }

      background: Rectangle {
        x: slider.leftPadding
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        implicitWidth: slider.width
        implicitHeight: slider.height * 0.1
        width: slider.availableWidth
        height: implicitHeight
        radius: 2 * QgsQuick.Utils.dp
        color: fieldItem.enabled ? customStyle.fields.fontColor : customStyle.fields.backgroundColorInactive
      }

      handle: Rectangle {
              x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
              y: slider.topPadding + slider.availableHeight / 2 - height / 2
              implicitWidth: slider.height * 0.6 * 0.66 + (2 * border.width) // Similar to indicator SwitchWidget of CheckBox widget
              implicitHeight: implicitWidth
              radius: height * 0.5
              color: "white"
              border.color: customStyle.fields.backgroundColorInactive
          }
    }
  }

}
