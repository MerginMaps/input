#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PWD=`pwd`
cd $DIR

# see https://cmake-format.readthedocs.io/en/latest/configuration.html
CONFIG=$DIR/cmake_format_config.py

RETURN=0
FORMATTER=$(which cmake-lint)
if [ $? -ne 0 ]; then
	echo "[!] cmake-lint not installed." >&2
    echo "pip3 install cmakelang"
	exit 1
fi

echo $FORMATTER; $FORMATTER --version

FILES=`find ../app ../cmake ../core -name \*.cmake* -print -o -name \CMakeLists.txt -print`

for FILE in $FILES; do
    echo "$FILE"
    cp $FILE $FILE.orig
    cmake-lint -c cmake_format_config.py --in-place $FILE
    cmp -s $FILE.orig $FILE 
    if [ $? -ne 0 ]; then
        echo "Changed $FILE" >&2
        RETURN=1
        diff -u $FILE.orig $FILE >&2
	    rm $FILE.orig
    else
        rm $FILE.orig
        echo "Unchanged $FILE" >&2
    fi
done

exit $RETURN

cd $PWD
