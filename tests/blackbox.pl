#!/usr/bin/perl -w

# DCPU-16 assembler black box testing
# Written by jon@leetfighter.com a.k.a. blueshift / cheese_magnet
#
# Designed for das (https://github.com/jonpovey/das) but maybe useful for other
# assemblers.
#
# Test assembly of various test files. Each input file can have associated
# binary, listing and console output reference files.
# If present, the reference files are checked against assembler
# output and diffs produced if they differ (a difference is a test failure).
#
# A report is generated with summary: tests passed/failed. This script returns
# 0 on success, else number of failed tests.
#
# BLACKBOX CONFIG
# ---------------
# A test directory can contain an optional blackbox.cfg file for things like
# assembler flags for this test.
# Config options:
#	; this is a config file comment, will be ignored
# 	DAS_FLAGS = flags	flags to pass to das when running this test
#	EXPECT_FAILURE		expect das to return nonzero when run on this file
#
# TODO: maybe EXPECT_NO_BINARY/_DUMP: error runs should not create these
#
# Example config file:
#	; le_long.s: this file tests little-endian output with no short literals
#	DAS_FLAGS = --le --no-short-literals
#
# Flags and options specific to other assemblers may be added later, e.g.
#	ORGANIC_FLAGS = --some-organic-flag

use strict 'vars';
use File::Path qw(make_path);
my $DEBUG = 0;

my $VER = "1.1";
my $ASSEMBLER = "das";
my $RESULTDIR = "results";
my $REPORT_PATH = "$RESULTDIR/blackbox-report.txt";
my $CONFIG_FILE = "blackbox.cfg";
my $OUT_BIN_FILE = "output.bin";
my $OUT_DUMP_FILE = "$ASSEMBLER.dump.txt";
my $OUT_CONSOLE_FILE = "$ASSEMBLER.console.txt";
my $SEPARATOR = "===============================================================\n";
my $DIFFCMD = "diff -u";
my $HEXDUMP_CMD = "hd -v";

# evaluated later
my %assemble_recipes;
$assemble_recipes{"das"} =
	'../das $test_extra_flags --no-dump-header -o $bin_path ' .
	'--dumpfile $dump_path $src_path >$console_path 2>&1';

my @all_tests;
my @tests;
my @failed_tests;
my $longest_test_name = 0;
my @report;
my @summary;

# for current test
my @failures;
my $test;
my @diff_output;	# globals? who, me?
my $expect_failure;
my $test_extra_flags;

sub dbg
{
	if ($DEBUG) {
		print @_;
	}
}

sub bail
{
	print @_;
	exit 1;
}

# make sorted list of all input test directories
# (any directory starting with a number)
# tests/	// run under this directory
#	001.operators/
#		operators.s
#		operators.bin
#		operators.das.dump.txt
#	002.short-literals/
#		...
sub get_tests
{
	my $dir = shift;
	my $failed = 0;
	@all_tests = ();
	opendir(my $dh, $dir) || die "failed reading directory $dir: $!";
	while (my $entry = readdir($dh)) {
		if (-d $entry && $entry =~ /^\d+/) {
			push @all_tests, $entry;
			my $tmp = length($entry);
			if ($tmp > $longest_test_name) {
				$longest_test_name = $tmp;
			}
		}
	}
	closedir($dh);
	@all_tests = sort @all_tests;

	# default no-args: run all tests
	if ($#ARGV == -1) {
		@tests = @all_tests;
		return;
	}

	# if test names/numbers supplied on command line, only run those.
	# use hashes to squash duplicates
	my %want_nums;
	my %want_names;
	my %runme;
	for my $test (@ARGV) {
		if ($test =~ /^\d+$/) {
			#dbg "$test looks like a number\n";
			$want_nums{int($test)} = 1;
		} else {
			#dbg "$test doesn't look like a number\n";
			$want_names{$test} = 1;
		}
	}

	# search @all_tests for specified tests by name and number.
	# would be O(faster) to sort want lists and walk once. but feh.
	for my $test (@all_tests) {
		# search by name
		for my $name (keys %want_names) {
			if ($test eq $name) {
				# remove from want, add to tests to run
				#dbg "Found $test by name\n";
				$runme{$test} = 1;
				delete $want_names{$name};
				last;
			}
		}
		# if test dir start with "nnn" also try search by number
		my $testnum;
		if ($test =~ /^(\d+)/) {
			#dbg "test num: $1\n";
			$testnum = $1;
		} else {
			next;	# skip number search
		}

		for my $num (keys %want_nums) {
			if ($testnum == $num) {
				# remove number from want, add name to tests to run
				#dbg "Found $test by number($num)\n";
				$runme{$test} = 1;
				delete $want_nums{$num};
				last;
			}
		}
	}
	if (keys %want_nums) {
		# didn't find some specified tests
		print("Can't find these test numbers: ",
			join(", ", sort keys %want_nums), "\n");
		$failed = 1;
	}
	if (keys %want_names) {
		print("Can't find these tests: '",
			join("', '", sort keys %want_names), "'\n");
		$failed = 1;
	}
	if ($failed) {
		bail("Abort\n");
	}
	@tests = sort keys %runme;
}

