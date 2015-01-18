#!/usr/bin/perl
use strict;

sub do_test($$)
{ my ($cmd, $rms) = @_;
  my @result = `./hello_fft.bin $cmd` or die "Failed to execute hello_fft: $?\n";
  print "$cmd ... ";
  my $count;
  foreach (@result)
  { /rel_rms_err\s*=\s*(\S+),/ or next;
    $1 == $rms or die "FFT returned wrong RMS error $1, expected $rms\n";
    ++$count;
  }
  $count or die "No reasonable result retunred by execution of FFT.\n";
  print "OK\n";
}

do_test "8 2 2", 3.05839e-7;
do_test "9 2 2", 4.7795e-7;
do_test "10 2 2", 5.56503e-7;
do_test "11 2 2", 7.91228e-7;
do_test "12 2 2", 2.56654e-6;
do_test "13 2 2", 5.02574e-6;
do_test "14 2 2", 8.7084e-6;
do_test "15 2 2", 1.04876e-5;
do_test "16 2 2", 2.08061e-5;
do_test "17 2 2", 7.98968e-5;
do_test "18 2 2", 9.3954e-5;
do_test "19 2 2", 1.67001e-4;
do_test "20 2 2", 3.32664e-4;
do_test "21 2 2", 6.65217e-4;

