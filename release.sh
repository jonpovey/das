#!/bin/sh
set -e

if [ $# -lt 1 ]; then
  echo "need n.n version argument"
  exit 1
fi

VER=$1
DIR=das-v$VER

make && make WINDOWS=1
rm -rf $DIR
mkdir -p $DIR
cp README.md $DIR/README.md.txt
cp das $DIR
strip $DIR/das
tar zcf $DIR.tar.gz $DIR
rm $DIR/das
cp das.exe $DIR
zip -9 -r -q $DIR.zip $DIR
echo done $DIR
