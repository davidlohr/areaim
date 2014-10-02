#!/bin/sh


tput clear
cat << END
This is setup information for the re-aim workload. 
There are two files which control the workload. 

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

echo "The program is installed by defaul at /usr/local/bin workfiles are at /usr/local/share/reaim"

echo "hit any key"
read ANS
