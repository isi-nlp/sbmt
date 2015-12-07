# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <sstream>

using namespace std;

std::ostream& print_no_internals(std::ostream& os, rule_data const& rd)
{
  bool print_preterm = false;
  bool first = true;
  os << 'X' << '(';
  for (size_t idx = 1; idx != rd.lhs.size(); ++idx) {
      lhs_node r = rd.lhs[idx];
      
      if (r.indexed) {
	  print_preterm = true;
	  if (not first) os << ' ';
          first = false;
	  os << 'x' << r.index << ':' << 'X';
      } else if(not r.children) {
	  if (not first) os << ' ';
	  first = false;
	  os << '"' << r.label << '"';
      } else {
	  lhs_node rfc = rd.lhs[idx + 1];
	  bool preterm = rfc.next == 0 and not rfc.indexed and not rfc.children;
          if (preterm) {
 	      if (idx + 2 < rd.lhs.size()) print_preterm = true; 
	      if (print_preterm) {
		  if (not first) os << ' ';
		  first = false;
		  os << 'X' << '(' << '"' << rfc.label << '"' << ')';
		  ++idx;
	      }
	  }
      }
  }
  os << ')';
  return os;
}


int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);
    cout << print_rule_data_lhs(false);
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }
         
        print_no_internals(cout,rd);
        cout << " -> " << rd << '\n';
    }
    return 0;
}
