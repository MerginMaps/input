/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import "../components"
import "../inputs"

Page {
  id: root

  width: parent.width

  signal backClicked
  signal signInClicked
  signal signUpClicked
  signal changeServerClicked
  signal forgotPasswordClicked

  // background as Drawer design
  Rectangle {
    anchors.fill: parent
    color: __style.whiteColor

    Rectangle {
      width: parent.width
      height: 20 * __dp
      color: __style.forestColor
    }

    Rectangle {
      width: parent.width
      height: 40 * __dp
      color: __style.whiteColor
      radius: height / 2
    }
  }

  ScrollView {
    width: parent.width
    height: parent.height - changeServerButton.height

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: ScrollBar.AlwaysOff

    Column {
      id: mainColumn

      width: root.width
      spacing: 20 * __dp
      padding: 20 * __dp

      MMHeader {
        width: parent.width - 2 * mainColumn.padding
        headerTitle: qsTr("Log In")
        titleFont: __style.h3
        backColor: __style.lightGreenColor

        onBackClicked: root.backClicked()
      }

      Item { width: 1; height: 1 }

      MMInputEditor {
        width: parent.width - 2 * mainColumn.padding
        title: qsTr("Username")
        bgColor: __style.lightGreenColor
      }

      MMPasswordEditor {
        width: parent.width - 2 * mainColumn.padding
        title: qsTr("Password")
        bgColor: __style.lightGreenColor
      }

      MMLink {
        width: parent.width - 2 * mainColumn.padding
        height: 20 * __dp
        text: qsTr("Forgot password?")

        onClicked: root.signInClicked()
      }

      Item { width: 1; height: 1 }

      MMButton {
        width: parent.width - 2 * mainColumn.padding
        text: qsTr("Sign in")

        onClicked: root.signInClicked()
      }

      Item { width: 1; height: 1 }

      MMHlineText {
        width: parent.width - 2 * mainColumn.padding
        title: qsTr("Don't have an account?")
      }

      MMLinkButton {
        width: parent.width - 2 * mainColumn.padding
        text: qsTr("Sign up")

        onClicked: root.signUpClicked()
      }
    }
  }

  MMLink {
    id: changeServerButton

    width: parent.width
    height: 50 * __dp
    anchors.bottom: parent.bottom
    text: "https://app.merginmaps.com/"
    leftIcon: __style.globeIcon

    onClicked: changeServerDrawer.visible = true
  }

  MMDrawer {
    id: changeServerDrawer

    property string newServerUrl

    title: qsTr("Change server")
    primaryButton: qsTr("Confirm")
    visible: false
    specialComponent: MMInputEditor {
      width: changeServerDrawer.width - 40 * __dp
      title: qsTr("Server address")
      bgColor: __style.lightGreenColor
      text: changeServerButton.text

      onTextChanged: changeServerDrawer.newServerUrl = text
    }

    onPrimaryButtonClicked: {
      changeServerButton.text = changeServerDrawer.newServerUrl
      visible = false

      root.changeServerClicked()
    }
  }
}
