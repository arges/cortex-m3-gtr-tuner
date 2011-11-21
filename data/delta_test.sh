#!/bin/bash
# Written by Chris J Arges <christopherarges@gmail.com>

# print real time
TIMEFORMAT="%E"

stty 115200 time 0 min 1 < /dev/ttyUSB0
while true; do
	# get remote time, time will output real time between read
	time read -n 5 remote</dev/ttyUSB0
done 2>&1

