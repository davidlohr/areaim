#!/bin/sh

# Script to do the re-aim setup
# cliffw OSDL 4/3/2003

INS_DIR=`pwd`
TMPCONF="tmp.reaim.config"

if [ ! -f "./reaim.config" ]; then
	echo "BASE_DIR $INS_DIR" > $TMPCONF
fi


cat << END
This is the setup script for the re-aim workload. 
There are two files which control the workload. 
If you are running 'make install' and just don't want to deal with this
now, you'll have a chance to exit without doing a configuration. 

The 'reaim.config' file holds some basic information for the test setup. 
'#' works for comments. Items are described by:
NAME <value>
First, the base diretory, ( --prefix arg to configure ) 
BASEDIR <path>

Next is a list of disk directories which
will be used for temporary files by the pipe,link and disk tests.
DISKDIR <path>
If the config file is empty, the current working directory will be used
for the tests. Maximum directories is currently #define limited to 256
Please be sure the directories exist before running the workload. 

The 'workload' file contains the FILESIZE and POOLSIZE parameters which govern
the distribution of io tasks, and a weighted lists of tests. We provide multiple
sample workload files, and use a symlink to create the actual 'workload' file
The workload file can also be specified with the -f (--file ) option at runtime.

The default location for the configuration files is /usr/local/share/reaim/. 
The default location for the program is /usr/local/bin/reaim. 

The "aim_*.sh" scripts are run by the re-aim tests. The other scripts in this directory
are samples of wrapper scripts. 


END

printf "Want to skip configuration and exit now? (y/n) ? [n] : "
read ANS
case "$ANS" in
y|Y|yes|YES)
	printf "Checklist:\n"
	printf "'reaim.config' or -l (other file) for disk dirs (dummy config required if no disk)\n"
	printf "'workfile' or -f (other file) to run test list\n"
	printf "'aim_*.sh' scripts in current working dir if running shell_ or exec_  tests\n"
	printf "'results will be in *.ss files \n"
	printf "Complaints to cliffw@osdl.org\n"
	printf "Bye!\n"
	exit 0
	;;
n|N|no|NO)
	printf "Proceeding....\n"
	;;
	*)
	printf "I'll take that to mean no :)\n"
	;;
esac



if [  -f $INS_DIR/reaim.config ]; then 
	printf "Current config file contains: \n"
	cat $INS_DIR/reaim.config
	else 
		printf "Config file not found or empty\n"
	fi

printf  "Do you want to edit the \"config\" file now (y/n) ? [y] : "
read ANS
case "$ANS" in
n|N|no|NO)
        ;;
Q)
        exit 0
        ;;
"")
        vi $INS_DIR/reaim.config
        ;;
y|Y|yes|YES)
        vi $INS_DIR/reaim.config
        ;;
esac

if [ -h $INS_DIR/workfile ]; then
	printf "You are running setup, so we are deleting the old link. \n"
	rm -f $INS_DIR/workfile
	fi

printf "Current workfiles: \n"
declare -a wf
declare -i cnt
cnt=0
for i in `ls $INS_DIR/workfile.*`
do
	wf[$cnt]=`basename $i`
	printf "$cnt $i\n"
	cnt=$(( cnt + 1 ))
done

printf "Enter number of desired workfile or 'f' for other file: "
read ANS
if [ $ANS == 'f' ] ; then
	printf "Enter file name: "
	read ANS
	if [ -f $ANS ]; then 
		ln -s $ANS $INS_DIR/workfile
	else
		printf "$ANS does not exitst\n"
		exit 1
		
	fi
else 
	ln -s $INS_DIR/${wf[$ANS]} $INS_DIR/workfile
	fi

	
exit 0
