#!/usr/bin/perl -w
# Written by Nathan Dabney
# OSDL
# Stolen by Cliff White
# OSDL :)

use strict;

my $debug = 0;
my $var_mark = 0.05;

die unless (scalar @ARGV > 1);

my $f1 = $ARGV[0];
my $f2 = $ARGV[1];

die "$f1 missing" unless ( -f $f1 );
die "$f2 missing" unless ( -f $f2 );

my %r1 = scan( $f1 );
my %r2 = scan( $f2 );

sub scan {
    my $file = shift;
    my %data;

    open (F, $file) || die "Cannot open $file: $!";

    while (<F>) {
        if ( /^(\d+)\s+(\S+)\s+\S+\s+\S+\s+\S+\s+(\S+)\s+(.+)$/ ) {
            print( "[debug $file] $1 [$2] $3 $4\n" ) if ( $debug );
            $data{ $1 }{ rate } = $3;
            $data{ $1 }{ desc } = $4;
            $data{ $1 }{ name } = $2;
        }
    }
    
    return %data;
}

my $name1 = $f1;
my $name2 = $f2;

$name1 =~ s/.+\///g;
$name2 =~ s/.+\///g;

print "--- Results Comparison Report for AIM9 ---\n\n";
print "Comparison of results of $name1 and $name2\n";
print "Trimming variances less than +/- $var_mark%\n\n";
print "Positive numbers mean $name2 is faster.\n\n";

print "   -- Variance --\n";
print "Operations   Percent \tDescription\n";
print "-"x79, "\n";

my $diff;
for ( sort { $r1{$a} <=> $r1{$b} } keys %r1 ) {
    my $rate1 = $r1{ $_ }{ rate };
    my $rate2 = $r2{ $_ }{ rate };
    my $desc =  $r1{ $_ }{ desc };
    
    $diff = $rate2 - $rate1;
    my $per = $rate2 / $rate1 - 1;
    
    next if ( ( $per > -$var_mark ) && ( $per < $var_mark ) );

    if ( $per < 0) {
        printf("%10d   %3.2f%%\t%s - %s\n", $diff, $per * 100 , $r1{ $_ }{ name },$r1{ $_ }{ desc } );
    } else { 
        printf("%10d    %3.2f%%\t%s - %s\n", $diff, $per * 100 ,  $r1{ $_ }{ name } ,$r1{ $_ }{ desc });
    }
}
