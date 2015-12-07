# Jonathan Graehl - "jonathan#graehl!org" =~ tr/#!/@./

=begin twiki

---++ Description


   For web browser debug output, &webHeadersDone must be called after your HTTP
   headers are printed, or else you won't see anything.

    Debug routines with custom printed representations depending on argument
    types.  Configurable output by default to STDERR (if the DEBUG environment
    variable is set) or to HTML (if the SCRIPT_NAME var contains "debug").  For
    HTML output, prints a default response header unless you have already done
    so yourself (and indicated it with &webHeadersDone).

=cut

package WebDebug;

use strict;
#BEGIN { $diagnostics::PRETTY = 1 } 
#use diagnostics;

use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $WEBDEBUG $SCRIPTURL $REQUESTURI $BROWSERDEBUG $WRITELNDRIVER $WEBHEADERSDONE $SHELLDEBUG $DEBUGDEFAULTON @WEBBUFFER %DEBUGONPACKAGE %DEBUGTABLE);

$VERSION     = 1.00;
@ISA         = qw(Exporter);
@EXPORT      = qw();
@EXPORT_OK   = qw(&init &debug &debugAlways &debugOn &debugOff &debugging &debugDefaultOn &debugDefaultOff &debugDump &setDumperFor &webHeadersDone &webBoxedQuote &debugWriteLn &installWriteLnDriver);
%EXPORT_TAGS = ( all => \@EXPORT_OK,
		 );


# =========================
=pod

---++ Functions: Debug messages and value inspection

=cut


=pod

---+++ &init

called once on module load, but can be repeated for new web requests in a
mod_perl like environment.  blows away &debugOn, &installWriteLnDriver, etc.

=cut

sub init
{
 $SCRIPTURL=exists $ENV{SCRIPT_NAME} ? $ENV{SCRIPT_NAME} : '';
 $REQUESTURI=exists $ENV{REQUEST_URI} ? $ENV{REQUEST_URI} : '';
 $WEBDEBUG=($REQUESTURI =~ /debug/ || (exists($ENV{REMOTE_USER}) && $ENV{REMOTE_USER} =~ /Test/));
 $BROWSERDEBUG=$WEBDEBUG; # && exists $ENV{REMOTE_HOST};
eval("use CGI;use CGI::Carp qw(fatalsToBrowser warningsToBrowser);") if $BROWSERDEBUG;
# eval'use CGI;print &CGI::header(),&CGI::start_html(),&webBoxedQuote($REQUESTURI);';
 $WRITELNDRIVER=undef;
 $WEBHEADERSDONE=0;
 $SHELLDEBUG=exists $ENV{DEBUG} || ($WEBDEBUG && !$BROWSERDEBUG);

 @WEBBUFFER=();
 &debugDefaultOn;
 %DEBUGONPACKAGE=();
}

&initDump;

&init;

=pod

---+++ &webBoxedQuote($text)

returns HTML for displaying boxed blockquoted verbatim (fully escaped) text as
it would appear in a plain terminal

=cut

sub webBoxedQuote {
    my($message) = @_;
    my $escapeH=eval '\\&CGI::escapeHTML';
    my $esc=defined($escapeH) ? $escapeH->($message) : $message;
    return <<__EOF;
<table width = 100% border=0 cellspacing=0 cellpadding=1><tr><td bgcolor=black>
<table width = 100% border=0 cellspacing=0 cellpadding=5><tr><td bgcolor=white>
    <pre>$esc</pre>
</td></tr></table>
</td></tr></table>
__EOF
}



=pod

---+++ &installWriteLnDriver

takes a reference to code to be called with any debug text (even if it is also displayed in HTML or to console)

=cut


sub installWriteLnDriver {
    $WRITELNDRIVER=$_[0];
}


=pod

---+++ &debugWriteLn

calls writeDebug (appends message to data/debug.txt).  if the scriptname
contains 'debug', then messages are also output as HTML.  if the $DEBUG
environment var is set and the script is run from the command line the message
is output to STDERR as well.  also passes through argument to any user supplied WriteLnDriver

=cut


sub debugWriteLn
{
    if ($BROWSERDEBUG) {
	my $out=&webBoxedQuote;
	if ($WEBHEADERSDONE) {
	    print $out;
	} else {
#	    print &CGI::header(),&CGI::start_html(&CGI::script_name()." - debug mode");
	    push @WEBBUFFER,$out;
	}
    } elsif ($SHELLDEBUG) {
	print STDERR $_[0],"\n";
    }
    &$WRITELNDRIVER if (defined $WRITELNDRIVER);    
}


