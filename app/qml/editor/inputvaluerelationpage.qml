﻿/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

import lc 1.0
import "../"

AbstractEditor {
  id: root

  /*required*/ property var parentValue: root.parent.value
  /*required*/ property bool parentValueIsNull: root.parent.valueIsNull
  /*required*/ property var fieldConfig: root.parent.config
  /*required*/ property var featureLayerPair: root.parent.featurePair
  /*required*/ property bool isReadOnly: root.parent.readOnly
  /*required*/ property var stackView: root.parent.formView

  property bool allowMultivalue: config["AllowMulti"]

  signal editorValueChanged( var newValue, bool isNull )

  function reload()
  {
    if ( !root.isReadOnly )
    {
      vrModel.pair = root.featureLayerPair
    }
  }

  function setText()
  {
    title.text = vrModel.convertFromQgisType( root.parentValue, FeaturesModel.FeatureTitle ).join( ', ' )
  }

  function pushVrPage()
  {
    let props = {
      pageTitle: labelAlias,
      allowMultiselect: root.allowMultivalue,
      toolbarVisible: root.allowMultivalue,
      preselectedFeatures: root.allowMultivalue ? vrModel.convertFromQgisType( root.parentValue, FeaturesModel.FeatureId ) : []
    }
    let obj = root.stackView.push( featuresPageComponent, props )
    obj.forceActiveFocus()
  }

  onParentValueChanged: {
    vrModel.pair = root.featureLayerPair
    // we call setText here too in case the pair did not change
    setText()
  }

  onRightActionClicked: pushVrPage()
  onContentClicked: pushVrPage()

  enabled: !isReadOnly

  ValueRelationFeaturesModel {
    id: vrModel

    config: root.fieldConfig
    pair: root.featureLayerPair

    onInvalidate: {
      if ( root.parentValueIsNull )
      {
        return // ignore invalidate signal if value is already NULL
      }
      if ( root.isReadOnly )
      {
        return // ignore invalidate signal if form is not in edit mode
      }
      root.editorValueChanged( "", true )
    }

    onFetchingResultsChanged: {
      if ( !isFetching )
      {
        setText()
      }
    }
  }

  content: Text {
    id: title

    anchors.fill: parent
    anchors.leftMargin: customStyle.fields.sideMargin
    color: customStyle.fields.fontColor
    font.pixelSize: customStyle.fields.fontPixelSize
    elide: Text.ElideRight

    horizontalAlignment: Text.AlignLeft
    verticalAlignment: Text.AlignVCenter
  }

  rightAction: Item {
    anchors.fill: parent

    Image {
      id: rightArrow

      y: parent.y + parent.height / 2 - height / 2
      x: parent.x + parent.width - 1.5 * width

      source: customStyle.icons.valueRelationMore

      width: parent.width * 0.3
      sourceSize.width: parent.width * 0.3
    }

    ColorOverlay {
      anchors.fill: rightArrow
      source: rightArrow
      color: root.isReadOnly ? customStyle.toolbutton.backgroundColorInvalid : customStyle.fields.fontColor
    }
  }

  Component {
    id: featuresPageComponent

    FeaturesListPage {
      id: featuresPage

      featuresModel: ValueRelationFeaturesModel {
          id: vrPageModel

          config: root.fieldConfig
          pair: root.featureLayerPair
        }

      pageTitle: qsTr( "Features" )
      allowSearch: true
      focus: true
      toolbarButtons: ["done"]
      toolbarVisible: false

      onBackButtonClicked: {
        root.stackView.pop()
      }

      onSelectionFinished: function( featureIds ) {
        if ( root.allowMultivalue )
        {
          let isNull = featureIds.length === 0

          if ( !isNull )
          {
            // We need to convert feature id to string prior to sending it to C++ in order to
            // avoid conversion to scientific notation.
            featureIds = featureIds.map( function(x) { return x.toString() } )
          }

          root.editorValueChanged( vrModel.convertToQgisType( featureIds ), isNull )
        }
        else
        {
          // We need to convert feature id to string prior to sending it to C++ in order to
          // avoid conversion to scientific notation.
          featureIds = featureIds.toString()
          root.editorValueChanged( vrModel.convertToKey( featureIds ), false )
        }
        root.stackView.pop()
      }
    }
  }
}
