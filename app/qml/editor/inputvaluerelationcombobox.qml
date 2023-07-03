/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import "../components"
import lc 1.0

AbstractEditor {
  id: root

  /*required*/ property var parentValue: root.parent.value
  /*required*/ property bool parentValueIsNull: root.parent.valueIsNull
  /*required*/ property var fieldConfig: root.parent.config
  /*required*/ property var featureLayerPair: root.parent.featurePair
  /*required*/ property var isReadOnly: root.parent.readOnly

  signal editorValueChanged( var newValue, bool isNull )

  function reload() // called from FeatureForm when form is recalculated
  {
    if ( !root.isReadOnly )
    {
      vrModel.pair = root.featureLayerPair
    }
  }

  function setIndex()
  {
    let fid = vrModel.convertFromQgisType( root.parentValue, FeaturesModel.FeatureId )

    if ( !Array.isArray( fid ) || !fid.length )
    {
      combobox.currentIndex = -1
    }
    else
    {
      combobox.currentIndex = vrModel.rowFromRoleValue( FeaturesModel.FeatureId, fid[0] )
    }

    combobox.popup.close() //  combobox might still be opened
  }

  onParentValueChanged: {
    vrModel.pair = root.featureLayerPair
  }

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

    onDonePopulating: {
      // we need to re-set the current index every time the model is done re-populating
      setIndex()
    }
  }

  content: InputComboBox {
    id: combobox

    comboStyle: customStyle.fields
    textRole: 'FeatureTitle'
    height: parent.height
    readOnly: isReadOnly
    iconSize: height / 2
    model: vrModel

    onItemClicked: function( index ) {
      // We need to convert feature id to string prior to sending it to C++ in order to
      // avoid conversion to scientific notation.
      let fid = index.toString()
      editorValueChanged( vrModel.convertToKey( fid ), false )
    }
  }
}
