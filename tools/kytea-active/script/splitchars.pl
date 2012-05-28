#!/usr/bin/perl

binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";

while(<STDIN>) {
    chomp;
    my @arr = map { s/([ \-\&\|\\\?])/\\$1/g; $_ } split(//);
    print "@arr\n";
}
