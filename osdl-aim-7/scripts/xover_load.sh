#!/bin/sh
# Script to run multiple iteration of the test
#sized for a very small machine
# TEMPORARY
export PATH=/usr/local/aim3/bin:$PATH


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
# ./run.single.sh $1


# If problems
# 
iostat 30 1000 > $OUT/10.iostat & 
 clearprof
reaim -s 10 -e 10 -r 5 > $OUT/run.10.out
 getprof
 mkdir $OUT/10user
 mv ./multiuser.*.ss $OUT/10user
 mv ./multiuser.*.csv $OUT/10user
 mv /tmp/reaim.log $OUT/run.10.log
TIME=`/bin/date +%H%M_%S`
echo "run.10 finished at $TIME" >> $OUT/profiles/load.times
killall -9 iostat

NPROCS=`grep bogomips /proc/cpuinfo | wc -l`
SCALE=$(( NPROCS * 8 ))

 iostat 60 1000 >$OUT/qconv.iostat &
 clearprof
reaim -s $SCALE -i 2 -c -q -t -r 5 > $OUT/run.qconv.out
 getprof
 mkdir $OUT/qconv
 mv ./multiuser.*.ss $OUT/qconv
 mv ./multiuser.*.csv $OUT/qconv
 mv /tmp/reaim.log $OUT/run.qconv.$i.log
TIME=`/bin/date +%H%M_%S`
echo "run.qconv finished at $TIME" >> $OUT/profiles/load.times
killall -9 iostat

PEAK=`tail -2 $OUT/run.qconv.out | grep -v Crossover | awk '{print $1}'`

NU_ST=$(( PEAK * 2 ))
INC=$(( PEAK / 2 )) 
 iostat 60 1000 > $OUT/q2conv.iostat &
 clearprof
reaim -s $NU_ST  -i $INC -c -t -r 3 > $OUT/run.q2conv.out
 getprof
 mkdir q2conv
 mv ./multiuser.*.ss $OUT/q2conv
 mv ./multiuser.*.csv $OUT/q2conv
 mv /tmp/reaim.log $OUT/run.q2conv.log
TIME=`/bin/date +%H%M_%S`
echo "run.q2conv finished at $TIME" >> $OUT/profiles/load.times
killall -9 iostat

 iostat 60 1000 > $OUT/q4conv.iostat &
 clearprof
reaim -s $NU_ST  -i $INC -c -r 3 > $OUT/run.q4conv.out
 getprof
 mkdir q4conv
 mv ./multiuser.*.ss $OUT/q4conv
 mv ./multiuser.*.csv $OUT/q4conv
 mv /tmp/reaim.log $OUT/run.q4conv.log
TIME=`/bin/date +%H%M_%S`
echo "run.q4conv finished at $TIME" >> $OUT/profiles/load.times
killall -9 iostat

PEAK=`tail -2 $OUT/run.q2conv.out | grep -v Crossover | awk '{print $1}'`
NU_ST=$(( PEAK - 2  ))
reaim -s $NU_ST  -e $NU_ST -r 3 > $OUT/run.q3conv.out
 mkdir q3conv
 mv ./multiuser.*.ss $OUT/q3conv
 mv ./multiuser.*.csv $OUT/q3conv
 mv /tmp/reaim.log $OUT/run.q3conv.log
INC=$(( INC / 3 )) 


# This is sized by cpu 
SCALE=$(( NPROCS * 50 ))

 iostat 60 1000 >$OUT/bconv.iostat &
 clearprof
reaim -s $SCALE -i $INC -c -r 2 > $OUT/run.bigconv.out
 getprof
 mv ./multiuser.*.ss $OUT/
 mv ./multiuser.*.csv $OUT/
 mv /tmp/reaim.log $OUT/run.bigconv.$i.log
TIME=`/bin/date +%H%M_%S`
echo "run.bigconv finished at $TIME" >> $OUT/profiles/load.times
killall -9 iostat

mv $MYHOME/results/*.prof $OUT/profiles

