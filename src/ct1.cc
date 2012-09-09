// Copyright (C) 2007 Satya Kiran Popuri <spopur2@uic.edu>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  
// This program is the first in the concurrent test series
// It readlocks blocks #2 #3 and write locks block #1

# include <Transaction.hh>
# include <iostream>

void print_block(byte *);

int main()
{

  Begin (T)

  void *oldval = (void *) new byte[BLOCKSIZE];
  byte *newval = new byte[BLOCKSIZE];
  memset((void *)newval, 'G', BLOCKSIZE);
  
  cout << "ct1.cc: Reading block 2" << endl;
  byte *t = (byte *)T.read(2);
  print_block(t);

  cout << "ct1.cc: Reading block 3" << endl;
  t = (byte *)T.read(3);
  print_block(t);
  
  cout <<"ct1.cc: Now writing GGG.. to block 1" << endl;
  T.write(1,oldval,(void *)newval);
  cout << "ct1.cc: Block 1 previously was: " << endl;
  print_block((byte *)oldval);

  sleep(2);
  End (T)
}
    
void
print_block(byte *t)
{
  for (int i=0; i<BLOCKSIZE; ++i)
    cout << (byte)t[i];
  cout<<endl;  
}
