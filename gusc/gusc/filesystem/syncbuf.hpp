# if ! defined(GUSC__FILESYSTEM__SYNCBUF_HPP)
# define       GUSC__FILESYSTEM__SYNCBUF_HPP

# include <iostream>
# include <cstdio>

namespace gusc {
////////////////////////////////////////////////////////////////////////////////
//
// syncbuf is a buffer interface that defers all buffering to an underlying
// FILE*.  if you know how it works then you understand more about iostreams
// than i do...
////////////////////////////////////////////////////////////////////////////////
class syncbuf : public std::streambuf {
public:
   syncbuf(FILE*);
   virtual ~syncbuf();
protected:
   virtual int overflow(int c = EOF);
   virtual int underflow();
   virtual int uflow();
   virtual int pbackfail(int c = EOF);
   virtual int sync();

private:
   FILE* fptr;
};

} // namespace gusc

# endif  //    GUSC__FILESYSTEM__SYNCBUF_HPP
