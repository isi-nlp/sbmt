// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/*
 * LiBEDebug.H --
 *	General object debugging facility
 */
#ifndef _LiBEDebug_H_
#define _LiBEDebug_H_

#include <iostream>


using namespace std;
/*
 * Here is the typical usage for this mixin class.
 * First, include it in the parents of some class FOO
 *
 * class FOO: public OTHER_PARENT, public FOO { ... }
 *
 * Inside FOO's methods use code such as
 *
 *	if (debug(3)) {
 *	   dout() << "I'm feeling sick today\n";
 *	}
 *
 * Finally, use that code, after setting the debugging level
 * of the object and/or redirecting the debugging output.
 *
 *      FOO foo;
 *	foo.debugme(4); foo.dout(cout);
 *
 * LiBEDebugging can also be set globally (to affect all objects of
 * all classes.
 *
 *	foo.debugall(1);
 *
 */
class LiBEDebug
{
public:
    LiBEDebug(unsigned level = 0)
      : nodebug(false), debugLevel(level), debugStream(&cerr) {};

    bool debug(unsigned level)  const  /* true if debugging */
	{ return (!nodebug && (debugAll >= level || debugLevel >= level)); };
    virtual void debugme(unsigned level) { debugLevel = level; };
				    /* set object's debugging level */
    void debugall(unsigned level) { debugAll = level; };
				    /* set global debugging level */
    unsigned debuglevel() { return debugLevel; };

    virtual ostream &dout() const { return *debugStream; };
				    /* output stream for use with << */
    virtual ostream &dout(ostream &stream)  /* redirect debugging output */
	{ debugStream = &stream; return stream; };

    bool nodebug;		    /* temporarily disable debugging */
private:
    static unsigned debugAll;	    /* global debugging level */
    unsigned debugLevel;	    /* level of output -- the higher the more*/
    ostream *debugStream;	    /* current debug output stream */
};

#endif /* _LiBEDebug_h_ */

