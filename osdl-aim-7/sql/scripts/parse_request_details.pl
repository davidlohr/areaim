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

my $file_in;
my $file_out;
my $db_conf;

GetOptions(
	   "infile=s" => \$file_in,
	   "outfile=s" => \$file_out,
	   "db_config=s" => \$db_conf
);

unless ( $file_in ) { die "no file $!"; }

print "Input file is $file_in\n" if DEBUG;

my $request = new FileHandle;
my %details;
my $ky;
my $vl;

unless ( $request->open( "< $file_in" ) ) {
	die "Bad file";
}
while ( <$request> ) {
	chomp;
	if ( m/^$/ ) { next; }
	s/^\s+//;
	if ( m/^REQUEST/ ) { next; }
	if ( m/^Sysctl / ) {
		( $ky, $vl ) = split / /;
	} else {
		( $ky, $vl ) = split /:/;
	}
	$vl =~ s/^\s+//;
	$details{ $ky } = $vl;
	}
$request->close;

if ( DEBUG ) {
my $foo;
my $bar;
foreach $foo ( keys(%details) ) {
	unless( $foo ) { print "no foo\n"; next; }
	unless( $details{ $foo } ) { print "no details\n"; next; }
	print "Hi |$foo| is $details{ $foo }\n";
	}
}

# Fill out the details in a handy form. 
# Make the insert a bit more readable

my $stp_id = $details{ 'Request ID' };
my $kernel_name = $details{ 'Kernel Name' };

unless ( ( $stp_id ) && ( $kernel_name ) ) { die "Bad data $!"; }

my $distro_name = $details{ 'Distro Name' } || 'unknown';
my $kernel_id = $details{ 'Kernel ID' } || '0';
my $host_name = $details{ 'Host Name' } || 'unknown';
my $lilo = $details{ 'LILO opt' } || 'default';
my $sysctl = $details{ 'Sysctl' } || 'none';
my $env = $details{ 'Environment' } || 'none';

my $params = $details{ 'Script Params' } || 'Default';

my $workload = "new_dbase";
my $filesys = "ext3";
my $prof = "no";

# go with these defaults unless
# otherwise found - we use defaults if error
unless (( $params eq 'Default' ) || ( $params eq 'DEFAULT' )) {
  my @plist = split / /, $params;

  for ( my $i = 0; $i <= $#plist; $i++ ) {
    if ( $plist[ $i ] eq '-p' ) {
      $prof = "yes";
    } elsif ( $plist[ $i ] eq '-w' ) {
      $workload = $plist[ $i + 1 ] || 'new_dbase';
    } elsif ( $plist[ $i ] eq '-f' ) {
      $filesys = $plist[ $i + 1 ] || 'ext3';
    }
  }
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
