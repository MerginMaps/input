/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import "."
import ".."

Rectangle {
  id: control

  required property real position // [0 - 1]

  StyleV2 { id: styleV2 }

  width: parent.width
  height: 12 * __dp
  color: StyleV2.lightGreenColor
  radius: height / 2

  Rectangle {
    width: parent.width * control.position
    height: parent.height
    color: StyleV2.grassColor
    radius: height / 2
  }
}
