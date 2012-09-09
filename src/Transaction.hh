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
// A Transaction class. This class handles initiation, reads, writes, and
// proper termination (commit/abort) of a transaction.

# ifndef TRANSACTION_HH
# define TRANSACTION_HH

# include <config.h>
# include <LogManager.hh>
# include <Transport.hh>
# include <trq.hh>
# include <string>
# include <map>
# include <fstream>
using namespace std;

enum TransactionException { TE_ABORT, TE_RESTART, TE_FAIL };
enum locktype { A_LOCK, R_LOCK, W_LOCK, S_LOCK }; // allocate, read, write and release locks

struct mblock
{
  locktype lock;
  byte *data;
  byte *oldval;
  char oldindex;		// for release()
};

class Transaction
{
  trid_t tid;			// The Transaction ID
  pid_t  pid;			// Who is using running this transaction.

  int storefd;			// The main storage area
  int indexfd;			// Index file
  
  TransportLayer tcp;		// For communication with TPM
  int sockfd;

  bool error_occured;		// tracks if an error has occured during this transaction
  static unsigned int last_sleep;
  static int last_free_block;
  
  LogManager logger;		// Log manager for this transaction
private:

  std::map<int, mblock> blockmap; // This stores newly allocated blocks.
  
  void commit();

  void lock(int block, locktype ltype);
  void unlock(int block);

  int find_free_block() throw (TransactionException);	// Find the next available free block in the store.
  
  bool ask_tpm(trq::request_t rq, int block, trq& reply);
  bool is_allocated(int blockno); // check if this block is allocated at all.

  void backoff_sleep();		// implements a binary backoff sleep
  void restart();		// restarts transaction
  
public:
  Transaction();		// Constructor
  ~Transaction();		// Destructor
  
  int  allocate() throw (TransactionException);		// Allocate a new block in the store.
  void release(int block) throw (TransactionException);	// Release a previously allocated block.

  void abort();			// Abort this transaction
  
  void *read(int block) throw (TransactionException);	// Read a block
  void write(int block, void *oldval, void *newval) throw (TransactionException);
};


# define Begin(T) begin_transaction: \
                   try{ \
		   Transaction T; \

# define End(T) } \
                catch (TransactionException tex) \
		{ \
		  switch (tex) \
		    { \
		    case TE_ABORT: \
		      cerr<<"Transaction failed. Now aborting!"<<endl; \
		      exit(1); \
		      break; \
		    case TE_RESTART: \
		      cout<<"Restarting transaction..." << endl; \
		      goto begin_transaction; \
		      break; \
		    } \
		} \


# endif
