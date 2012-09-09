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
//
// This test demonstrates:
// - Read operation
// - write operation
// - Release operation
// - Release after Allocate in the same transaction
// - Read after write in the same transation.


# include <Transaction.hh>
# include <iostream>

void print_block(byte *);

int main()
{

  Begin (T)

  void *oldval = (void *) new byte[BLOCKSIZE];
  byte *newval = new byte[BLOCKSIZE];
  memset((void *)newval, 'C', BLOCKSIZE);
  
  cout << "writing block #1 as CCC.." << endl;
  T.write(1,oldval,(void *)newval);
  
  cout << "Block 1 previously was: " << endl;
  print_block((byte *)oldval);

  int nb1 = T.allocate();
  cout << "Allocated block # " << nb1 << endl;
  int nb2 = T.allocate();
  cout << "Allocated block # " << nb2 << endl;

  byte *new3val = new byte[BLOCKSIZE];
  memset((void *)new3val, 'D', BLOCKSIZE);
  
  cout << "Writing " << nb1 << " as DDD.. " << endl;
  T.write(nb1, oldval, (void *)new3val);

  memset((void*)new3val, 'E', BLOCKSIZE);
  cout << "Writing " << nb2 << " as EEE.. " << endl;
  T.write(nb2, oldval, (void *)new3val);

  cout << "Releasing block 5" <<endl;
  T.release(5);
  
  cout << "(Release after allocate) Releasing block " << nb2 << endl;
  T.release(nb2);

  cout << "(Read after write) Reading block " << nb1 << endl;
  void *t = T.read(nb1);
  print_block((byte*)t);


  cout << "Writing block #2"  << endl;
  memset((void*)new3val, 'K', BLOCKSIZE);
  T.write(2,oldval, (void *)new3val);
  
  End (T)

}
    
void
print_block(byte *t)
{
  for (int i=0; i<BLOCKSIZE; ++i)
    cout << (byte)t[i];
  cout<<endl;  
}
