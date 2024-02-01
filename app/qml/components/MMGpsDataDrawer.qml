/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import "."
import ".."
import lc 1.0

Drawer {
  id: root

  property alias title: title.text
  property int minFeaturesCountToFullScreenMode: 4
  //property string coordinatesInDegrees: __inputUtils.degreesString( __positionKit.positionCoordinate )

  padding: 20 * __dp

  width: ApplicationWindow.window.width
  height: (mainColumn.height > ApplicationWindow.window.height ? ApplicationWindow.window.height : mainColumn.height) - 20 * __dp
  edge: Qt.BottomEdge

  Rectangle {
    color: roundedRect.color
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: 2 * radius
    anchors.topMargin: -radius
    radius: 20 * __dp
  }

  ListModel {
    id : model

    Component.onCompleted: {

      var source = __positionKit.positionProvider ? __positionKit.providerName: qsTr( "No receiver" )
      var status = __positionKit.positionProvider ? __positionKit.providerMessage : ""

      //var string latitude = { return __positionKit.latitude}
      // var longitude = {
      //     if (!__positionKit.hasPosition || Number.isNaN(__positionKit.latitude)) {
      //         return "N/A";
      //     }

      //     let coordParts = root.coordinatesInDegrees.split(", ")
      //     if (coordParts.length > 1) {
      //         return coordParts[0];
      //     }

      //     return "N/A";
      // }

      //var latitude = 22

      append({ FeatureId: 1,
               FeatureTitleLeft: "Source",
               //DescriptionLeft: source,
               DescriptionLeft: function () {
                   if (!__positionKit.hasPosition || Number.isNaN(__positionKit.latitude)) {
                       return "N/A";
                   }

                   let coordParts = root.coordinatesInDegrees.split(", ")
                   if (coordParts.length > 1) {
                       return coordParts[0];
                   }

                   return "N/A";
               },
               showLeftColumn: true,
               FeatureTitleRight: "Status",
               DescriptionRight: status,
               showRightColumn: true
               // showRightColumn: function() {
               //   return (__positionKit.positionProvider && __positionKit.providerType) === "external" ? true: false;
               // }
             });

      append({ FeatureId: 2,
               FeatureTitleLeft: "Latitude",
               DescriptionLeft: function () {
                 return  __positionKit.latitude
               },
               // DescriptionLeft: function () {
               //     if (!__positionKit.hasPosition || Number.isNaN(__positionKit.latitude)) {
               //         return "N/A";
               //     }

               //     let coordParts = root.coordinatesInDegrees.split(", ")
               //     if (coordParts.length > 1) {
               //         return coordParts[0];
               //     }

               //     return "N/A";
               // },
               showLeftColumn: __positionKit.hasPosition,
               FeatureTitleRight: "Longitude",
               DescriptionRight: __positionKit.longitude,
               // DescriptionRight: {
               //   if ( !__positionKit.hasPosition || Number.isNaN( __positionKit.longitude ) ) {
               //     return "N/A";
               //   }

               //   let coordParts = root.coordinatesInDegrees.split(", ")
               //   if ( coordParts.length > 1 )
               //     return coordParts[0]

               //   return "N/A";
               // },
               showRightColumn: __positionKit.hasPosition 
             });

      append({ FeatureId: 3,
               FeatureTitleLeft: "X",
               DescriptionLeft: function () {
                 return __positionKit.x
               },
               showLeftColumn: true,
               FeatureTitleRight: "Y",
               DescriptionRight: function (){
                 return __positionKit.y
               },
               showRightColumn: true
             });

      append({ FeatureId: 4,
               FeatureTitleLeft: "Horizontal accuracy",
               DescriptionLeft: function () {
                 return __positionKit.horizontalAccuracy
               },
               showLeftColumn: true,
               FeatureTitleRight: "Vertical accuracy",
               DescriptionRight: function () {
                 return __positionKit.verticalAccuracy
               },
               showRightColumn: true
             });

      append({ FeatureId: 5,
               FeatureTitleLeft: "Altitude",
               DescriptionLeft: function () {
                 return __positionKit.altitude + " m";
               },
               showLeftColumn: true,
               FeatureTitleRight: "Satellites (in use/view)",
               DescriptionRight: function () {
                 return __positionKit.satellitesUsed + "/" + __positionKit.satellitesVisible;
               },
               showRightColumn: true
             });

      append({ FeatureId: 6,
               FeatureTitleLeft: "Speed",
               DescriptionLeft: function () {
                 return __positionKit.speed + " km/h";
               },
               showLeftColumn: true,
               FeatureTitleRight: "Last Fix",
               DescriptionRight: function() {
                 return __positionKit.lastRead
                },
               showRightColumn: true
             });

      append({ FeatureId: 7,
               FeatureTitleLeft: "GPS antenna height",
               DescriptionLeft: function() {
                 return __positionKit.gpsAntennaHeight > 0 ? __positionKit.gpsAntennaHeight + " m" : qsTr( "Not set" )
               },
               showLeftColumn: true,
             });
    }
  }

  Rectangle {
    id: roundedRect

    anchors.fill: parent
    color: __style.whiteColor

    Column {
      id: mainColumn

      width: parent.width
      spacing: 40 * __dp
      leftPadding: root.padding
      rightPadding: root.padding
      bottomPadding: root.padding

      Row {
        width: parent.width - 2 * root.padding
        anchors.horizontalCenter: parent.horizontalCenter

        Item { width: closeButton.width; height: 1 }

        Text {
          id: title

          anchors.verticalCenter: parent.verticalCenter
          font: __style.t1
          width: parent.width - closeButton.width * 2
          color: __style.forestColor
          horizontalAlignment: Text.AlignHCenter
          verticalAlignment: Text.AlignVCenter
          elide: Text.ElideRight
        }

        Image {
          id: closeButton

          source: __style.closeButtonIcon

          MouseArea {
            anchors.fill: parent
            onClicked: root.visible = false
          }
        }
      }

      ListView {
        id: listView

        bottomMargin: primaryButton.height + 80 * __dp
        width: parent.width - 2 * root.padding
        height: {
          if(model.count >= root.minFeaturesCountToFullScreenMode) {
            if(ApplicationWindow.window)
              return (ApplicationWindow.window.height) * __dp
            else return 0
          }
          if(model)
            return model.count * __style.comboBoxItemHeight
          return 0
        }

        clip: true

        model: model

        delegate: Item {
          id: delegate

          width: listView.width
          height: __style.comboBoxItemHeight

          Row {
            id: itemsRow
            height: parent.height
            width: parent.width
            anchors.left: parent.left
            anchors.right: parent.right

            Column {
              width: parent.width * 0.5
              height: parent.height
              visible: showLeftColumn

              Text {
                text: FeatureTitleLeft
                color: __style.nightColor
                font: __style.p6
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 8
              }

              Text {
                text: DescriptionLeft
                color: __style.nightColor
                font: __style.t3
                elide: Text.ElideLeft
                horizontalAlignment: Text.AlignRight
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.bottomMargin: 8
              }
            }

            Column {
              width: parent.width * 0.5
              height: parent.height
              visible: showRightColumn

              Text {
                text: FeatureTitleRight
                color: __style.nightColor
                font: __style.p6
                width: parent.width
                horizontalAlignment: Text.AlignRight
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 8
              }

              Text {
                text: DescriptionRight
                color: __style.nightColor
                font: __style.t3
                width: parent.width
                horizontalAlignment: Text.AlignRight
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottomMargin: 8
              }
            }
          }

          Rectangle {
            width: parent.width
            height: (1 * __dp) < 1 ? 1 : 1 * __dp
            color: __style.greyColor
            visible: true
            anchors.bottom: parent.bottom
          }
        }
      }
    }

    Item {
      anchors.fill: parent
      height: 5
    }

    MMButton {
      id: primaryButton

      width: parent.width - 2 * 20 * __dp
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.bottom: parent.bottom
      anchors.bottomMargin: 20 * __dp

      text: qsTr("Manage GPS receivers")

      onClicked: {
        additionalContent.push( positionProviderComponent )
      }
    }

    // Component {
    //   id: positionProviderComponent
    //   PositionProviderPage {
    //     onClose: additionalContent.pop(null)
    //     stackView: additionalContent
    //     Component.onCompleted: forceActiveFocus()
    //   }
    // }
  }
}
