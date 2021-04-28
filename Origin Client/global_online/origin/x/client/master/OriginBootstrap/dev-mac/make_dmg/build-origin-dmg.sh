#! /bin/bash
MYDIR="$(dirname "$0")"
test -f $MYDIR/Origin.dmg && rm $MYDIR/Origin.dmg
$MYDIR/create-dmg/create-dmg \
    --window-size 550 300 \
    --background $MYDIR/background@1x.tiff \
    --icon-size 128 \
    --volname "Origin" \
    --app-drop-link 410 130 \
    --icon "Origin" 140 130 \
    --volicon $MYDIR/originvol.icns \
    --move-hidden-files 140 370 \
    $MYDIR/Origin.dmg \
    $MYDIR/contents
