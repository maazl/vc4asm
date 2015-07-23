#!/usr/bin/perl
use strict;

my $ret = 0;
sub do_test($$)
{ my ($cmd, $rms) = @_;
  my @result = `./hello_fft.bin $cmd` or die "Failed to execute hello_fft: $?\n";
  print "$cmd ... ";
  my $count;
  my $rmsret;
  foreach (@result)
  { #print "$_\n";
    /rel_rms_err\s*=\s*(\S+),/ or next;
    $rmsret = $1;
    $rmsret <= $rms or $ret |= 1, warn "FFT returned wrong RMS error $1, expected $rms\n";
    ++$count;
  }
  $count or $ret |= 2, warn "No reasonable result retunred by execution of FFT.\n";
  print "$rmsret - done\n";
}

do_test "8 12000 2", 1E-5;
do_test "9 6000 2", 1E-5;
do_test "10 3400 2", 1E-4;
do_test "11 1800 2", 1E-4;
do_test "12 960 2", 1E-4;
do_test "13 480 2", 1E-4;
do_test "14 240 2", 1E-4;
do_test "15 120 2", 1E-4;
do_test "16 60 2", 1E-3;
do_test "17 32 2", 1E-3;
do_test "18 16 2", 1E-3;
do_test "19 8 2", 1E-3;
do_test "20 4 2", 1E-3;
do_test "21 2 2", 1E-3;
do_test "22 1 2", 1E-2;

exit $ret;
