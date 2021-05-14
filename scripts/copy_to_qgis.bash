#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "copy_to_qgis.bash QGIS_SRC_DIR"

QGIS_SRC_DIR=$1
QGIS_SRC_DIR=`realpath $QGIS_SRC_DIR`
QGSQUICK_QGIS_DIR="$QGIS_SRC_DIR/src/quickgui"
if [ ! -f "$QGSQUICK_QGIS_DIR/qgsquickutils.cpp" ]; then
  echo "Wrong QGIS_SRC_DIR, missing $QGSQUICK_QGIS_DIR/qgsquickutils.cpp!"
  exit 1;
fi

QGSQUICK_INPUT_DIR=$DIR/../qgsquick
if [ ! -f "$QGSQUICK_INPUT_DIR/qgsquickutils.cpp" ]; then
  echo "Wrong QGSQUICK_INPUT_DIR"
  exit 1;
fi

# COPY
echo "update qgsquick"
rm -rf $QGSQUICK_QGIS_DIR/*
rsync -zavh \
      --exclude="*.prepare" \
      --exclude="*.sortinc" \
      --exclude="CMakeLists.txt" \
      --exclude="qgsquickplugin.*" \
      --exclude="qmldir" \
      --exclude="*.qrc" \
      --exclude="*.pri" \
      --exclude="qgis_quick.h" \
      $QGSQUICK_INPUT_DIR/ $QGSQUICK_QGIS_DIR

# EXIT
cd $PWD
