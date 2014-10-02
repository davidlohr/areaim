#! /usr/bin/perl

package parse_config;

#use Cwd 'fast_abs_path';
use File::Basename;
use Exporter;
our @ISA = 'Exporter';
our @EXPORT = qw($disk_entries @disks @diskdirectories @ramdisk @testdir
                 @fssize @fstype @mkfsopts @mountopts %allowed_file_systems
                 $config_line_num @config_line %disk_hash %diskdir_hash);

$disk_entries = 0;
%allowed_file_systems = (
	'vxfs'  => 1,    # HP-UX
	'ext2'  => 2,    # Linux
	'ext3'  => 3,    # Linux
	'ext4'  => 4,    # Linux
	'hfs'   => 5,    # HP-UX
	'advfs' => 6,    # HP-UX
	'ramfs' => 7     # Linux
);

# return the absolute path that points to the support scripts
# and configuration directories - ending with '/'
sub mk_full_scriptpath {
    # outside of support scripts dir
    return  dirname(__FILE__);
}

sub parse_disk_configuration_file {
	my ($filename) = @_;
	my ($opt_f) = $_[1];
	my $return_status = 1;
	my $line_no = 1;
	my $full_line;
	my $bad_format;
	my $fullpath; 

	if (!$opt_f) {
	    $fullpath = mk_full_scriptpath."/".$filename; 
	} else {
	    $fullpath = $filename;
	}

	$fd = open CFILE, $fullpath ;#mk_full_scriptpath."/".$filename;
	if (!$fd) {
		print STDERR "Open of $fullpath for read failed!\n";
		return 0;
	}
	$disk_entries = 0;
	while (<CFILE>) {
		s/\s*$//;
		$bad_format = 0;
		@line = ();
		$full_line = $_;
		if (/^#/) {
			$line_no++;
			next;
		}
		if (/^\s*$/) {
			$line_no++;
			next;
		}
		s/^\s*//;
		$i = 0;
		while (!/^$/) {
			if (/^'/) {
				if ( ! /^'[^']*?'\s|^'[^']*?'$/ ) {
					printf STDERR "Bad format on line $line_no: unmatched \'.\n";
					printf STDERR "(Quoted strings must begin with whitespace then \' or \" and end with a\n";
					printf STDERR "corresponding \' or \" and whitespace or end-of-line.)\n";
					/^('.*?\s)$/;
					$line[$i++] = $1;
					$_ = "";
					$bad_format = 1;
				} else {
					/^('[^']*?')(.*)$/;
					$line[$i++] = $1;
					$_ = $2;
					s/^\S*//;
				}
			} elsif (/^"/) {
				if ( ! /^"[^"]*?"\s|^"[^"]*?"$/ ) {
					printf STDERR "Bad format on line $line_no: unmatched \".\n";
					printf STDERR "(Quoted strings must begin with whitespace then \' or \" and end with a\n";
					printf STDERR "corresponding \' or \" and whitespace or end-of-line.)\n";
					/^(".*)$/;
					$line[$i++] = $1;
					$_ = "";
					$bad_format = 1;
				} else {
					/^("[^"]*?")(.*)$/;
					$line[$i++] = $1;
					$_ = $2;
					s/^\S*//;
				}
			} else {
				/^(\S+)(.*)$/;
				$line[$i++] = $1;
				$_ = $2;
			}
			s/^\s*//;
		}
		$size = $#line + 1;
		if ($size < 6) {
			print STDERR "Bad format on line $line_no: fewer than 6 elements in $full_line.\n";
			$bad_format = 1;
		}
		if ($bad_format) {
			print STDERR "Offending line ($size fields):\n\t";
			
			print STDERR "$line[0]";
			for ($i = 1; $i < $size; $i++) {
				print STDERR "|$line[$i]";
			}
			print STDERR "\n";
			$return_status = 0;
		} else {
			$disk = $line[0];
			$diskdirectory = $line[1];
			$lramdisk = $line[2];
			$ltestdir = $line[3];
			$lfssize = $line[4];
			$lfstype = $line[5];
			if ($size > 6) {
				$lmkfsopts = $line[6];
			} else {
				$lmkfsopts = "";
			}
			if ($size > 7) {
				$lmountopts = $line[7];
			} else {
				$lmountopts = "";
			}
			$disks[$disk_entries] = $disk;
			$diskdirectories[$disk_entries] = $diskdirectory;
			$ramdisk[$disk_entries] = $lramdisk;
			$testdir[$disk_entries] = $ltestdir;
			$fssize[$disk_entries] = $lfssize;
			$fstype[$disk_entries] = $lfstype;
			$mkfsopts[$disk_entries] = $lmkfsopts;
			$mountopts[$disk_entries] = $lmountopts;
			if ( $disk !~ /^\// ) {
				print STDERR "Bad format on line $line_no: Disk entry must begin with \"/\".\n";
				$bad_format = 2;
			}
			$disk_hash_entry = $disk_hash { $disk };
			if ( $disk_hash_entry != 0 ) {
				print STDERR "Bad entry on line $line_no: Apparent duplicate disk entry \"$disk\".\n";
				$bad_format = 2;
			} else {
				$disk_hash { $disk } = ($disk_entries+1);
			}
			$diskdir_hash_entry = $diskdir_hash { $diskdirectory };
			if ( $diskdir_hash_entry != 0 ) {
				print STDERR "Bad entry on line $line_no: Apparent duplicate disk directory entry \"$diskdirectory\".\n";
				$bad_format = 2;
			} else {
				$diskdir_hash { $diskdirectory } = ($disk_entries+1);
			}
			if ( $diskdirectory !~ /^\// ) {
				print STDERR "Bad format on line $line_no: Disk directory entry must begin with \"/\".\n";
				$bad_format = 2;
			}
			if ( $lramdisk !~ /^ram$|^io$/ ) {
				print STDERR "Bad format on line $line_no: disk type \"$lramdisk\" must be either \"ram\" or \"io\".\n";
				$bad_format = 2;
			}
			if ( $ltestdir !~ /^test$|^nontest$/ ) {
				print STDERR "Bad format on line $line_no: test specifier \"$ltestdir\" must be either \"test\" or \"nontest\".\n";
				$bad_format = 2;
			}
			if ( $lfssize !~ /^[0-9]+$/ ) {
				if ( $lfssize !~ /^full$/ ) {
					print STDERR "Bad format on line $line_no: disk size \"$lfssize\" must be a\n\tnumeric field (KBytes) or the word \"full\" (ramdisks must be numeric).\n";
					$bad_format = 2;
				}
			}
			$fs_lookup = $allowed_file_systems{$lfstype};
			if ( ! $fs_lookup ) {
				print STDERR "Bad format on line $line_no: \"$lfstype\" is not a known file system type.\n";
				$bad_format = 2;
			}
			if ($bad_format == 2) {
				print STDERR "Offending line ($size fields):\n\t";
			
				print STDERR "$line[0]";
				for ($i = 1; $i < $size; $i++) {
					print STDERR "|$line[$i]";
				}
				print STDERR "\n";
				$return_status = 0;
			}
			if ($bad_format == 0) {
				$disk_entries++;
			}
		}
		if ($verbose) {
			print STDERR "Processed line:\n\t$full_line\n\t";
			print STDERR "$line[0]";
			for ($i = 1; $i < $size; $i++) {
				print STDERR "|$line[$i]";
			}
			print STDERR "\n";
		}
		$line_no++;
	}
	close CFILE;
	return $return_status;
}

sub parse_reaim_configuration_file {
	my ($filename) = @_;
	my $return_status = 1;
	my $line_no = 1;
	my $full_line;
	my $bad_format;

	$config_line_num = 0;
	$fd = open CFILE, $filename;
	if (!$fd) {
		print STDERR "Open of reaim configuration file \"$filename\" for read failed!\n";
		return 1;
	}
	while (<CFILE>) {
		s/\n$//;
		$config_line[$config_line_num++] = $_;
	}
	close CFILE;
	return 1;
}