=pod

---+++ &webHeadersDone

For web browser debug output, must be called after your HTTP headers are printed, or else you won't see anything.

=cut

sub webHeadersDone {
    $WEBHEADERSDONE=1;
    eval("&warningsToBrowser(1);") if $BROWSERDEBUG;
    print $_ for (@WEBBUFFER);
    @WEBBUFFER=();
}

=pod

---+++ &setDumperFor($argtype,$handler)

$argtype is a reference to an object, and $handler is the new &debugDump handler
for obtaining printed representations of objects of that type

=cut

sub setDumperFor {
    my ($argtype,$handler) = @_;
    $DEBUGTABLE{$argtype} = $handler;
}

=pod

---+++ &debugDump($datum)

$datum is an object (not a reference to it) - a printed representation of the
object's value is returned

=cut

sub debugDump {
    my $argtype = ref($_[0]) || '';
    my $handler = $DEBUGTABLE{$argtype};
#     if (!$handler) {
# 	foreach my $anc ( ancestors($argtype) ) {
# 	    $handler = $DEBUGTABLE{$anc};
# 	    next unless $handler;
# 	    $DEBUGTABLE{$argtype} = $handler;
# 	    last;
# 	}
#     }
    return "unknown<$argtype>($_[0])" unless $handler;
    return $handler->(@_);
}

sub initDump {
%DEBUGTABLE=();

setDumperFor ''
    => sub { defined($_[0]) ? qq{"$_[0]"} : 'undef' };

setDumperFor "REF"
    => sub { '\\('.&debugDump(${$_[0]}).')' };

setDumperFor "SCALAR"
    => sub { '\\('.&debugDump(${$_[0]}).')' };
#    => sub { $_[0].'='.&debugDump(${$_[0]}) };

setDumperFor "ARRAY"
    => sub {
	my @arrayreps=map {&debugDump($_)} @{$_[0]};
	'[' . join(',',@arrayreps) . ']'
    };

setDumperFor "HASH"
    => sub { 
	my @entryreps=map {&debugDump($_) . "=>".&debugDump($_[0]->{$_})} keys(%{$_[0]});
	'{' . join(',',@entryreps) . '}'
    };

setDumperFor "CODE"
  => sub { "$_[0]" };

setDumperFor "GLOB"
  => sub { "GLOB:$_[0]" };
}

=pod

---+++ &debugAlways(...)

same as &debug(...) but always active, even if &debugOff was called

=cut

sub debugAlways {
    my @args=map { &debugDump($_) } @_;
    my ($package, $filename, $line) = caller;
    $filename = $1 if $filename =~ m|/([^/]+)$|;
    my $dbg="[$package]$filename($line): ".join('; ',@args);
    &debugWriteLn($dbg);
}

sub setDebug {
    my ($package, $filename, $line) = caller;
    $DEBUGONPACKAGE{$package} = $_[0];
}

=pod

---+++ &debugOn

enables debug output for the calling package

=cut
sub debugOn {
    my ($package, $filename, $line) = caller;
    $DEBUGONPACKAGE{$package} = 1;
}

=pod

---+++ &debugOff

disables debug output for the calling package

=cut
sub debugOff {
    my ($package, $filename, $line) = caller;
    $DEBUGONPACKAGE{$package} = 0;
}


=pod
---+++ &debugDefaultOn

sets default to &debugOn

=cut
sub debugDefaultOn {
    $DEBUGDEFAULTON = 1;
}

=pod
---+++ &debugDefaultOff

sets default to &debugOff

=cut
sub debugDefaultOff {
    $DEBUGDEFAULTON = 0;
}

=pod

---+++ &debugging($package)

returns true if debug output is enabled in the package (defaults to calling package) 
(else returns false, duh)

=cut

sub debugging
{
  my ($package, $filename, $line) = caller;
  $package = $_[0] if defined($_[0]);
  return (exists $DEBUGONPACKAGE{$package}) ? $DEBUGONPACKAGE{$package} : $DEBUGDEFAULTON;
}

=pod

---+++ &debug(...)

dumps the values of the arguments to the debug display (debugWriteLn) if &debugOn was called

=cut

sub debug {
    my ($package, $filename, $line) = caller;
    if (&debugging($package)) {
	my @args=map { &debugDump($_) } @_;
	$filename = $1 if $filename =~ m|/([^/]+)$|;
	my $dbg="[$package]$filename($line): ".join('; ',@args);
	&debugWriteLn($dbg);
    }
}


1;
