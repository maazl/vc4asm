#!/usr/bin/perl
use strict;

sub do_test($$)
{ my ($cmd, $rms) = @_;
  my @result = `./hello_fft.bin $cmd` or die "Failed to execute hello_fft: $?\n";
  print "$cmd ... ";
  my $count;
  foreach (@result)
  { #print "$_\n";
    /rel_rms_err\s*=\s*(\S+),/ or next;
    $1 <= $rms or die "FFT returned wrong RMS error $1, expected $rms\n";
    ++$count;
  }
  $count or die "No reasonable result retunred by execution of FFT.\n";
  print "OK\n";
}

do_test "8 2 2", 3.05839e-7;
do_test "9 2 2", 4.32761e-7;
do_test "10 2 2", 4.90194e-7;
do_test "11 2 2", 5.55598e-7;
do_test "12 2 2", 6.17574e-7;
do_test "13 2 2", 7.99032e-7;
do_test "14 2 2", 8.84898e-7;
do_test "15 2 2", 9.49359e-7;
do_test "16 2 2", 9.97415e-7;
do_test "17 2 2", 1.06804e-6;
do_test "18 2 2", 1.13211e-6;
do_test "19 2 2", 1.38829e-6;
do_test "20 2 2", 1.44965e-6;
do_test "21 2 2", 1.48539e-6;
do_test "22 1 2", 1.53800e-6;
