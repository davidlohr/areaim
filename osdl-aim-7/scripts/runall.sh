#!/bin/sh
# Script to run all the known aim tests with aim7 timing. 
# edit this if you add any tests :)


clearprof () {
        # /usr/sbin/readprofile -m /boot/System.map -r
        /usr/local/bin/readprofile_ap -m /boot/System.map -r
        }

getprof () {
        TIMES=`/bin/date +%H%M_%S`
        /usr/local/bin/readprofile_ap -m /boot/System.map -v | sort -grk3,4 > $MYHOME/results/$TIMES.prof
}

export MYHOME=`pwd`

trap 'echo Test Starting'  10 16 30
for i in add_double add_float add_long add_int add_short array_rtns brk_test dgram_pipe dir_rtns_1 div_double div_float div_long div_int div_short exec_test fork_test jmp_test link_test matrix_rtns mem_rtns_1 mem_rtns_2 misc_rtns_1 mul_double mul_float mul_long mul_int mul_short new_raph num_rtns_1 page_test pipe_cpy ram_copy series_1 shared_memory shell_rtns_1 signal_test sort_rtns_1 stream_pipe string_rtns tcp_test trig_rtns udp_test shell_rtns_2 shell_rtns_3 disk_brr disk_brw disk_brd disk_bwrt disk_bcp disk_rr disk_rw disk_rd disk_wrt disk_cp sync_disk_rw sync_disk_wrt sync_disk_cp sync_disk_update disk_src
do
	echo "FILESIZE 100k" > ./workfile
	echo "POOLSIZE 1M" >> ./workfile
	echo "10 $i" >> ./workfile

	clearprof
	./reaim -o
	mv singleuser.1.ss results/allsing/$i.single
	./reaim -s1 -e1 > results/multi1/$i.multi
	mv multiuser.1.ss results/multi1/$i.ss
	mv multiuser.1.csv results/multi1/$i.csv

	getprof
	TIMES=`/bin/date +%H%M_%S`
	echo "$i finished at $TIMES" >> results/load.times
done
for i in workfile.*
do
	./reaim -s5 -e5 -r 3 -f $i > results/multi5/$i.out
	mv multiuser.1.ss results/multi5/$i.ss
	mv multiuser.1.csv results/multi5/$i.csv
	TIMES=`/bin/date +%H%M_%S`
	echo "$i finished at $TIMES" >> results/load.times
done
