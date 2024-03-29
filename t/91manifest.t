#!perl -w
use strict;
use ExtUtils::Manifest qw(maniread fullcheck manifind maniskip);
use Test::More;
use File::Spec;
use Cwd qw(getcwd);

$ENV{AUTOMATED_TESTING} || $ENV{IMAGER_AUTHOR_TESTING}
  or plan skip_all => "manifest only tested under automated or author testing";

my $base_mani = maniread();
my @base_mani = keys %$base_mani;

my $have = maniread();
my @have = sort keys %$have;
my $skipchk = maniskip();
my @expect = grep !$skipchk->($_), sort keys %{+manifind()};
unless (is_deeply(\@have, \@expect, "check manifind vs MANIFEST")) {
  diag "Have:";
  diag "  $_" for @have;
  diag "Expect:";
  diag "  $_" for @expect;
}

done_testing();