# sanity check the tests to run:
# - check the directories are readable
# - check they contain at least one asm source (.s, maybe .dasm)
# - check they contain at least one output test file
sub sanity_check_tests
{
	# LATER
}

# make/clear a directory, or fail script
sub make_clear_directory
{
	my $dir = shift;
	if (-e "$dir" && ! -d "$dir") {
		bail("$dir exists but is not a directory.. fix that\n");
	}
	if (-e "$dir") {
		# not portable, maybe later
		#dbg "I would now rm -rf $dir/*\n";
		if (system("rm -rf $dir/*") == 0) {
			#dbg "success\n";
		} else {
			bail("rm -rf $dir/* failed: $!\n");
		}
		return;
	} else {
		# make it
		#dbg "Create directory $dir\n";
		make_path($dir) || bail("Failed creating $dir: $!\n");
	}
}

sub is_src_filename
{
	$_ = shift;
	return (/\.s$/ || /\.dasm$/);
}

# Push a failure for the current test
# first arg is failure type, any further args are report lines.
sub test_failure
{
	my $failtype = shift;
	push @failures, $failtype;
	push @report, $SEPARATOR, "$test FAILURE: ", @_, "\n\n",
}

sub do_diff
{
	my $old = shift;
	my $new = shift;
	my $diffcmd = "$DIFFCMD --label reference $old --label new $new 2>&1";
	@diff_output = `$diffcmd`;
	#dbg "diff returned $?\n";
	return $?
}

sub do_hex_diff
{
	my $old = shift;
	my $new = shift;
	# $new is the path to the test output binary. can use it as a temporary
	# base filename (directory guaranteed to be emptyish)
	my $old_hex = "$new.good.hex";
	my $new_hex = "$new.bad.hex";
	# this is sort of hacky for now.
	# a more dcpu-ish dump with word offset values might be nice.
	if (system("$HEXDUMP_CMD $old > $old_hex")) {
		bail("Failed dumping hex for $old\n");
	}
	if (system("$HEXDUMP_CMD $new > $new_hex")) {
		bail("Failed dumping hex for $new\n");
	}
	my $diffcmd = "$DIFFCMD --label reference $old_hex " .
		"--label new $new_hex 2>&1 | head -20";
	@diff_output = `$diffcmd`;
	return $?
}

sub reset_test_config
{
	$expect_failure = 0;
	$test_extra_flags = "";
}

# read blackbox.cfg for a test
sub read_test_config
{
	my $cfgfile = shift;
	open(CFG, "<$cfgfile") or bail("Failed opening $cfgfile\n");
	while (<CFG>) {
		chomp;
		s/^(.*);.*$/$1/;		# strip assembler-style comments
		s/^\s*(.*?)\s*$/$1/;	# strip leading, trailing whitespace
		if (/^EXPECT_FAILURE$/) {
			dbg("expect failure\n");
			$expect_failure = 1;
		} elsif (/^DAS_FLAGS\s*=\s*(.*)$/) {
			$test_extra_flags = $1;
			dbg("extra flags: $1\n");
		} elsif (/\S+/) {
			bail("Unknown config '$_' in $cfgfile\n");
		} else {
			#dbg("No match for '$_'\n");
		}
	}
	close(CFG);
}

