#!/bin/sh
# Simple sample script to run test in a loop
# Test is run 5 time with output in results directory

clearprof () {
        /usr/sbin/readprofile -m /boot/System.map -r
        }

getprof () {
        TIMES=`/bin/date +%H%M_%S`
        /usr/sbin/readprofile -m /boot/System.map -v | sort -grk3,4 > $MYHOME/results/$TIMES.prof
}

MYHOME=`pwd`
mkdir $MYHOME/results

trap 'echo Test Starting'  16 10 30
 for i in 1 2 3 4 5 
 do
 clearprof
./reaim -s 10 -e 10 > $MYHOME/results/run.10.$i.out
 getprof
 mv ./reaim.ss $MYHOME/results/run.10.$i.ss
 mv /tmp/reaim.log $MYHOME/results/run.10.$i.log
 done
