import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.3
import QgsQuick 0.1 as QgsQuick
import "."  // import InputStyle singleton

Item {
    signal openProjectClicked()
    signal myLocationClicked()
    signal myLocationHold()
    signal addFeatureClicked()
    signal openMapThemesClicked()
    signal openSettingsClicked()
    signal zoomToProject()
    property alias recordButton: recBtnIcon

    property real itemSize: mainPanel.height * 0.8
    property color gpsIndicatorColor: InputStyle.softRed

    id: mainPanel

    Rectangle {
        anchors.fill: parent
        color: InputStyle.clrPanelBackground
        opacity: InputStyle.panelOpacity
    }

    Row {
        id: panelRow
        anchors.fill: parent
        height: mainPanel.itemSize
        property real itemWidth: mainPanel.height * 1.2
        property int itemsToShow: Math.min((width / panelRow.itemWidth), children.length) - 1
        property real calculatedItemWidth: itemsToShow ? parent.width/itemsToShow : parent.width

        Item {
            id: openProjectsItem
            height: parent.height
            visible: panelRow.itemsToShow > 1
            width: visible ? panelRow.calculatedItemWidth : 0

            MainPanelButton {
                id: openProjectBtn
                width: mainPanel.itemSize
                text: qsTr("Projects")
                imageSource: "project.svg"

                onActivated: mainPanel.openProjectClicked()
            }
        }

        Item {
            id: myLocationItem
            height: parent.height
            visible: panelRow.itemsToShow > 2
            width: visible ? panelRow.calculatedItemWidth : 0

            MainPanelButton {
                id: myLocationBtn
                width: mainPanel.itemSize

                text: qsTr("GPS")
                imageSource: "ic_gps_fixed_48px.svg"
                imageSource2: "ic_gps_not_fixed_48px.svg"
                imageSourceCondition: __appSettings.autoCenterMapChecked

                onActivated: mainPanel.myLocationClicked()
                onActivatedOnHold: mainPanel.myLocationHold()

                RoundIndicator {
                    width: parent.height/4
                    height: width
                    anchors.right: parent.right
                    anchors.top: parent.top
                    color: gpsIndicatorColor
                }
            }
        }

        Item {
            id: recItem
            height: parent.height
            visible: panelRow.itemsToShow > 3
            width: visible ? panelRow.calculatedItemWidth : 0

            MainPanelButton {
                id: recBtn
                width: mainPanel.itemSize
                text: qsTr("Record")

                RecordBtn {
                    id: recBtnIcon
                    width: mainPanel.itemSize
                    anchors.top: parent.top
                    anchors.margins: width/4
                    anchors.topMargin: -anchors.margins/2
                    enabled: true
                }

                onActivated: mainPanel.addFeatureClicked()
            }
        }

        Item {
            id: zoomToProjectItem
            height: parent.height
            visible: panelRow.itemsToShow > 4
            width: visible ? panelRow.calculatedItemWidth : 0

            MainPanelButton {

                id: zoomToProjectBtn
                width: mainPanel.itemSize
                text: qsTr("Zoom to project")
                imageSource: "zoom_to_project.svg"

                onActivated:mainPanel.zoomToProject()
            }
        }

        Item {
            id: mapThemesItem
            height: parent.height
            visible: panelRow.itemsToShow > 5
            width: visible ? panelRow.calculatedItemWidth : 0

            MainPanelButton {

                id: mapThemesBtn
                width: mainPanel.itemSize
                text: qsTr("Map themes")
                imageSource: "map_styles.svg"
                onActivated: mainPanel.openMapThemesClicked()
            }
        }

        // Last item
        Item {
            id: settingsItem
            height: parent.height
            visible: panelRow.itemsToShow > 5
            width: visible ? panelRow.calculatedItemWidth : 0

            MainPanelButton {

                id: settingsBtn
                width: mainPanel.itemSize
                text: qsTr("Settings")
                imageSource: "settings.svg"
                onActivated: mainPanel.openSettingsClicked()
            }
        }

        Item {
            width: panelRow.calculatedItemWidth
            height: parent.height
            visible: !settingsItem.visible

            MainPanelButton {
                id: menuBtn
                width: mainPanel.itemSize
                text: qsTr("More")
                imageSource: "more_menu.svg"
                onActivated: {
                    if (rootMenu.isOpen) {
                        rootMenu.close()
                    } else {
                        rootMenu.open()
                    }
                }
            }
        }
    }

    Menu {
        id: rootMenu
        title: "Menu"
        x:parent.width - rootMenu.width
        y: -rootMenu.height
        visible: menuBtn.visible
        property bool isOpen: false
        width: parent.width < 300 * QgsQuick.Utils.dp ? parent.width : 300 * QgsQuick.Utils.dp
        closePolicy: Popup.CloseOnPressOutside

        onClosed: isOpen = false
        onOpened: isOpen = true

        MenuItem {
            width: parent.width
            visible: !openProjectsItem.visible
            height: visible ? mainPanel.itemSize : 0

            ExtendedMenuItem {
                height: mainPanel.itemSize
                rowHeight: height
                width: parent.width
                contentText: qsTr("Projects")
                imageSource: "project.svg"
            }

            onClicked: {
                openProjectBtn.activated()
                rootMenu.close()
            }
        }

        MenuItem {
            width: parent.width
            visible: !myLocationItem.visible
            height: visible ? mainPanel.itemSize : 0

            ExtendedMenuItem {
                height: mainPanel.itemSize
                rowHeight: height
                width: parent.width
                contentText: qsTr("GPS")
                imageSource: __appSettings.autoCenterMapChecked ? "ic_gps_fixed_48px.svg" : "ic_gps_not_fixed_48px.svg"
            }

            onClicked: {
                myLocationBtn.activated()
                rootMenu.close()
            }
        }

        MenuItem {
            width: parent.width
            visible: !recItem.visible
            height: visible ? mainPanel.itemSize : 0

            ExtendedMenuItem {
                height: mainPanel.itemSize
                rowHeight: height
                width: parent.width
                contentText: qsTr("Record")

                RecordBtn {
                    id: recBtnIcon2
                    width: mainPanel.itemSize
                    anchors.margins: width/4
                    anchors.topMargin: -anchors.margins/2
                    enabled: true
                    color: InputStyle.fontColor
                }
            }

            onClicked: {
                recBtn.activated()
                rootMenu.close()
            }
        }

        MenuItem {
            width: parent.width
            visible: !zoomToProjectItem.visible
            height: visible ? mainPanel.itemSize : 0

            ExtendedMenuItem {
                height: mainPanel.itemSize
                rowHeight: height
                width: parent.width
                contentText: qsTr("Zoom to project")
                imageSource: "zoom_to_project.svg"
            }

            onClicked: {
                zoomToProjectBtn.activcated()
                rootMenu.close()
            }
        }

        MenuItem {
            width: parent.width
            visible: !mapThemesItem.visible
            height: visible ? mainPanel.itemSize : 0

            ExtendedMenuItem {
                height: mainPanel.itemSize
                rowHeight: height
                width: parent.width
                contentText: qsTr("Map themes")
                imageSource: "map_styles.svg"
            }

            onClicked: {
                mapThemesBtn.activated()
                rootMenu.close()
            }
        }

        MenuItem {
            visible: !settingsItem.visible
            height: visible ? mainPanel.itemSize : 0
            width: parent.width

            ExtendedMenuItem {
                anchors.fill: parent
                rowHeight: parent.height
                width: parent.width
                contentText: qsTr("Settings")
                imageSource: "settings.svg"
            }

            onClicked: {
                settingsBtn.activated()
                rootMenu.close()
            }
        }
    }

    // Menu shadow
    Rectangle {
        x: rootMenu.x
        y: rootMenu.y
        width: rootMenu.width
        height: rootMenu.height
        layer.enabled: true
        layer.effect: Shadow {}
        visible: rootMenu.isOpen
    }


}
