# $fh=openz($filename)
# &argvz; <>;
# exec_filter('cat',"a\nb") eq "a\nb"
# &cleanup would kill the current exec_filter process - naturally this assumes only one runs at a time per process (the global filehandles FW/FR require this as well)


my $DEBUG=$ENV{DEBUG};
sub debug {
    print STDERR join(',',@_),"\n" if $DEBUG;
}

use IPC::Open2;

my $cleanup_pid=undef;
my $waitpid_pid=undef;

sub cleanup {
    kill $cleanup_pid if defined $cleanup_pid;
}

#WARNING: returns list - don't use in scalar context blindly
sub exec_filter_list
{
    my ($program,@input)=@_;
    my $f_pid = open2(*FR, *FW, $program);
    die unless $f_pid;
    $waitpid_pid=$cleanup_pid=$f_pid;
    binmode FW;
    print FW $_ for (@input);
    close FW;
    binmode FR, ":utf8";
    my @output = <FR>;
    &debug("Output from $program",@output);
    close FR;
    $cleanup_pid=undef;
    return @output;
}

sub exec_filter
{
    my @list=&exec_filter_list;
    local $,='';
    return "@list";
}

sub exec_exitvalue
{
    die unless defined($waitpid_pid);
    my $ret=waitpid($waitpid_pid,0);
    $waitpid_pid=undef;
    return $ret;
}

#usage: $fh=openz($filename); while(<$fh>) ...
sub openz {
    my ($file)=@_;
    my $fh;
    if ($file =~ /gz$/) {
        open $fh,'-|','gunzip','-c',$file or die "can't gunzip -c $file: $!"; 
    } elsif ( $file =~ /bz2$/) {
        open $fh,'-|','bunzip2','-c',$file  or die "can't bunzip2 -c $file: $!"; 
    } elsif ( $file =~ /^(http|ftp|gopher):/) {
        open $fh,'-|','GET',$file  or die "can't GET $file: $!"; 
    } elsif ( $file eq '-') {
        return \*STDIN;
    } else {
        open $fh,$file or die "can't read $file: $!";
    }
    return $fh;
}

sub openz_out {
    my ($file)=@_;
    my $fh;
    if ($file =~ /gz$/) {
        open $fh,"|gzip -c >'$file'" or die "can't gzip -c > '$file': $!"; 
    } elsif ( $file =~ /bz2$/) {
        open $fh,"|bzip2 -c > '$file'"  or die "can't bzip2 -c > '$file': $!"; 
    } elsif ( $file =~ /^(http|ftp|gopher):/) {
        die "upload to URL not implemented";
    } elsif ( $file eq '-') {
        return \*STDOUT;
    } else {
        open $fh,'>',$file, or die "can't write $file: $!";
    }
    return $fh;
}

#usage: &argvz; while(<>) ...
sub argvz() {
    foreach (@ARGV) {
        $_ = "gunzip -c \"$_\" |" if /gz$/;
        $_ = "bunzip2 -c \"$_\" |" if /bz2$/;
        $_ = "GET \"$_\" |" if /^(http|ftp|gopher):/;
    }
}

1;
