/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
import QtQuick 2.14
import "../components"
import lc 1.0

AbstractEditor {
  id: root
  /*required*/
  property var featureLayerPair: root.parent.featurePair
  /*required*/
  property var fieldConfig: root.parent.config
  /*required*/
  property var isReadOnly: root.parent.readOnly

  /*required*/
  property var parentValue: root.parent.value

  enabled: !isReadOnly

  signal editorValueChanged(var newValue, bool isNull)
  function reload() {
    // called from FeatureForm when form is recalculated
    if (!root.isReadOnly) {
      vrModel.pair = root.featureLayerPair;
      setIndex();
    }
  }
  function setIndex() {
    let fid = vrModel.convertFromQgisType(root.parentValue, FeaturesModel.FeatureId);
    if (!Array.isArray(fid) || !fid.length) {
      combobox.currentIndex = -1;
    } else {
      combobox.currentIndex = vrModel.rowFromRoleValue(FeaturesModel.FeatureId, fid[0]);
    }
    combobox.popup.close(); //  combobox might still be opened
  }

  onParentValueChanged: {
    vrModel.pair = root.featureLayerPair;
    setIndex();
  }

  ValueRelationFeaturesModel {
    id: vrModel
    config: root.fieldConfig
    pair: root.featureLayerPair

    onInvalidate: {
      if (!root.isReadOnly) {
        root.editorValueChanged("", true);
      }
    }
  }

  content: InputComboBox {
    id: combobox
    comboStyle: customStyle.fields
    height: parent.height
    iconSize: height / 2
    model: vrModel
    readOnly: isReadOnly
    textRole: 'FeatureTitle'

    onItemClicked: {
      // We need to convert feature id to string prior to sending it to C++ in order to
      // avoid conversion to scientific notation.
      let fid = index.toString();
      editorValueChanged(vrModel.convertToKey(fid), false);
    }
  }
}
