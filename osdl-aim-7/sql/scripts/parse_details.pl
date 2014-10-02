#!/usr/bin/perl -w 
#
use constant DEBUG => 1;
# This script reads the STP request-details file and 
# turns it into a database insert
# Database is 
#  uid int(20) NOT NULL auto_increment,
#  stp_id  int(20) NOT NULL,
#  distro_name VARCHAR(20) default NULL,
#  kernel_id int(20) NOT NULL,
#  kernel_name VARCHAR(50) NOT NULL,
#  host_name VARCHAR(20),
#  host_group VARCHAR(20) NOT NULL default 'stp',
#  lilo VARCHAR(20) NOT NULL default 'default',
#  workload VARCHAR(20) NOT NULL default 'new_dbase',
#  filesystem VARCHAR(20) NOT NULL default 'ext3',
#  profile VARCHAR(20) NOT NULL default 'no',
#  sysctl VARCHAR(20) NOT NULL default 'default',
#  env VARCHAR(20) NOT NULL default 'none',
#
# Request-details file:
# REQUEST DETAILS:
# Request ID:  296321
# Test Name:   reaim
# Distro Name: RedHat 9.0
# Kernel ID:   3253
# Kernel Name: linux-2.6.8
# LILO opt:    profile=2

# REQUEST INFO:
# Host Name:   stp4-001
# Host State:  inst
# Script Params: -p
# Environment:  DEFAULT
# Sysctl DEFAULTS
#
use strict;
use Getopt::Long;
use Config::Simple;
use FileHandle;
use Data::Dumper;

use DBI;

my $stp_run;
my $file_out;
my $db_conf;

GetOptions(
	   "stp_run=s" => \$stp_run,
	   "outfile=s" => \$file_out,
	   "db_config=s" => \$db_conf
);

my %details;
my $ky;
my $vl;



my $dsn     = "DBI:mysql:STPDB:osdlab.pdx.osdl.net";
my $dsnuser = "robot";
my $dsnpass = "BrimEidetic#991";
my $dbh     = DBI->connect( $dsn, $dsnuser, $dsnpass );

my $ssql = "SELECT dt.descriptor AS distro, tr.lilo, tr.environment, tr.sysctl, h.descriptor AS host, "
	. "pt.descriptor AS patch, trpt.patch_tag_uid AS plm, tr.status "
	. "FROM test_request tr LEFT JOIN host h ON h.host_type_uid = tr.host_type_uid "
	. "LEFT JOIN test t ON t.uid=tr.test_uid "
	. "LEFT JOIN test_request_to_patch_tag trpt on trpt.test_request_uid = tr.uid "
	. "LEFT JOIN patch_tag pt ON trpt.patch_tag_uid = pt.uid "
	. "LEFT JOIN distro_tag dt ON tr.distro_tag_uid = dt.uid "
	. "WHERE tr.uid = $stp_run";

my $sth = $dbh->prepare( $ssql );
$sth->execute();
my $href = $sth->fetchall_arrayref( {} );
$sth->finish();

print Dumper $href if DEBUG;

my $stat = $href->[0]{'status'};

unless ( $stat eq 'Complete' ) { exit 1; }

my $stp_id = $stp_run;
my $kernel_name = $href->[0]{ 'patch' };
my $distro_name = $href->[0]{ 'distro' };
my $kernel_id = $href->[0]{ 'plm' };
my $host_name = $href->[0]{ 'host' };
my $lilo = $href->[0]{ 'lilo' } || 'default';
my $sysctl = $href->[0]{ 'sysctl' } || 'none';
my $env = $href->[0]{ 'environment' } || 'none';

my $workload = "new_dbase";
my $filesys = "ext3";
my $prof = "no";

$ssql = "select param_0, param_1, param_2 "
	. "from test_request_to_test_parameter where test_request_uid = $stp_run";

$sth = $dbh->prepare( $ssql );
$sth->execute();
my $pref = $sth->fetchall_arrayref( {} );
$sth->finish();

if ( $pref ) {
	$workload = $pref->[0]{ 'param_0' } || "new_dbase";
	$filesys  = $pref->[0]{ 'param_1' } || "ext2";
	if ( ( $pref->[0]{ 'param_2' }) && ( $pref->[0]{ 'param_2' } eq '1' )) {
		$prof = "yes";
	} else {
		$prof = "no";
	}
} else {
	$ssql = "SELECT * from test_request_to_parameter where test_request_uid = $stp_run";
	$sth = $dbh->prepare( $ssql );
	$sth->execute();
	my $pref = $sth->fetchall_arrayref( {} );
	if ( $pref ) {
		for ( my $i = 0; $i <= $#{$pref}; $i++ ) {
			if ( $pref->[ $i ]{ 'parameter_uid' } eq '3' ) {
				$workload =  $pref->[ $i ]{ 'parameter_value' };
			} elsif ( $pref->[ $i ]{ 'parameter_uid' } eq '4' ) {
				$filesys = $pref->[ $i ]{ 'parameter_value' };
			} elsif ( ( $pref->[ $i ]{ 'parameter_uid' } eq '8' ) && ( $pref->[ $i ]{ 'parameter_value' } eq '1' ) ) {
				$prof = "yes";
			}
		}
	}
}
# otherwise take the defaults
				
	
# my $params = $href->[0]{ 'Script Params' } || 'Default';

$dbh->disconnect;
if ( DEBUG ) {
	print "$stp_id\n$kernel_name\n$distro_name\n$workload\n$filesys\n";
}



if ( DEBUG ) { 
  print " OUT $prof $workload $filesys\n";
}

# TODO - all more stuff
my $fh;

if ( $file_out ) {
  $fh = new FileHandle;
  unless ( $fh->open( ">>$file_out" ) ) { die "can't open $file_out $!"; }
  print $fh "INSERT INTO run_summary ( stp_id, distro_name, kernel_id, kernel_name, host_name, host_group, lilo, workload, filesystem, profile, sysctl, env ) VALUES( $stp_id, '$distro_name', $kernel_id, '$kernel_name', '$host_name', 'stp', '$lilo', '$workload', '$filesys', '$prof', '$sysctl', '$env' );\n";
} elsif ( ! $db_conf )  {
  print "INSERT INTO run_summary ( stp_id, distro_name, kernel_id, kernel_name, host_name, host_group, lilo, workload, filesystem, profile, sysctl, env ) VALUES( $stp_id, '$distro_name', $kernel_id, '$kernel_name', '$host_name', 'stp', '$lilo', '$workload', '$filesys', '$prof', '$sysctl', '$env' );\n";
}

# if a configuration file for the database exists, insert directly
# We need
# reaim_dsn
# reaim_user
# reaim_password
# We will use the standard Config::Simple
#

if ( $db_conf ) {
  my $config = new Config::Simple( $db_conf );
  my $user = $config->param( 'reaim_user' );
  my $dsn = $config->param( 'reaim_dsn' );
  my $pw = $config->param( 'reaim_password' );

  my $dbh = DBI->connect( $dsn, $user, $pw );
  unless ( $dbh ) { die "Can't connect to database $!"; }

  my $sql = "INSERT INTO run_summary ( stp_id, distro_name, kernel_id, kernel_name, host_name, host_group, lilo, workload, filesystem, profile, sysctl, env ) VALUES( $stp_id, '$distro_name', $kernel_id, '$kernel_name', '$host_name', 'stp', '$lilo', '$workload', '$filesys', '$prof', '$sysctl', '$env' )";
  
  my $sth = $dbh->prepare( $sql );
  $sth->execute();
  $sth->finish;
  $dbh->disconnect();
}
