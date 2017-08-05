#!/usr/bin/perl
use strict;

die "usage: vc4asm -V sourcfile.qasm | checkOutput.pl sourcefile.qasm\n" unless @ARGV;

my %expect; # expected warnings and errors: $expect{$file}{$line} = Warning|Error|Ok
my ($file, $line, $type, $id, $msg);

my $fail;
sub fail($)
{	my $msg = shift;
	chomp $msg;
	print "$file ($line): $msg\n";
	++$fail;
}

sub checkMsg()
{	my $expect = \$expect{$file}{$line};
	#print "$file ($line): $type - $$expect\n";
	if (defined $$expect)
	{	fail "unexpected message of type $type, expected $$expect\n$msg" if $$expect !~ s/$type// && $type !~ /^I/;
	} else
	{	fail "unexpected error\n$msg" if $type =~ /^[EF]/;
	}
}

# read expected result
foreach (@ARGV)
{	$file = $_;
	open F, "<$file" or die "Failed to open $file: $?\n";
	$line = 0;
	while (<F>)
	{	++$line;
		/#([\w\.:]+)/ or next;
		$expect{$file}{$line} = $1;
		#print "$file ($line): $1\n";
	}
	close F;
}

# compare against vc4asm output
while (<STDIN>)
{	$msg .= $_;
	($file, $line, $type, $id) =
		/^([^:]+) \((\d+)(?:,\d+)?\): ([IWEF])\w+: (\w\d+(?:\.\d+)?)/ or next;
	#$file =~ s/.*[\/\\]//;
	$type = "$type:$id";
	$msg = $_;
	checkMsg if $file;
}

foreach (sort keys %expect)
{	$file = $_;
	foreach (sort { $a <=> $b } keys %{$expect{$file}})
	{	$line = $_;
		$type = $expect{$file}{$line};
		fail "expected message of type $type" if $type && $type ne 'O';
	}
}

exit $fail;
