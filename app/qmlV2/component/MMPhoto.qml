import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import "."
import ".."

Image {
  id: control

  signal clicked( var path )

  width: visible ? 120 * __dp : 0
  height: width
  source: model.modelData
  asynchronous: true
  layer.enabled: true
  layer {
    effect: OpacityMask {
      maskSource: Item {
        width: control.width
        height: control.height
        Rectangle {
          anchors.centerIn: parent
          width: parent.width
          height: parent.height
          radius: 20 * __dp
        }
      }
    }
  }
  MouseArea {
    anchors.fill: parent
    onClicked: control.clicked(model.modelData)
  }
  Rectangle {
    anchors.centerIn: parent
    width: parent.width
    height: parent.height
    radius: 20 * __dp
    color: StyleV2.transparentColor
    border.color: StyleV2.forestColor
    border.width: 1 * __dp
  }

  onStatusChanged: {
    if (status === Image.Error) {
      console.error("MMPhoto: Error loading image: " + model.modelData);
    }
  }
}
