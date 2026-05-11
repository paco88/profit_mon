#!/bin/sh

pgrep -f profit_mon.py >/dev/null && pkill -9 -f profit_mon.py && sleep 3

/usr/bin/python3 -u ./profit_mon.py >profit_mon.log 2>&1 &
