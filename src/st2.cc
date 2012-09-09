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
// This test demonstrates:
// - Allocate operation
// - Read operation
// - write after allocate
// - read after allocate, write

# include <Transaction.hh>
# include <iostream>

void print_block(byte *);

int main()
{

  for (int i=0; i<2; ++i)
    {
  Begin (T)

  int b = T.allocate();

  void *oldval = (void *) new byte[BLOCKSIZE];
  byte *newval = new byte[BLOCKSIZE];
  memset(newval,'B', BLOCKSIZE);

  cout << "Reading block #1" << endl;
  byte *t = (byte *)T.read(1);
  print_block(t);
  
  // write to this block.
  cout << "writing a string to newly allocated block " << b << endl;
  T.write(b, oldval, (void *)newval);

  cout << "Now reading the newly allocate block back" << endl;
  t = (byte *)T.read(b);
  print_block(t);

  End (T)
    }

}
    
void
print_block(byte *t)
{
  for (int i=0; i<BLOCKSIZE; ++i)
    cout << (byte)t[i];
  cout<<endl;  
}
