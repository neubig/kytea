#!/usr/bin/perl

if(@ARGV != 2) {
    print STDERR "Usage: ./merge-annot.pl curr.part curr.annot > next.part\n";
    exit 1;
}

binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";

my %annot;
open ANNOT, "<:utf8", $ARGV[1] or die $!;
while(<ANNOT>) {
    chomp;
    /!/ and die "Annotation contains '!', please finish annotating: $_\n";
    /^(\d*): (.*)$/ or die "Badly Formatted Line: $_\n";
    $annot{$1} = $2;
    $2 =~ /([^ \?\|\-\&][^ \?\|\-\&])/ and die "Unmarked character bigram '$1' at $_\n";
    $2 =~ /[^\\]([ \?\|\-][ \?\|\-])/ and die "Double boundary '$1' at $_\n";
}
close ANNOT;

my $line = -1;
open PART, "<:utf8", $ARGV[0] or die $!;
while(<PART>) {
    if($annot{++$line}) {
        print "$annot{$line}\n";
    } else {
        print $_;
    }
}
close PART;
