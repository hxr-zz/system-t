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
// This is the second program in concurrent test series.
// It read locks blocks #2, and #3 and sleeps.

# include <Transaction.hh>
# include <iostream>

void print_block(byte *);

int main()
{

  Begin (T)

    cout << "ct2.cc: Now reading block 2" << endl;
    byte *t = (byte *)T.read(2);
    print_block(t);

    cout << "ct2.cc: Now reading block 3" << endl;
    t = (byte *)T.read(3);
    print_block(t);
  
    sleep(20);
  End (T)
}
    
void
print_block(byte *t)
{
  for (int i=0; i<BLOCKSIZE; ++i)
    cout << (byte)t[i];
  cout<<endl;  
}
