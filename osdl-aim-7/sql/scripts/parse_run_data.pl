#!/usr/bin/perl -w 
#
use constant DEBUG => 1;

# This script reads the csv files generated by the reaim run 
# and creates a set of records to be inserted into a database
# Default with no options spews the inserts to stdout.
# The database table looks like:
# desc run_data;
#+-------------+-------------+------+-----+---------+----------------+
#| Field       | Type        | Null | Key | Default | Extra          |
#+-------------+-------------+------+-----+---------+----------------+
#| forks       | int(10)     |      | MUL | 0       |                |
#| jpm         | float       |      |     | 0       |                |
#| jpm_child   | float       |      |     | 0       |                |
#| jps_child   | float       |      |     | 0       |                |
#| parent_time | float       |      |     | 0       |                |
#| child_usec  | float       |      |     | 0       |                |
#| child_ssec  | float       |      |     | 0       |                |
#| std_dev     | float       |      |     | 0       |                |
#| jti         | float       |      |     | 0       |                |
#| max_child   | float       |      |     | 0       |                |
#| min_child   | float       |      |     | 0       |                |
#| pass_num    | int(5)      | YES  | MUL | NULL    |                |
#| run_exit    | varchar(20) | YES  | MUL | NULL    |                |
#| stp_id      | int(20)     | YES  | MUL | NULL    |                |
#| uid         | int(20)     |      | PRI | NULL    | auto_increment |
#+-------------+-------------+------+-----+---------+----------------+


use strict;
use Getopt::Long;
use Config::Simple;
use FileHandle;
use Data::Dumper;

use DBI;

my $file_in;
my $file_out;
my $db_conf;
my $pass_no;
my $stp_id;
my $run_t;
my $cpu;

GetOptions(
	   "infile=s" => \$file_in,
	   "outfile=s" => \$file_out,
	   "db_config=s" => \$db_conf,
	   "pass=i" => \$pass_no,
	   "stp_id=i" => \$stp_id,
	   "run=s" => \$run_t,
	   "cpu=i" => \$cpu

);

unless ( $file_in ) { die "no file $!"; }
unless ( $stp_id ) { die "no run number given $!"; }

print "Input file is $file_in\n" if DEBUG;

unless ( $pass_no ) { $pass_no = 1; }
unless ( $run_t ) { $run_t = 'max'; }
my $rdat = new FileHandle;
my @ln;
my $fh;
my $sql;
my $dbh;
my $sth;

unless( $rdat->open( "<$file_in" ) ) { die "Bad input file $!"; }

if ( $db_conf ) {
  my $config = new Config::Simple( $db_conf );
  my $user = $config->param( 'reaim_user' );
  my $dsn = $config->param( 'reaim_dsn' );
  my $pw = $config->param( 'reaim_password' );

  $dbh = DBI->connect( $dsn, $user, $pw );
  unless ( $dbh ) { die "Can't connect to database $!"; }
} # We will connect to the database here, insert in the loop 
# This disconnect will be done at the end of the loop 

my $t_name = "run_data_stp_" . $cpu . "_CPU";

while ( <$rdat> ) {
  chomp;
  unless ( m/^\d/ ) { next; }
  @ln = split/,/;
# Now we print, write a file and/or input to the database
  if ( $file_out ) {
    $fh = new FileHandle;
    unless ( $fh->open( ">>$file_out" ) ) { die "can't open $file_out $!"; }
    print $fh "INSERT INTO $t_name ( forks, jpm, jpm_child, jps_child, parent_time, child_usec, child_ssec, std_dev, jti, max_child, min_child, pass_num, run_exit, stp_id ) VALUES ( $ln[0],$ln[1],$ln[2], $ln[3], $ln[4], $ln[5], $ln[6], $ln[7], $ln[8], $ln[9], $ln[10], $pass_no, '$run_t', $stp_id );\n"
  } elsif ( ! $db_conf ) { 
    print  "INSERT INTO $t_name ( forks, jpm, jpm_child, jps_child, parent_time, child_usec, child_ssec, std_dev, jti, max_child, min_child, pass_num, run_exit, stp_id ) VALUES ( $ln[0],$ln[1],$ln[2], $ln[3], $ln[4], $ln[5], $ln[6], $ln[7], $ln[8], $ln[9], $ln[10], $pass_no, '$run_t', $stp_id );\n"
    

  }
  if ( $dbh ) {
    $sql =  "INSERT INTO $t_name ( forks, jpm, jpm_child, jps_child, parent_time, child_usec, child_ssec, std_dev, jti, max_child, min_child, pass_num, run_exit, stp_id ) VALUES ( $ln[0],$ln[1],$ln[2], $ln[3], $ln[4], $ln[5], $ln[6], $ln[7], $ln[8], $ln[9], $ln[10], $pass_no, '$run_t', $stp_id )";
    $sth = $dbh->prepare( $sql );
    $sth->execute();
    $sth->finish;
  }

}

if ( $file_out ) {
  $fh->close;
}

if ( $dbh ) {
  $dbh->disconnect();
}

