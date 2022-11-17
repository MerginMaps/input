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
import QtQuick.Layouts
import QtMultimedia
import lc 1.0 as InputClass

import "./components"

Item {
  id: root

  signal backButtonClicked()
  signal scanFinished( var data )

  Page {
    id: scannerPage

    anchors.fill: parent

    header: PanelHeader {
      id: scannerPageHeader

      width: parent.width
      height: InputStyle.rowHeightHeader

      color: InputStyle.clrPanelMain
      rowHeight: InputStyle.rowHeightHeader

      titleText: qsTr( "Scan code" )

      onBack: {
        qrcodeScanner.setProcessing( false )
        root.backButtonClicked()
       }

      withBackButton: true
    }

    InputClass.CodeScanner {
      id: qrcodeScanner

      videoSink: videoOutput.videoSink

      captureRect: Qt.rect(root.width / 4, root.height / 4, root.width / 2, root.height / 2)

      onCapturedStringChanged: function( captured ) {
        qrcodeScanner.setProcessing( false )
        root.scanFinished( captured )
      }
    }

    Rectangle {
      id: videoContainer
      width: root.width
      height: root.height - header.height
      color: InputStyle.clrPanelBackground

      VideoOutput {
        id: videoOutput

        width: root.width
        height: root.height
        focus: visible
        fillMode: VideoOutput.PreserveAspectCrop
      }

      CodeScannerOverlay {
        id: scannerOverlay

        rectSize: Math.min(root.height, root.width) * 0.8
        width: root.width
        height: root.height - header.height
      }
    }
  }
}
