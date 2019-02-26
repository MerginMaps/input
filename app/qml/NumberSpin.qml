import QtQuick 2.0
import QtGraphicalEffects 1.0
import "."  // import InputStyle singleton

Item {
    id: root
    property real rowHeight: height
    property real value: 0
    property string suffix
    property int maxValue: 99
    property int minValue: 0

    Image {
        id: imageDecrease
        height: root.rowHeight
        width: height
        source: "back.svg"
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectFit

        MouseArea {
            anchors.fill: parent
            onClicked: if (minValue <= root.value - 1) root.value -=1
        }
    }

    ColorOverlay {
        anchors.fill: imageDecrease
        source: imageDecrease
        color: InputStyle.fontColorBright
    }

    Text {
        id: valueText
        text: value + suffix
        color: InputStyle.fontColorBright
        font.pixelSize: InputStyle.fontPixelSizeNormal
        height: root.rowHeight
        width: root.width - (2 * imageIncrease.width)
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.left: imageDecrease.right
    }

    Image {
        id: imageIncrease
        height: root.rowHeight
        width: height
        anchors.left: valueText.right
        source: "back.svg"
        rotation: 180
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectFit

        MouseArea {
            anchors.fill: parent
            onClicked: if (maxValue >= root.value + 1) root.value +=1
        }
    }

    ColorOverlay {
        anchors.fill: imageIncrease
        source: imageIncrease
        color: InputStyle.fontColorBright
        rotation: imageIncrease.rotation
    }

}
