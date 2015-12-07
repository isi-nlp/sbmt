/* (C) Franz Josef Och; Information Science Institute / University of Southern California */
vector<string> substr(const vector<string>&r,int n,int l){
    return vector<string>(r.begin()+n,r.begin()+n+l+1);}

double computeBLEU(const vector<vector<string> >&REF,const vector<string>&HYP,int ngr,double&nhyp,double&nhypfound,int type){
  map<vector<string>,int> ref,hyp;
  vector<map<vector<string>,int> > refcount(REF.size());
  for(unsigned int k=0;k<REF.size();++k)
    for(int i=0;i<int(REF[k].size())-ngr;++i)
      {
	ref[substr(REF[k],i,ngr)]=1;
	refcount[k][substr(REF[k],i,ngr)]++;
	if( type==2 )
	  {
	    if( ngramWeights.count(substr(REF[k],i,ngr))==0 )
	      {
		cerr << "ERROR: ngram weight: " << substr(REF[k],i,ngr) << " is missing.\n";
	      }
	  }
      }
  for(int i=0;i<int(HYP.size())-ngr;++i)hyp[substr(HYP,i,ngr)]++;
  for(unsigned int k=0;k<REF.size();++k)
    for(map<vector<string>,int>::iterator i=ref.begin();i!=ref.end();++i)
      i->second=max(i->second,refcount[k][i->first]);
  if( int(HYP.size())>ngr )
    nhyp+=HYP.size()-ngr;
  for(map<vector<string>,int>::const_iterator hit=hyp.begin();hit!=hyp.end();++hit){
    if( type==2 )
      {
	nhypfound+=min(ref[hit->first],hit->second)* ngramWeights[hit->first];
	if( verbose>1&&min(ref[hit->first],hit->second)*ngramWeights[hit->first] )
	  cout << min(ref[hit->first],hit->second)*ngramWeights[hit->first] << " ; " << min(ref[hit->first],hit->second) << " " << ngr << "-grams = '"<<hit->first <<"'\n";
      }
    else
      {
	int n=min(ref[hit->first],hit->second);
	nhypfound+=n;
	if( n==0&&verbose==2 )
	  cerr << "ERROR: " << hit->first << endl;
      }
    if( verbose==2 && min(ref[hit->first],hit->second) )
      cout << "NGRAMM " << ngr << " " << hit->first << " " <<min(ref[hit->first],hit->second) << endl;
  }
  return (nhyp==0)?1.0:(nhypfound/(double)nhyp);
}
/*D computeBLEU(const vector<vector<string> >&REF,const vector<string>&HYP,int ngr,D&nhyp,D&nhypfound,int type){
  map<vector<string>,int> ref,hyp;
  vector<map<vector<string>,int> > refcount(REF.size());
  for(unsigned int k=0;k<REF.size();++k)
    for(int i=0;i<int(REF[k].size())-ngr;++i)
      {
	vector<string> x=substr(REF[k],i,ngr);
	ref[x]=1;
	refcount[k][x]++;
      }
  for(int i=0;i<int(HYP.size())-ngr;++i)
    hyp[substr(HYP,i,ngr)]++;
  for(unsigned int k=0;k<REF.size();++k)
    for(map<vector<string>,int>::iterator i=ref.begin();i!=ref.end();++i)
      i->second=max(i->second,refcount[k][i->first]);
  if( int(HYP.size())>ngr )
    nhyp+=HYP.size()-ngr;
  for(map<vector<string>,int>::const_iterator hit=hyp.begin();hit!=hyp.end();++hit){
    if( type==2 )
      {
	nhypfound+=min(ref[hit->first],hit->second)* ngramWeights[hit->first];
	if( verbose&&min(ref[hit->first],hit->second)*ngramWeights[hit->first] )
	  cout << ngramWeights[hit->first] << " info for each of " << min(ref[hit->first],hit->second) << " " << ngr << "-grams = '"<<hit->first <<"'\n";
      }
    else
      {
	nhypfound+=min(ref[hit->first],hit->second);
      }
    if( verbose==2 && min(ref[hit->first],hit->second) )
      cout << "NGRAMM " << ngr << " " << hit->first << " " <<min(ref[hit->first],hit->second) << endl;
  }
  return (nhyp==0)?1.0:(nhypfound/(D)nhyp);
  }*/
