/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick

import "../components" as MMComponents
import "../components/private" as MMPrivateComponents

MMPrivateComponents.MMBaseSingleLineInput {
  id: root

  property bool delayedSearch: false
  property int emitInterval: 200
  property bool showClearIcon: true
  property string searchText: ""

  leftContent: MMComponents.MMIcon {
    id: searchIcon

    size: __style.icon24
    source: __style.searchIcon
    color: root.enabled ? __style.nightColor : __style.mediumGreenColor
  }

  rightContent: MMComponents.MMIcon {
    id: rightIcon

    size: __style.icon24
    source: __style.closeIcon
    color: root.enabled ? __style.forestColor : __style.mediumGreenColor
  }

  rightContentVisible: root.showClearIcon && textField.activeFocus && root.text.length > 0

  onRightContentClicked: {
    if ( root.showClearIcon ) {
      textField.clear()
      root.searchText = ""
    }
    else {
      // if the clear button should not be there, let's open keyboard instead
      textField.forceActiveFocus()
    }
  }

  onTextEdited: {
    if ( root.delayedSearch ) {
      searchTimer.restart()
    }
    else
    {
      root.searchText = root.text
    }
  }

  Timer {
    id: searchTimer

    interval: root.emitInterval
    running: false

    onTriggered: root.searchText = root.text
  }

  /**
    * Used for deactivating focus on MMSearchInput when another component should have focus.
    * and the current element's forceActiveFocus() doesnt deactivates SearchBar focus.
    */
  function deactivate() {
    root.textField.focus = false
    if ( root.text.length > 0 )
      root.textField.clear()
    root.searchText = ""
  }
}
