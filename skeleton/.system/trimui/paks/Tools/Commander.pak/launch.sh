#!/bin/sh
# /.system/trimui/paks/Tools/Commander.pak/launch.sh

cd $(dirname "$0")

HOME="$SDCARD_PATH"
./DinguxCommander &> "$LOGS_PATH/Commander.txt"