# for each test (this can be a perl function):
# create output directory named as input directory
# read blackbox.cfg options, if it exists
# run das from input directory. flags to output binary, dump and console to
# output directory.
# collect das return value and compare against EXPECT_FAILURE
# for reference files present (bin, dump, console) diff against outputs.
# for binaries, diff the hexdumps. write diffs to files in output dir
#
# if there are differences, write string summary of test failure to memory
# buffer. append diff to another, verbose memory buffer. add to counter of
# failed tests, for "nn/nnn tests failed" report heading
sub run_test
{
	$test = shift;
	@failures = ();
	my $outdir = "$RESULTDIR/$test";
	my @srcs = ();
	my $ref_bin = "";
	my $ref_dump = "";
	my $ref_console = "";

	my $testname = $test;
	$testname =~ s/^\d+\.?//;
	if ($testname eq "") {
		bail("Test directory $test should be named $test.testname\n");
	}
	make_clear_directory($outdir);
	reset_test_config();
	dbg "run test $test\n";

	# See what's in the test directory
	opendir(my $test_dh, $test) or bail("Failed opening directory $test: $!\n");
	while ($_ = readdir($test_dh)) {
		if ($_ eq $CONFIG_FILE) {
			read_test_config("$test/$CONFIG_FILE");
		} elsif (is_src_filename($_)) {
			#dbg "$_ is src\n";
			push @srcs, $_;
		} elsif ($_ eq $OUT_BIN_FILE) {
			$ref_bin = $_;
		} elsif ($_ eq $OUT_DUMP_FILE) {
			$ref_dump = $_;
		} elsif ($_ eq $OUT_CONSOLE_FILE) {
			$ref_console = $_;
		} else {
			#dbg "Ignore file $_\n";
		}
	}
	closedir($test_dh);

	if ($ref_bin eq "" && $ref_dump eq "" && $ref_console eq "") {
		print("Test $test: No reference output to test against\n",
			"(any of $OUT_BIN_FILE, $OUT_DUMP_FILE, $OUT_CONSOLE_FILE)\n");
		bail();
	}

	if (1 != scalar @srcs) {
		bail("test $test: only one source file supported so far\n");
	}

	my $bin_path = "$outdir/$OUT_BIN_FILE";
	my $dump_path = "$outdir/$OUT_DUMP_FILE";
	my $console_path = "$outdir/$OUT_CONSOLE_FILE";
	my $src_path = "$test/" . $srcs[0];
	my $runcmd = $assemble_recipes{'das'};
	$runcmd =~ s/(\$\w+)/$1/eeg;			# evaluate vars

	#dbg "run '$runcmd'\n";
	my $retcode = system($runcmd);
	$retcode >>= 8;

	# first, check for exit status mismatch
	if (!$retcode == !$expect_failure) {
		# got the expected result, success or failure
		dbg "success!\n";
	} else {
		# error, did not get the expected result
		dbg "assembler returned $retcode\n";
		test_failure("exit status", "Exit status was $retcode, expected ",
			$expect_failure ? "nonzero" : "0");
	}

	# check binary if there's a reference
	if ($ref_bin) {
		#dbg "have ref bin\n";
		my $ref_path = "$test/$ref_bin";
		$retcode = do_diff($ref_path, $bin_path);
		if ($retcode) {
			# FIXME diff hexdumps
			do_hex_diff($ref_path, $bin_path);
			test_failure("binary", "Binary output differs:\n", @diff_output);
		}
	}

	# check dump, if reference
	if ($ref_dump) {
		#dbg "have ref dump\n";
		my $ref_path = "$test/$ref_dump";
		$retcode = do_diff($ref_path, $dump_path);
		if ($retcode) {
			test_failure("dump", "Dump differs:\n", @diff_output);
		}
	}

	# check console, if reference
	if ($ref_console) {
		#dbg "have ref console\n";
		my $ref_path = "$test/$ref_console";
		$retcode = do_diff($ref_path, $console_path);
		if ($retcode) {
			test_failure("console", "Console output differs:\n", @diff_output);
		}
	}

	if (scalar @failures) {
		push @failed_tests, $test;
		push @summary, sprintf("  %-${longest_test_name}s : ", $test);
		push @summary, join ", ", @failures;
		push @summary, "\n";
		push @report, "$test cmdline was:\n$runcmd\n";
	}
}

# after all tests run, print summary and write report to file, something
# like tests/results/blackbox-results.txt. report is summary plus verbose
# diffs.
#
# report is something like
#
# Blackbox version xxx test report		// report file only
# Testing: das							// report file only
#
# 2 of 5 tests failed. Mismatches:
#   001.operators: listing, return value
#   003.foo:       binary, listing
#
# Failure diffs follow.
#
# Failure 001.operators: listing
# diff blah blah blah:
# [unified diff]

sub print_summary
{
	my $total_tests = scalar @tests;
	my $num_failed = scalar @failed_tests;
	if ($num_failed == 0) {
		unshift(@summary, "All $total_tests tests passed\n");
	} else {
		unshift(@summary, "$num_failed/$total_tests tests FAILED:\n");
	}
	print join "", @summary;
	print "Writing report to $REPORT_PATH\n";
}

sub write_report
{
	my $total_tests = scalar @tests;
	my $datestamp = localtime;
	open(RPT, ">$REPORT_PATH") or bail ("Failed writing $REPORT_PATH: $!\n");
	print RPT "Blackbox version $VER report generated $datestamp\n\n";
	print RPT join '', @summary;
	if (scalar @report) {
		print RPT "\nFull report:\n";
		print RPT join '', @report;
	}
	print RPT $SEPARATOR;
	print RPT "\nList of tests run ($total_tests):\n  ";
	print RPT join "\n  ", @tests;
	print RPT "\n";
	close RPT;
}

get_tests(".");
sanity_check_tests();
make_clear_directory($RESULTDIR);

foreach my $test (@tests) {
	run_test($test);
}

print_summary();
write_report();

exit (scalar @failed_tests);
