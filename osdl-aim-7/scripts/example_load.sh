#!/bin/sh
# Script to run multiple iteration of the test
#sized for a very small machine


clearprof () {
        # /usr/sbin/readprofile -m /boot/System.map -r
        readprofile -n -m /boot/System.map -r
        }

getprof () {
        TIMES=`/bin/date +%H%M_%S`
        readprofile -n -m /boot/System.map -v | sort -grk3,4 > $MYHOME/results/$TIMES.prof
}

MYHOME=`pwd`
mkdir -p $MYHOME/results

if [ $# -eq 1 ]; then
	ANS=$1
else 
	ANS="example_run"
fi
OUT="$MYHOME/results/$ANS"
mkdir -p $OUT
mkdir -p $OUT/profiles
cp workfile $OUT

trap 'echo test starting '  10 16 30
# This might cause problems if using fork or exec tests
./run.single.sh $1


# If problems
# 
iostat 30 1000 > $OUT/10.iostat & 
 clearprof
reaim -s 10 -e 10 -r 5 > $OUT/run.10.out
 getprof
 mv ./multiuser.*.ss $OUT/
 mv ./multiuser.*.csv $OUT/
 mv /tmp/reaim.log $OUT/run.10.log
TIME=`/bin/date +%H%M_%S`
echo "run.10 finished at $TIME" >> $OUT/profiles/load.times
killall -9 iostat

 iostat 60 1000 > $OUT/100.iostat &
 clearprof
reaim -s 100 -e 100 -r 3 > $OUT/run.100.out
 getprof
 mv ./multiuser.*.ss $OUT/
 mv ./multiuser.*.csv $OUT/
 mv /tmp/reaim.log $OUT/run.100.log
TIME=`/bin/date +%H%M_%S`
echo "run.100 finished at $TIME" >> $OUT/profiles/load.times
killall -9 iostat
 done

NPROCS=`grep bogomips /proc/cpuinfo | wc -l`
SCALE=$(( NPROCS * 8 ))

 iostat 60 1000 >$OUT/qconv.iostat &
 clearprof
reaim -s $SCALE -i 2 -c -q -t -r 5 > $OUT/run.qconv.out
 getprof
 mv ./multiuser.*.ss $OUT/
 mv ./multiuser.*.csv $OUT/
 mv /tmp/reaim.log $OUT/run.qconv.$i.log
TIME=`/bin/date +%H%M_%S`
echo "run.qconv finished at $TIME" >> $OUT/profiles/load.times
killall -9 iostat
 done

# This is sized by cpu 
SCALE=$(( NPROCS * 50 ))

 iostat 60 1000 >$OUT/bconv.iostat &
 clearprof
reaim -s $SCALE -i 4 -c -r 2 > $OUT/run.bigconv.out
 getprof
 mv ./multiuser.*.ss $OUT/
 mv ./multiuser.*.csv $OUT/
 mv /tmp/reaim.log $OUT/run.bigconv.$i.log
TIME=`/bin/date +%H%M_%S`
echo "run.bigconv finished at $TIME" >> $OUT/profiles/load.times
killall -9 iostat
 done

mv $MYHOME/results/*.prof $OUT/profiles

