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
// This test demos a write after read operation (for extra-credit).

# include <Transaction.hh>
# include <iostream>

void print_block(byte *);

int main()
{

  Begin (T)

  byte *oldval = new byte[BLOCKSIZE];
  
  cout << "Reading block 2" << endl;
  byte *t = (byte *)T.read(2);
  print_block(t);

  cout << "Reading block 3" << endl;
  t = (byte *)T.read(3);
  print_block(t);
  
  // write to this block.
  byte *newval = new byte[BLOCKSIZE];
  memset((void *)newval, 'Z', BLOCKSIZE);
  
  cout<< "writing ZZZZ... to block 2" << endl;
  T.write(2, (void *)oldval, (void *)newval);

  cout << "Block 2 previously was: " << endl;
  print_block((byte *)oldval);

  cout << "Now aborting tranasction.." << endl;
  T.abort();
  
  End (T)
}
    
void
print_block(byte *t)
{
  cout << "block info:\n";
  for (int i=0; i<BLOCKSIZE; ++i)
    cout << (byte)t[i];
  cout<<endl;  
}
