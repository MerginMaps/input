import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.1
import QgsQuick 0.1 as QgsQuick
import "."  // import YdnpaStyle singleton

Item {
    signal openProjectClicked()
    signal openLayersClicked()
    signal myLocationClicked()
    signal addFeatureClicked()
    signal openLogClicked()
    property alias recordButton: recBtn

    property string activeProjectName: "(none)"
    property string activeLayerName: "(none)"
    property string gpsStatus: "GPS \n (none)"

    property int itemSize: mainPanel.height * 0.8
    property int labelWidth: (mainPanel.width - 4*mainPanel.itemSize - 8*InputStyle.panelSpacing - logoYD.width - gpsLabel.width)/2

    id: mainPanel

    FontMetrics {
        id: fontMetrics
        font: gpsLabel.font
    }

    Rectangle {
        anchors.fill: parent
        color: InputStyle.clrPanelBackground
        opacity: InputStyle.panelOpacity
    }

    Image {
        id: logoYD
        x: InputStyle.panelSpacing
        source: "logo_yd.png"
        height: mainPanel.itemSize
        fillMode: Image.PreserveAspectFit
        anchors.verticalCenter: parent.verticalCenter

        MouseArea {
            anchors.fill: parent
            onClicked: {
                mainPanel.openLogClicked()
            }
        }
    }

    Row {
        x: 2*InputStyle.panelSpacing  + logoYD.width
        spacing: InputStyle.panelSpacing
        anchors.centerIn: parent
        height: parent.height

        OpenProjectBtn {
            id: openProjectBtn
            width: mainPanel.itemSize
            anchors.verticalCenter: parent.verticalCenter
            onActivated: mainPanel.openProjectClicked()
        }

        Label {
            text: mainPanel.activeProjectName
            width: mainPanel.labelWidth
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            color: InputStyle.clrPanelMain
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                onClicked: mainPanel.openProjectClicked()
            }
        }

        LayerTreeBtn {
            id: layerTreeBtn
            width: mainPanel.itemSize
            anchors.verticalCenter: parent.verticalCenter
            onActivated: mainPanel.openLayersClicked()
        }

        Label {
            text: mainPanel.activeLayerName
            width: mainPanel.labelWidth
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            color: InputStyle.clrPanelMain
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                onClicked: mainPanel.openLayersClicked()
            }
        }

        MyLocationBtn {
            id: myLocationBtn
            width: mainPanel.itemSize
            anchors.verticalCenter: parent.verticalCenter
            onActivated: mainPanel.myLocationClicked()
        }
        
        Label {
            id: gpsLabel
            text: mainPanel.gpsStatus
            height: parent.height
            width: fontMetrics.boundingRect("XXXXXXXXXXXXXXXX").width
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            color: InputStyle.clrPanelMain
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    RecordBtn {
        id: recBtn
        x: mainPanel.width - InputStyle.panelSpacing - recBtn.width
        width: myLocationBtn.width
        anchors.verticalCenter: parent.verticalCenter
        onActivated: mainPanel.addFeatureClicked()
    }
}
