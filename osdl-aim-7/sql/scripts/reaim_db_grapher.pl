#!/usr/bin/perl -w

use strict;
use Getopt::Long;
use FileHandle;
use Data::Dumper;
use DBI;
use Pod::Usage;

# Need to compare a single run,
# Need to compare a kernel to a single run

# inputs
# kernel name - should trigger an average
# PLM id - could be a list?
# runs - stp run id
# CPU's necessary for table name
# outfile - used to build a few names
# Switches not coded for now
my $k_name;
my $plm;
my $runs;
my $wload;
my $cpus;
my $outf;
my $txt_sw;
my $html_sw;
my $fstype;

GetOptions(
            "kernel=s" => \$k_name,
            "plm=s"    => \$plm,
            "run=s"    => \$runs,
            "load=s"   => \$wload,
            "cpu=i"    => \$cpus,
            "text"     => \$txt_sw,
            "out=s"    => \$outf,
            "html"     => \$html_sw,
	    "fs=s"	=> \$fstype
);

# The output data
my @labels;
my @compare_list;

# Hardcoded db connection
my $user = 'reaim';
my $dsn  = "DBI:mysql:reaim_db:localhost";
my $pw   = 'reaim';

my $dbh = DBI->connect( $dsn, $user, $pw );
unless ( $dbh ) { die "Can't connect to database $!"; }

# Usage goop - TODO - separate run number only case

unless ( ( $outf ) && ( $cpus ) && ( $wload ) ) {
    print STDERR "Usage: Must specify all of:\n outfile ( -out )\n "
      . "Number of CPUS ( -cpu )\n Workload type ( -load )\n";
    exit 1;
}
unless ( ( $k_name ) || ( $plm ) || ( $runs ) ) {
    print STDERR "Usage: Must specify one or all of(comma-separated list):\n"
      . "Kernel Name(s) ( -kernel )\n"
      . "PLM ID(s) ( -plm )\n"
      . "STP Run ID(s) ( -run )\n";
    exit 1;
}

# Default if we don't use fstype
unless ( $fstype ) { $fstype = "NOT"; }
# Build database table name
my $tname = "run_data_stp_" . $cpus . "_CPU";
# Someday we may want to know the biggest
my $max_fork = 0;
# Push items onto list, push labels onto a list.
# Different functions depending on what data was
# supplied, same method... might generalize..
if ( $k_name ) {
    my @knames;
    if ( $k_name =~ m/./ ) {
        @knames = split /,/, $k_name;
    } else {
        push( @knames, $k_name );
    }

    my $kn;
    foreach $kn ( @knames ) {
        my $rref = get_kernel( $dbh, $kn, $wload, $tname, $fstype );
        if ( defined($rref->[0]{'avg'}) ) {
	  my $rmax = $#{ $rref };
	  if ( $rmax > $max_fork ) { $max_fork = $rmax; }
            push( @labels,       $kn );
            push( @compare_list, $rref );
        } else {
            print STDERR "Kernel $kn - no data\n";
        }
    }
}
if ( $plm ) {
    my @pnames;
    if ( $plm =~ m/,/ ) {
        @pnames = split /,/, $plm;
    } else {
        push( @pnames, $plm );
    }
    my $pn;
    foreach $pn ( @pnames ) {
        my $pref = get_plm( $dbh, $plm, $wload, $tname, $fstype);
        if ( defined($pref->[0]{'avg'}) ) {
	  my $pmax = $#{ $pref };
	  if ( $pmax > $max_fork ) { $max_fork = $pmax; }
            push( @labels,       "PLM_" . $pn );
            push( @compare_list, $pref );
        } else {
            print STDERR "PLM $pn - no data\n";
        }
    }

}
if ( $runs ) {
    my @rnames;
    if ( $runs =~ m/,/ ) {
        @rnames = split /,/, $runs;
    } else {
        push( @rnames, $runs );
    }
    my $rn;
    foreach $rn ( @rnames ) {
        my $sref = get_stpid( $dbh, $runs, $wload, $tname);
	  my $smax = $#{ $sref };
	  if ( $smax > $max_fork ) { $max_fork = $smax; }
        if ( defined($sref->[0]{'avg'}) ) {
            push( @labels,       "STP_" . $rn );
            push( @compare_list, $sref );
        } else {
            print STDERR "STP $rn - no data\n";
        }
    }

}

# Build the gnuplot file with the boilerplate
my $gpname = $outf . "errbars.input";
my $othname = $outf. ".input";
my $GH = new FileHandle;
unless ( $GH->open( ">$gpname" )) { die "Bad file open $!"; }
print $GH "set title \"Reaim Compare - Jobs per Minute vs Child Processes\"\n";
print $GH "set data style lines\n";
print $GH "set grid xtics ytics\n";
my $H = new FileHandle;
unless ( $H->open( ">$othname" )) { die "Bad file open $!"; }
print $H "set title \"Reaim Compare - Jobs per Minute vs Child Processes\"\n";
print $H "set data style lines\n";
print $H "set grid xtics ytics\n";

