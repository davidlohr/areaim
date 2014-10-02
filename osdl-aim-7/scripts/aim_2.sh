#!/bin/sh
# This shell script is used by multiuser to test shell commands.
# The current user load is passed in as a parameter and can be 
# accessed as $1

#				' @(#) aim_2.sh:1.3 1/22/96 00:00:00'

if [ $# -lt 1 ]; then
	exit 0
	fi

if [ $1 -lt 10 ]; then
	sleep 5
else
	sleep 1
	fi
exit 0
