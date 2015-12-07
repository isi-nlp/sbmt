#!/usr/bin/env perl

use strict;
use warnings;
use Getopt::Long;
use POSIX;
Getopt::Long::Configure('no_ignore_case');

my $e_insertion_word_list;
GetOptions(
     'e-insertion-word-list|e=s' => \$e_insertion_word_list,
) || die "Wrong options!\n";

# escape the chars in feature names so that special chars wont confuse 
# the shell when weights are passed via command line to the decoder.
# it does lower casing as well.
sub convert_label {
    my $name=shift;
    $name=~s/-\d+-BAR$/_BAR/;
    $name=~s/-\d+$//;
    $name=~s/^\@//;
    $name=~s/^,/comma/;
    $name=~s/^:/colon/;
    #$root=~s/^;/semicolon/;
    $name=~s/^#/pound/;
    $name=~s/^``/lquote/;
    $name=~s/^''/rquote/;
    $name=~s/^\./period/;
    $name=~s/^\$/dollar/;
    $name=~s/-/_/g;
    # catch-all: transform non-alpha-numeric characters to numeric repr.
    # and ensure feature names are of form: [A-Za-z_][A-Za-z0-9_]*
    $name=~s/([^A-Za-z-0-9_])/"x".ord($1)."x"/eg;
    $name=~s/^(\d+)/_$1/;
    $name=lc($name);
    return $name;
}

sub convert_terminal {
    my $k=shift;
    $k=~s/-\d+$//g;
    $k=~s/,/comma/g;
    $k=~s/:/colon/g;
    $k=~s/;/semicolon/g;
    $k=~s/#/pound/g;
    $k=~s/``/lquote/g;
    $k=~s/"/rquote/g;
    $k=~s/'/squote/g;
    $k=~s/\@/at/g;
    $k=~s/\./period/g;
    $k=~s/\$/dollar/g;
    $k=~s/-/_/;
    $k=~s/\(/lparen/g;
    $k=~s/\)/rparen/g;
    # catch-all: transform non-alpha-numeric characters to numeric repr.
    # and ensure feature names are of form: [A-Za-z_][A-Za-z0-9_]*
    $k=~s/([^A-Za-z-0-9_])/"x".ord($1)."x"/eg;
    $k=~s/^(\d+)/_$1/;
    return $k;
}


# return the unsplit rule roots 
# Use a array as return value to return both split and non-split roots in future.
sub rule_root_features {
    my $root=shift;
    my $features=shift;
    $root=convert_label($root);
    push @{$features}, "${root}_rooted=1";

}

sub n_var_features {
    my $rhs=shift;
    my $features=shift;
    my $nvars= 0;
    for (split /\s+/, $rhs) {
        if(/^x\d+/){
            $nvars++;
        }
    }
    push @{$features}, "n_var=$nvars";
}

sub num_X_nodes_features {
    my $lhs=shift;
    my $features=shift;

    # delete all the lexical leaves 
    $lhs=~s/\("\S+?"\)//g;
    # put space before/after the )|(
    $lhs=~s/(\)|\()/ $1 /g;
    my %nt=();
    for (split /\s+/, $lhs){
        if(/^x\d+:/) { next;}
        if(not /^(\(|\))$/){ $nt{convert_label($_)}++; }
    }
    for(keys %nt){
        push @{$features}, $_ . "_nodes=" . $nt{$_};
    }
}

# only fires for non-lex rules. probably can be extended to all rules
# by looking at the word alignments.
sub e_insertion_features {
    my $einserts=shift;
    my $lhs=shift;
    my $rhs=shift;
    my $features=shift;

    # checks if this is a non-lex rule.
    if(scalar(grep { /^".*"$/ } split /\s+/, $rhs) == 0) {
        my %ws=();
        while($lhs=~/\("(\S+?)"\)/g){
            if(exists $einserts->{$1}){
                $ws{$1}++;
            }
        }
        for (keys %ws){
            my $k=$_;
            push @{$features}, convert_terminal($k) . "_insertion=" . $ws{$_}
        }
    }
}

sub count_features {
    my $count=shift;
    my $output=shift;
    my $x = 1;
    foreach my $c (split(/\s+/, $count)) { 
        # counts 1 - 4 are very close to weis count1, count2, count3to5, count6to10
        # except our count_category_4 is count6to11
        # and we include a count_category_5 = count12toinfinity
	if ($c > 0) { 
	    $c=ceil(log($c)/log(2) + 0.5);
	    if($c > 5){ $c = 5;}
	    if($c < -5) { $c = -5;}
	    if ($x > 1) {
		my $idx = $x - 2;
		push @{$output}, "count_category_$c.$idx" . "=1";
	    } else {
		push @{$output}, "count_category_$c" . "=1";
            }
        }
        $x = $x + 1;
    }
}



# load the e-insertion word list.
my %einserts=();
if(defined($e_insertion_word_list)){
    open(F, "<$e_insertion_word_list") || 
        die "FATAL: cannot open file $e_insertion_word_list for reading:$!\n";
    while(<F>){
        s/[\015\012]+$//g;
        s/^\s+|\s+$//g;
        $einserts{$_}=1;
    }
}

while(<STDIN>){
    if(/^(([^\(]+)\(.*?) -> (.*?) ###(.*)/){
        my $lhs=$1;
        my $root=$2;
        my $rhs=$3;
        my $attr=$4;

        my $id;
        if($attr=~/ id=(\d+)/){ $id=$1;}
        my $count;
        if($attr=~/ count={{{(.*?)}}}/){ $count=$1;}

        my @features=();

        rule_root_features($root, \@features);
        n_var_features($rhs, \@features);
        num_X_nodes_features($lhs, \@features);
        if( defined($e_insertion_word_list)){
            e_insertion_features(\%einserts, $lhs, $rhs, \@features);
        }
        count_features($count, \@features);

        print $id . "\t" . join(' ', @features) . "\n";
    }
}