# Add a line to the .input file for each item,
# Add a data file for each item.
# A little string magic
my $outstr = "plot ";
my $otstr = "plot ";
my $maxy = 0;
for ( my $i = 0; $i <= $#labels; $i++ ){
  my $FH = new FileHandle;
  my $fname = $outf . "." . $labels[ $i ] . ".data";

  unless ( $FH->open( ">$fname" )) { die "Bad file open $!"; }
  $outstr .= " \"$fname\" using 1:2:3 with yerrorbars title \"$labels[ $i ]\",";
  $otstr .= " \"$fname\" using 1:2 title \"$labels[ $i ]\",";
  my $ro = $compare_list[ $i ]; # copy the ref back out
  my $av;
  my $ss;
  my $n = $#{$ro};
  for ( my $j = 0; $j <= $n; $j++ ) {
    ( $av = $ro->[ $j ]{ 'avg' } ) =~ s/,//;
    ( $ss = $ro->[ $j ]{ 'std' } ) =~ s/,//;
    if ( $av > $maxy ) { $maxy = $av };
    print $FH "$ro->[ $j ]{'forks'}  $av  $ss\n";
  }
  $FH->close();
}
# Set the scale factor
my $yrange = $maxy + ( $maxy * 0.1 );

chop $outstr;
print $GH "$outstr\n";
print $GH "set yrange [0:$yrange]\n";
print $GH "set xlabel \"Children Forked\"\n"
  . "set ylabel \"Jobs per Minute\"\n"
  . "set term png medium \nset output \"$outf.eb.png\"\nreplot\n";

$GH->close();
chop $otstr;
print $H "$otstr\n";
print $H "set yrange [0:$yrange]\n";
print $H "set xlabel \"Children Forked\"\n"
  . "set ylabel \"Jobs per Minute\"\n"
  . "set term png medium \nset output \"$outf.png\"\nreplot\n";

$H->close();
`gnuplot $gpname`;
`gnuplot $othname`;
exit;



=head1 FUNCTION get_kernel

Input: a string identifiying a kernel
Output: an array of hashes containing the data

=cut

sub get_kernel {
    my ( $dbhand, $kname, $wload, $tname, $fstype ) = @_;

    my $sql = "SELECT "
      . "FORMAT(AVG(rd.jpm),2) AS avg, rd.forks AS forks, "
      . "FORMAT(STD(rd.jpm),4) AS std "
      . "FROM $tname rd, run_summary rs "
      . "WHERE rs.workload = '$wload' AND rs.stp_id = rd.stp_id "
      . "AND rs.kernel_name LIKE '$kname' ";
   	unless ( $fstype eq "NOT" ) {
   	$sql .= "AND rs.filesystem = '$fstype' ";
	}
      $sql .= "GROUP BY rd.forks";

    my $sth = $dbh->prepare( $sql );
    $sth->execute();
    my $ref = $sth->fetchall_arrayref( {} );
    $sth->finish;
    return $ref;

}

sub get_plm {
    my ( $dbhand, $plm, $wload, $tname, $fstype ) = @_;

    my $sql = "SELECT "
      . "FORMAT(AVG(rd.jpm),2) AS avg, rd.forks AS forks, "
      . "FORMAT(STD(rd.jpm),4) AS std "
      . "FROM $tname rd, run_summary rs "
      . "WHERE rs.workload = '$wload' AND rs.stp_id = rd.stp_id "
      . "AND rs.kernel_id = $plm ";
   	unless ( $fstype eq "NOT" ) {
   	$sql .= "AND rs.filesystem = '$fstype' ";
	}
      $sql .= "GROUP BY rd.forks";

    my $sth = $dbh->prepare( $sql );
    $sth->execute();
    my $ref = $sth->fetchall_arrayref( {} );
    $sth->finish;
    return $ref;
}

sub get_stpid {
    my ( $dbhand, $stpid, $wload, $tname) = @_;

    my $sql = "SELECT "
      . "FORMAT(AVG(rd.jpm),2) AS avg, rd.forks AS forks, "
      . "FORMAT(STD(rd.jpm),4) AS std "
      . "FROM $tname rd, run_summary rs "
      . "WHERE rs.workload = '$wload' AND rs.stp_id = rd.stp_id "
      . "AND rs.stp_id = '$stpid' "
      . "GROUP BY rd.forks";

    my $sth = $dbh->prepare( $sql );
    $sth->execute();
    my $ref = $sth->fetchall_arrayref( {} );
    $sth->finish;
    return $ref;

}
