#!/usr/bin/perl
use strict;

# Work around for inability of cmake to generate Makefiles that ignore the result of a command.
system @ARGV;
