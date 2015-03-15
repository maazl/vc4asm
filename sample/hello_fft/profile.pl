#!/usr/bin/perl
use strict;

sub do_profile($)
{ my ($cmd) = @_;
  my @result = `./hello_fft.bin $cmd` or die "Failed to execute hello_fft: $?\n";
  print "$cmd ... ";
  shift @result; # remove first line
  my @time;
  foreach (@result)
  { /usecs\s*=\s*(\S+),/ or next;
    push @time, $1;
    /rel_rms_err\s*=\s*(\S+),/ or die "No rel_rms_err found in $_.\n";
    $1 > 0 && $1 < 1E-5 or die "RMS error out of range: $_\n";
  }
  @time or die "No reasonable result retunred by execution of FFT:\n", @result;

  # analyze
  @time = sort { $a <=> $b } @time;
  #print "@time\n";
  my $count = @time;
  my $med = $time[$#time/2];
  
  @time = grep $_ > .95*$med && $_ < 1.05*$med, @time;
  #print "@time\n";
  
  $med = int $#time/2;
  my $sum;
  $sum += $_ foreach @time;
  
  printf "drop %d, avg = %g, median = %g", $count - @time, $sum/@time, $time[$med];
  if (@time > 3)
  { my $dev = $time[$med+1] - $time[$med-1];
    printf ", dev. = %g = %.2g%%", $dev, 100 * $dev / $time[$med];
  }
  print "\n";
}

@ARGV = (map("$_ 1 20", (8..22)), map ("$_ 10 15", (8..19))) unless @ARGV;

do_profile $_ foreach @ARGV;

