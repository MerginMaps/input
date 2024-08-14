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

import mm 1.0 as MM

Item {
    id: root

    // Rectangle { //map
    //     anchors.fill: parent
    //     color: "lightblue"
    // }

    /*required*/ property var qgsProject
    /*required*/ property var mapSettings
    property bool shouldUseSnapping: false

    property point center: Qt.point( root.width / 2, root.height / 2 )

    property var recordPoint: snapUtils.recordPoint

    property point screenPoint: snapUtils.snapped && __activeLayer.vectorLayer ? __inputUtils.transformPointToScreenCoordinates(__activeLayer.vectorLayer.crs, mapSettings, recordPoint) : center

    property real outerSize: 60 * __dp
    property real innerDotSize: 10 * __dp

    MM.SnapUtils {
        id: snapUtils

        centerPosition: root.center
        mapSettings: root.mapSettings
        qgsProject: root.qgsProject
        useSnapping: root.shouldUseSnapping
        destinationLayer: __activeLayer.vectorLayer

    }

    Image {
        id: crosshairBackground // white background of the crosshair

        // Rectangle {
        //     anchors.fill: parent
        //     color: "lightcoral"
        // }

        x: root.screenPoint.x - width / 2
        y: root.screenPoint.y - height / 2

        Behavior on x {
            PropertyAnimation {
                properties: "x"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        Behavior on y {
            PropertyAnimation {
                properties: "y"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        height: root.outerSize
        width: height

        source: __style.crosshairBackgroundImage
        sourceSize.width: width
        sourceSize.height: height
    }

    Image {
        id: crosshairForeground // green / purple outer circle of the crosshair

        // Rectangle {
        //     anchors.fill: parent
        //     color: "lightyellow"
        // }

        x: root.screenPoint.x - width / 2
        y: root.screenPoint.y - height / 2

        Behavior on x {
            PropertyAnimation {
                properties: "x"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        Behavior on y {
            PropertyAnimation {
                properties: "y"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        height: root.outerSize
        width: height

        source: __style.crosshairForegroundImage
        sourceSize.width: width
        sourceSize.height: height
    }

    ColorOverlay {
        anchors.fill: crosshairForeground
        source: crosshairForeground
        color: snapUtils.snapped ? __style.snappingColor : __style.forestColor

        // Rectangle {
        //     anchors.fill: parent
        //     color: "lightpink"
        // }
    }

    Image {
        id: crossCenterDot // Center dot - visible when not snapped (green)

        // Rectangle {
        //     anchors.fill: parent
        //     color: "lightgrey"
        // }

        x: root.screenPoint.x - width / 2
        y: root.screenPoint.y - height / 2

        Behavior on x {
            PropertyAnimation {
                properties: "x"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        Behavior on y {
            PropertyAnimation {
                properties: "y"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        opacity: snapUtils.snapped ? 0 : 100

        Behavior on opacity {
            PropertyAnimation {
                properties: "opacity"
                duration: 100
                easing.type: Easing.InQuad
            }
        }

        height: root.innerDotSize
        width: height
        sourceSize.width: width
        sourceSize.height: height

        source: __style.crosshairCenterImage
    }

    Image {
        id: crossCenterPlus // Center plus - visible when not snapped (purple)

        // Rectangle {
        //     anchors.fill: parent
        //     color: "lightseagreen"
        // }

        x: root.screenPoint.x - width / 2
        y: root.screenPoint.y - height / 2

        Behavior on x {
            PropertyAnimation {
                properties: "x"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        Behavior on y {
            PropertyAnimation {
                properties: "y"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        opacity: snapUtils.snapped && ( snapUtils.snapType === MM.SnapUtils.Vertex || snapUtils.snapType === MM.SnapUtils.Other ) ? 100 : 0

        Behavior on opacity {
            PropertyAnimation {
                properties: "opacity"
                duration: 100
                easing.type: Easing.InQuad
            }
        }

        rotation: snapUtils.snapType === MM.SnapUtils.Other ? 0 : 45

        Behavior on rotation {
            PropertyAnimation {
                properties: "rotation"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        height: root.innerDotSize * 2
        width: height
        sourceSize.width: width
        sourceSize.height: height

        // Important: must be same color as __style.snappingColor
        source: __style.crosshairPlusImage
    }

    Image {
        id: crossCenterCircle // Center circle - visible when snapped to segment (purple)

        // Rectangle {
        //     anchors.fill: parent
        //     color: "lightsteelblue"
        // }

        x: root.screenPoint.x - width / 2
        y: root.screenPoint.y - height / 2

        Behavior on x {
            PropertyAnimation {
                properties: "x"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        Behavior on y {
            PropertyAnimation {
                properties: "y"
                duration: 50
                easing.type: Easing.InQuad
            }
        }

        opacity: snapUtils.snapped && snapUtils.snapType === MM.SnapUtils.Segment ? 100 : 0

        Behavior on opacity {
            PropertyAnimation {
                properties: "opacity"
                duration: 100
                easing.type: Easing.InQuad
            }
        }

        height: root.innerDotSize * 2
        width: height
        sourceSize.width: width
        sourceSize.height: height

        // Important: must be same color as __style.snappingColor
        source: __style.crosshairCircleImage
    }

    MMMapLabel {
        // Rectangle {
        //     anchors.fill: parent
        //     color: "lightgoldenrodyellow"
        // }

        //height:50
        //width: 70
        visible: true //root.state !== "inactive" && root.isStreaming
        //iconSource: __style.streamingIcon

        maxWidth: crosshairForeground.width + 6 * __dp
        bgColor: __style.forestColor
        textColor: __style.polarColor
        textBgColorInverted: false

        text: qsTr( "Test" )

        anchors.top: crosshairForeground.bottom
        anchors.horizontalCenter: crosshairForeground.horizontalCenter

        onClicked: console.log("MapLabel")
    }

    Connections {

        target: __activeProject

        function onProjectWillBeReloaded() {
            snapUtils.clear()
        }

        function onProjectReloaded( project ) {
            // We need to re-assign qgs project to snaputils, because
            // even though we loaded a different project,
            // internally we keep the same pointer for QgsProject.
            snapUtils.qgsProject = __activeProject.qgsProject
            snapUtils.mapSettings = root.mapSettings
        }
    }
}
