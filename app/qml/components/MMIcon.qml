/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import Qt5Compat.GraphicalEffects

Item {
  id: root

  required property real size
  required property url source
  required property color color

  width: size
  height: size

  Image {
    id: icon

    source: root.source
    anchors.fill: parent
  }

  ColorOverlay {
    id: overlay

    color: root.color
    anchors.fill: icon
    source: icon
  }
}
