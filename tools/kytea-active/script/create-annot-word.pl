#!/usr/bin/perl

use strict;
binmode STDOUT, ":utf8";
binmode STDERR, ":utf8";

if(@ARGV != 3) {
    print STDERR "Usage: merge-annot.pl train.part train.wordprob NUMBER > train.annot\n";
    exit(1);
}

my $SIZE = $ARGV[2];
my $STACK_SIZE = $SIZE*10;
my $STACK_LIMIT = 1000000;

my(@parts, @probs, @print, @ress);
print STDERR "Loading probabilities from $ARGV[1]\n";
my ($as, $bs, $ds, $line);
open PROB, "<:utf8", $ARGV[1] or die $!;
while($as = <PROB> and $bs = <PROB> and $ds = <PROB>) {
    chomp $bs; chomp $as;
    push @ress, $as;
    my @ba = split(/ /, $bs);
    for(0 .. $#ba) {
        my $score = $ba[$_];
        if($score < $STACK_LIMIT) {
            push @probs, [ $ba[$_], $line, $_ ];
            if(@probs > $STACK_SIZE*5) {
                @probs = sort { $a->[0] <=> $b->[0] } @probs;
                @probs = @probs[0 .. $STACK_SIZE-1];
                $STACK_LIMIT = $probs[-1]->[0];
                # print STDERR "Reduced stack, limit=$STACK_LIMIT\n";
            }
        }
    }
    $line++;
}
close PROB;

print STDERR "Loading corpus from $ARGV[0]\n";
open PART, "<:utf8", $ARGV[0] or die $!;
while(<PART>) {
    chomp;
    push @parts, $_;
}
close PART;

print STDERR "Annotating values\n";
@probs = sort { $a->[0] <=> $b->[0] } @probs;
my %added;
for(@probs) {
    last if keys(%added) >= $ARGV[2];
    my @arrtemp = split(/([ \?\|\-!\\])/, $parts[$_->[1]]);
    my (@arr, $bs);
    for(@arrtemp) {
        if(/\\/) { $bs = "\\"; }
        else { push @arr, $bs.$_; $bs=""; }
    }
    my @ca = split(/[^ ]/, $ress[$_->[1]]); shift @ca; pop @ca;
    # my $arr = $parts[$_->[1]];
    my $pos = $_->[2]*2 + 1;
    my $bigram = $arr[$pos-1].$arr[$pos+1];
    if($arr[$pos] eq ' ' and not $added{$bigram}) {
        $arr[$pos] = '!';
        for(my $p = $_->[2] - 1; $p >= 0 and $arr[$p*2 + 1] eq ' '; $p--) {
            if(length($ca[$p])) { $arr[$p*2 + 1] = '|'; last; }
            else { $arr[$p*2 + 1] = '-'; }
        }
        for(my $p = $_->[2] + 1; $p < @arr and $arr[$p*2 + 1] eq ' '; $p++) {
            if(length($ca[$p])) { $arr[$p*2 + 1] = '|'; last; }
            else { $arr[$p*2 + 1] = '-'; }
        }
        $parts[$_->[1]] = join('', @arr);
        $print[$_->[1]] = 1;
        $added{$bigram}++;
    }
}

for(0 .. $#print) {
    print "$_: $parts[$_]\n" if $print[$_];
}
