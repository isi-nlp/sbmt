#!/usr/usc/bin/perl

# transform input for POS file from PTB style to MT style

while($line = <>){
    chomp($line);
    @line = split " ", $line;
    @mtline = ();
    for($i=1, $mti=1; $i<=$#line; $i++){
	if( $i%2==0 ){ 
	    if( !$dashflag ){ $mtline[$mti++] = $line[$i]; }
	    else{ $dashflag = 0; }
	}
	else{
	    $dashflag = 0; 
	    if( $line[$i] =~ /^(\S+?)-(\S+?)$/ ){
		$lex = transformPTB2MT($line[$i], $i);
		@lex = split "-", $lex;
		for($j=0; $j<=$#lex; $j++){
		    $mtline[$mti++] = $lex[$j];
		    $mtline[$mti++] = $line[$i+1]; # keep the original POS
		    if( $j<$#lex ){
			$mtline[$mti++] = "\@-\@";
			$mtline[$mti++] = $line[$i+1]; 
		    }
		    $dashflag = 1;
		}
		next;
	    }
	    $mtline[$mti] = transformPTB2MT($line[$i], $i);
	    $mti++;
	}
    }
    $mtline[0] = ($mti-1)/2;
    $mtline = join " ", @mtline;
    print "$mtline\n";
}

sub transformPTB2MT{
    my ($mtline, $i) = @_;

    $mtline =~ s/-LRB-/\(/g;    
    $mtline =~ s/-RRB-/\)/g;    
    $mtline =~ s/-LCB-/\{/g;    
    $mtline =~ s/-RCB-/\}/g;
    $mtline =~ s/''/\"/g;
    $mtline =~ s/``/\"/g;
    if( $i==1 ){
	$mtline =~ s/--/\*/;
    }
    $mtline = lc($mtline);
    return $mtline;
}
