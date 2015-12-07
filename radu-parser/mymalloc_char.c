/* This code is the statistical natural language parser described in

   M. Collins. 1999.  Head-Driven
   Statistical Models for Natural Language Parsing. PhD Dissertation,
   University of Pennsylvania.

   Copyright (C) 1999 Michael Collins

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <assert.h>
#include "mymalloc_char.h"

int mychar_howfull;
unsigned char *charstartspace;

unsigned char *mymalloc_char(int size)
{
  void *space;

  if( mychar_howfull + size > MYMCSIZE )
    {
      mychar_howfull=size;
      charstartspace = (unsigned char *) malloc(MYMCSIZE);
      return charstartspace;
    }

  space=mychar_howfull+charstartspace;
  mychar_howfull+=size;
  return (unsigned char *)space;
}

void mymalloc_char_init()
{
  mychar_howfull=0;
  charstartspace = (unsigned char *) malloc(MYMCSIZE);  
}
