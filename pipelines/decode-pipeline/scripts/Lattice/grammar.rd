# IMPORTANT: If you make changes to this file, you MUST recompile
#            the Lattice::Parser module like this:
#
# perl -MParse::RecDescent - grammar.rd Lattice::Parser
#
# See https://nlg0.isi.edu/projects/sbmt/wiki/LatticeBnf
# Grammar in Parse::RecDescent to parse lattices
#
{
    use Carp;
    use Lattice::Node;
    use Lattice::Edge;
    use Lattice::Block;		
    use Lattice::Graph;

    sub c_unescape($) {
        my $s = shift;
	$s = substr($s,1,-1);		# strip quotes
	$s =~ s{\\([\\""])}{$1}g;	# unescape backslash-escapes
	$s;
    }
}
#
# start of grammar
# Treat any C/C++/Perl comment as whitespace inside grammar (skip expr.)
#
lattices: <skip: qr{\s*(?:(?:/[*].*?[*]/|(?://|\#)[^\n]*)\s*)*}s> 
	  lattice(s?)
	{
	    # last entry is an array reference to what we want
	    $return = pop(@item);
	}
lattice: 'lattice' feature(s?) '{' ( edge | block | vertex )(s?) '}' ';'
	{
	    $return = defined $item[2] && @{$item[2]} ? 
		Lattice::Graph->new( FEATS => { ( map { @{$_} } @{$item[2]} ) } ) :
		Lattice::Graph->new();

	    # add array of node/edge/block items
	    $return->add_item( @{$item[4]} );
	    1;
	}
block: 'block' feature(s?) '{' ( edge | block )(s?) '}' ';'
	{
	    $return = defined $item[2] && @{$item[2]} ?
		Lattice::Block->new( FEATS => { ( map { @{$_} } @{$item[2]} ) } ) :
		Lattice::Block->new();

	    # add any number of items in the list
	    $return->add_item( @{$item[4]} );
	    1;
	}
edge: '[' uint ',' uint ']' c_str(?) feature(s?) ';'
	{ 
	    die "ERROR: $item[4] <= $item[2]\n" if $item[4] <= $item[2]; 
	    my @argv = ( START => $item[2], FINAL => $item[4] );
	    push( @argv, 'LABEL', $item[6][0] ) if @{$item[6]};
	    push( @argv, 'FEATS', { ( map { @{$_} } @{$item[7]} ) } )
		if ( defined $item[7] && @{$item[7]} );
	    $return = Lattice::Edge->new( @argv );
	}
vertex: '[' uint ']' c_str(?) feature(s?) ';'
	{
	    my @argv = ( INDEX => $item[2] );
	    push( @argv, 'LABEL', $item[4][0] ) if @{$item[4]};
	    push( @argv, 'FEATS', { ( map { @{$_} } @{$item[5]} ) } )
		if ( defined $item[5] && @{$item[5]} );
	    $return = Lattice::Node->new( @argv );
	}
feature: /[a-z][-_a-z0-9]*/i '=' ( c_str | c_float | nlp_float )
	{
	    $return = [ $item[1], $item[3] ];
	}
c_float: /[-+]?(\d+\.?|\d*\.\d+)([eE][-+]?\d+)?/
nlp_float: /(10|[eE])\^[-+]?(\d+\.?|\d*\.d\+)/
uint: /[0-9]+/
c_str: { c_unescape(extract_delimited($text,'"')) }
