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
// This is the last program in the concurrent test series.
// This progam tries to wirte to blocks #1 and #2.
// Block #1 was write locked by ct1.cc, so this transaction restarts a few times
// Block #2 was read locked by ct1.cc and ct2.cc. But ct2.cc sleeps so much that it
// exceeds its time limit. So, it will be preempted by the monitor to give access to ct3.cc

# include <Transaction.hh>
# include <iostream>

void print_block(byte *);

int main()
{

  Begin (T)


  void *oldval = (void *) new byte[BLOCKSIZE];
  byte *newval = new byte[BLOCKSIZE];
  memset((void *)newval, 'H', BLOCKSIZE);
  
  // write to this block.
  cout << "ct3.cc: Asking to write block #1" << endl;
  T.write(1, oldval, (void *)newval);
  cout <<"ct3.cc: Block 1 written" << endl;

  memset((void *)newval, 'I', BLOCKSIZE);
  cout << "ct3.cc: Asking to write 2" << endl;
  T.write(2, oldval, (void*)newval);
  cout << "ct3.cc: Block 2 written" << endl;

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
