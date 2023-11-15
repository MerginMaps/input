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

import "../../app/qmlV2/component"
import "../../app/qmlV2/"

ScrollView {
  Column {
    padding: 20
    spacing: 20

    GroupBox {
      title: "MMInput"
      background: Rectangle {
        color: "lightGray"
        border.color: "gray"
      }
      label: Label {
        color: "black"
        text: parent.title
        padding: 5
      }

      Column {
        spacing: 10
        anchors.fill: parent
        MMInput {
          placeholderText: "Place holder"
        }
        MMInput {
          text: "Disabled"
          enabled: false
        }
        Column {
          TextInput { text: "iconSource: StyleV2.searchIcon" }
          MMInput {
            placeholderText: "Search"
            iconSource: StyleV2.searchIcon
          }
        }
        Column {
          TextInput { text: "iconSource: StyleV2.calendarIcon" }
          TextInput { text: "warningMsg: ..." }
          MMInput {
            text: "Calendar"
            iconSource: StyleV2.calendarIcon
            warningMsg: "Would you like to be so kind and select a date please?"
          }
        }
      }
    }

    GroupBox {
      title: "MMPasswordInput"
      background: Rectangle {
        color: "lightGray"
        border.color: "gray"
      }
      label: Label {
        color: "black"
        text: parent.title
        padding: 5
      }

      Column {
        spacing: 10
        anchors.fill: parent
        MMPasswordInput {
          text: "Password"
          regexp: '(?=.*[a-z])(?=.*[A-Z])(?=.*[0-9])(?=.*[^A-Za-z0-9])(?=.{6,})'
          errorMsg: "Password must contain at least 6 characters\nMinimum 1 number, uppercase and lowercase letter and special character"
        }
        MMPasswordInput {
          text: "Password"
          regexp: '(?=.*[a-z])(?=.*[A-Z])(?=.*[0-9])(?=.*[^A-Za-z0-9])(?=.{6,})'
          enabled: false
        }
      }
    }

    GroupBox {
      title: "MMButtonInput"
      background: Rectangle {
        color: "lightGray"
        border.color: "gray"
      }
      label: Label {
        color: "black"
        text: parent.title
        padding: 5
      }

      Column {
        spacing: 10
        anchors.fill: parent
        MMButtonInput {
          text: "Copy text to clipboard"
          buttonText: "Copy"
          onClicked: { textToClipboard(); console.log("Text in clipboard") }
        }
        MMButtonInput {
          text: "Send"
          buttonText: "Send"
          enabled: false
          onClicked: console.log("Clicked")
        }
      }
    }
  }
}
