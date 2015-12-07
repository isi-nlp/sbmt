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
#include "mymalloc.h"

int my_howfull;
void *startspace;

void *mymalloc(int size)
{
  void *space;

  if( my_howfull + size > MYMBSIZE )
    {
      my_howfull=size;
      startspace = (void *) malloc(MYMBSIZE);
      return startspace;
    }

  space=my_howfull+ (char *) startspace;
  my_howfull+=size;
  return space;
}

void mymalloc_init()
{
  my_howfull=0;
  startspace = (void *) malloc(MYMBSIZE);  
}
