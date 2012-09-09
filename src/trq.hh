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
// A 'Transaction Request' is the communication token between the TPM
// and a process executing a transaction.

# ifndef TRQ_HH
# define TRQ_HH

# include <config.h>

struct trq
{
  enum request_t { BEGIN, COMMIT, END, READ, WRITE, ABORT, ALLOCATE, RELEASE, RESTART };
  enum reply_t { OK, NOT_OK, DIE };
  
  trid_t    tid;		// Transaction ID
  pid_t     pid;		// process ID that sent this request  
  request_t request;		// begin/read/write/abort/allocate/release/rollback
  reply_t   reply;		// reply from server
  int       block_no;		// Block number affected in this transaction.
};

# endif
