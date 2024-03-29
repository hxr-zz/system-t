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
// This preliminary test case is here to initialize some blocks for further tests.
# include <Transaction.hh>
# include <iostream>
using namespace std;

int
main()
{
  Begin(T)

    int b;
    for(int i=0; i<5; ++i)
      {
	b = T.allocate();
	cout << "Allocated block #" << b << endl;
      }

    End(T)
}
