

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

RowLayout {
  id: root

  signal backClicked

  /* translate in parent! */
  required property var headerTitle

  /* -1 no step bar shown; 1, 2, 3 */
  property int step: -1

  property bool backVisible: true

  RoundButton {
    visible: backVisible
    onClicked: root.backClicked()
    text: "<"
  }

  Label {
    text: headerTitle
  }

  ProgressBar {
    visible: step > 0
    from: 0
    to: 3
    value: step > 0 ? step : 0
  }
}
