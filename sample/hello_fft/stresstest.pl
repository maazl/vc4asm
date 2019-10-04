#!/usr/bin/perl
use strict;

my $ret = 0;
sub do_test($$)
{ my ($cmd, $rms) = @_;
  my @result = `./hello_fft -- $cmd 2>&1` or die "Failed to execute hello_fft: $?\n";
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
  $count or $ret |= 2, warn "No reasonable result retunred by execution of FFT: @result\n";
  print "$rmsret - done\n";
}

do_test "-8 32768 2", 1E-5;
do_test "-9 16384 2", 1E-5;
do_test "-10 8192 2", 1E-4;
do_test "-11 4096 2", 1E-4;
do_test "-12 2048 2", 1E-4;
do_test "-13 1024 2", 1E-4;
do_test "-14 512 2", 1E-4;
do_test "-15 256 2", 1E-3;
do_test "-16 128 2", 1E-3;
do_test "-17 64 2", 1E-3;
do_test "-18 32 2", 1E-3;
do_test "-19 16 2", 1E-3;
do_test "-20 8 2", 1E-3;
do_test "-21 4 2", 1E-3;
do_test "-22 2 2", 1E-2;

exit $ret;
