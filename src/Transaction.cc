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
// Implementation of Transaction class. This is the main class in libtransaction

# include <Transaction.hh>
# include <iostream>

unsigned int Transaction::last_sleep = 0;
int Transaction::last_free_block;

Transaction::Transaction()
{
  // Open the store and index.
  if ( (storefd = open(STOREFILE, O_RDWR | O_SYNC) )  == -1 )
    {
      cerr << __FILE__ << ":Cannot open the datastore file " << STOREFILE << endl;
      exit(1);
    }

  if ( (indexfd = open(INDEXFILE, O_RDWR | O_SYNC)) == -1 )
    {
      cerr << __FILE__ << ":Cannot open the data index file " << INDEXFILE << endl;
      exit(1);
    }
  
  sockfd = tcp.createSocket();
  pid    = getpid();
  tid    = 0;
  error_occured = false;

  last_free_block = 0;
  // Connect to TPM and get a transaction ID;
  tcp.connectSocket(sockfd, std::string(TPM_HOST), TPM_PORT);

  trq rq;
  if ( !ask_tpm (trq::BEGIN,0,rq) )
    {
      cerr << "libtransaction: transaction monitor refused to assign an ID!" << endl;
      exit(1);
    }
 
  this->tid = rq.tid;
}


int
Transaction::allocate() throw (TransactionException)
{
  int fblock;
  bool found = false;
  
  while ( !found)
    {
      fblock= find_free_block(); // will restart after examining all blocks.
      trq rq;
      found = ask_tpm(trq::ALLOCATE, fblock, rq);

      if (!found)
	cerr << "libtransaction[tid #" << tid << "]: contention for block # " \
	     << fblock << " will find another one." << endl;
    }

  // Allocate a memory chunk and add it to the block map
  byte *block = (byte *)new byte[BLOCKSIZE];
  for(int i=0; i< BLOCKSIZE; ++i)
    block[i] = '0';
  
  mblock mb = { A_LOCK, block, block, 'U' }; // New and old values point to the same block
  blockmap[fblock] = mb;
  
  return fblock;
}

void*
Transaction::read(int block) throw (TransactionException)
{
  if (block <=0 )
    {
      cerr << "libtransaction: Invalid block number" << endl;
      throw TE_ABORT;
    }
  
  map<int, mblock>::iterator p = blockmap.find(block);

  if ( p != blockmap.end())
    {
      if ( (p->second).lock == S_LOCK )
	{
	  cerr << "libtransaction[tid #" << tid \
	       << "]: Transaction tried to read a block after releasing it! " << endl;
	  abort();
	}

      // for other locks, it is OK to read.
      return (void *) ((p->second).data);
    }

  // Read a block that is in the store.
  if ( ! is_allocated(block) )
    {
      cerr << "libtransaction[tid #" << tid \
	   << "]: Transaction is trying to read an un-allocated block!" << endl;
      abort();
    }
  
  trq reply;
  if ( !ask_tpm(trq::READ, block, reply) )
    {
      cerr << "libtransaction[tid #" << tid << "]: cannot read block # " \
	   << block << " at this time." << endl;
      restart();
    }

  // Now do an actual read.
  lock(block, R_LOCK);

  byte *buffer = new byte[BLOCKSIZE];
  lseek(storefd, (block-1)*BLOCKSIZE, SEEK_SET);
  if (::read (storefd, buffer, BLOCKSIZE) == -1)
    {
      cerr << "libtransaction: error reading datastore!" << endl;
      abort();
    }

  // put it in blockmap for next time.
  mblock mb = { R_LOCK, buffer, buffer, 'A' }; 
  blockmap[block] = mb;

  return (void *)buffer;
}


void
Transaction::write(int block, void *oldval, void *newval) throw (TransactionException)
{

  if (block <=0 )
    {
      cerr << "libtransaction: Invalid block number" << endl;
      throw TE_ABORT;
    }

  // Check if this block is in our buffers
  map<int, mblock>::iterator p = blockmap.find(block);

  if (p != blockmap.end())
    {
      switch (p->second.lock)
	{
	case S_LOCK:
	  cerr << "libtransaction[tid #" << tid \
	       << "]: transaction tried to write a block after releasing it!" << endl;
	  abort();
	  break;

	case R_LOCK:  // Not enough to be able to write.
	  if (p->second.data != NULL)
	    delete (p->second).data;
	  p->second.data = NULL;
	  break;

	case A_LOCK:
	case W_LOCK:  // Do a local write.
	  memcpy(oldval,(p->second).data,BLOCKSIZE);
	  memcpy((p->second).data,newval,BLOCKSIZE);
	  return;

	default:
	  break;
	}
    }

  // Write a block that is in the store.
  if ( ! is_allocated(block))
    {
      cerr << "libtransaction: transaction is attempting to write an un-allocated block!" << endl;
      abort();
    }

  trq reply;
  if ( !ask_tpm(trq::WRITE, block, reply) )
    {
      cerr << "libtransaction[tid #" << tid \
	   << "]: transaction monitor denied write access to block # " << block << " at this time." << endl;
      restart();
    }

  // Actual write will be delayed until commit time. For now, read the old value and return it.
  lseek(storefd, (block-1)*BLOCKSIZE, SEEK_SET);
  if (::read(storefd, oldval, BLOCKSIZE) == -1)
    {
      cerr << "libtransaction: error reading datastore!" << endl;
      abort();
    }

  byte *saved_val = new byte[BLOCKSIZE];
  memcpy(saved_val, oldval, BLOCKSIZE);

  byte *data = new byte[BLOCKSIZE];
  memcpy(data, newval, BLOCKSIZE);
  
  mblock mb = { W_LOCK, data, saved_val, 'A' };
  blockmap[block] = mb;
}

void
Transaction::release(int block) throw (TransactionException)
{
  if (block <=0 )
    {
      cerr << "libtransaction: Invalid block number" << endl;
      throw TE_ABORT;
    }

  // Check if block is in our map
  map<int, mblock>::iterator p = blockmap.find(block);

  if ( p != blockmap.end())
    {
      switch ( p->second.lock )
	{
	case R_LOCK: // Not sufficient to release a block!
	  if (p->second.data != NULL)
	    delete p->second.data; // get rid of the read buffer.
	  p->second.data = NULL;
	  break;
	case A_LOCK:
	case W_LOCK:
	case S_LOCK:
	  (p->second).lock = S_LOCK;
	  return;
	}
    }

  // Release an allocated block from store. Must get a write lock on the block.
  if (! is_allocated( block) )
    {
      cerr << "libtransaction: transaction tried to release an un-allocated block!" << endl;
      abort();
    }
  
  trq reply;
  if ( !ask_tpm(trq::RELEASE, block, reply) )
    {
      cerr << "libtransaction[tid #" << tid \
	   << "]: montior denied RELEASE permission for block # " << block << " at this time." << endl;
      restart();
    }

  // read in the old value (for logging purposes).
  byte *buffer = new byte[BLOCKSIZE];
  lseek(storefd, (block-1)*BLOCKSIZE, SEEK_SET);
  if (::read(storefd, buffer, BLOCKSIZE) == -1)
    {
      cerr << "libtransaction: error reading datastore!" << endl;
      abort();
    }

  mblock mb = { S_LOCK, buffer, buffer, 'A' };
  blockmap[block] = mb;
}

bool
Transaction::ask_tpm(trq::request_t request, int blockno, trq& rply)
{
  trq rq = { tid, pid, request, trq::OK, blockno};
  
  tcp.writeSocket(sockfd, rq);
  tcp.readSocket (sockfd, rply);

  // Monitor preempted me!
  // This condition CANNOT occur after a trq::COMMIT has been issued.
  if (rply.reply == trq::DIE) 
    {
      cerr << "libtransaction[tid #" << tid \
	   <<"]: transaction Monitor Preempted this transaction." << endl;
      abort();
    }
  
  return (rply.reply == trq::OK);
}

void
Transaction::lock(int block, locktype ltype)
{
}

void
Transaction::unlock(int block)
{
}

int
Transaction::find_free_block() throw (TransactionException)
{
  // Find an unallocated block in the index file and return block number.
  char bstat;

  last_free_block++;			// everytime this function is called, we search the next block.
  lseek(indexfd, last_free_block-1, SEEK_SET);
  
  while ( ::read (indexfd, &bstat, 1) != 0)
    {
      if (bstat == 'U')
	return last_free_block;
      else
	last_free_block++;
    }

  cerr<<"libtransaction: could not find an unallocated block in the store!"<<endl;
  restart();		// try again buddy.
}


// Check if a block is allocated at all
bool
Transaction::is_allocated ( int blockno )
{
  if (blockno > NBLOCKS)
    return false;
  
  lseek(indexfd, (blockno-1), SEEK_SET);

  char stat;
  if ( ( ::read ( indexfd, &stat, 1) == -1))
    {
      cerr << "libtransaction: error reading data index!" << endl;
      abort();
    }

  return (stat == 'A');
  
}

void
Transaction::abort()
{
  cerr << "libtransaction[tid #" << tid << "]: aborting Transaction!" << endl;
  error_occured = true;
  throw TE_ABORT;		// This will end the process. Promting the server to rollback.
}


void
Transaction::commit()
{
  char unalloc = 'U', alloc = 'A';
  // tell server we are committing.
  trq rq;
  if (! ask_tpm(trq::COMMIT, 0, rq))
    {
      cerr << "libtransaction: monitor does not let you commit at this time!" << endl;
      abort();
    }

  // start committing
  logger.log(tid, LBEGIN);
  map<int, mblock> :: iterator p = blockmap.begin();

  while ( p != blockmap.end() )
    {
      switch(p->second.lock)
	{
	case A_LOCK:		// commit an allocated block
	  logger.log(tid, LALLOCATE, p->first);
	  lseek(indexfd, ( (p->first)-1), SEEK_SET);
	  if(::write(indexfd, &alloc, 1) == -1)
	    {
	      cerr << "libtransaction: commit() cannot write to dataindex!" << endl;
	      abort();
	    }
	  // No break here deliberately. A write to an allocated block is not explicit.
	case W_LOCK:
	  if (p->second.lock == W_LOCK)
	    logger.log(tid, LWRITE, p->first, p->second.oldindex, (void *)p->second.oldval);
	  
	  lseek(storefd, ( (p->first) - 1)*BLOCKSIZE, SEEK_SET);
	  if (::write (storefd, p->second.data, BLOCKSIZE) == -1)
	    {
	      cerr << "libtransaction: commit() cannot write to datastore! " << endl;
	      abort();
	    }
	  unlock(p->first);
	  break;

	case S_LOCK:		// commit a block that we released.
	  logger.log(tid, LRELEASE, p->first, p->second.oldindex, (void *)p->second.oldval);
	  
	  lseek(indexfd, ( (p->first)-1), SEEK_SET);
	  if(::write (indexfd, &unalloc, 1) == -1)
	    {
	      cerr << "libtransaction commit() cannot write to dataindex! " << endl;
	      abort();
	    }
	  unlock(p->first);
	  break;		

	case R_LOCK:		// anything to be done ??
	  logger.log(tid, LREAD, p->first);
	  unlock(p->first);
	  break;
	}
      
      ++p;			// next block
    }

# ifdef OBSERVE_ROLLBACK
  cerr << "ALL COMMIT operations done. Now sleeping for 5 seconds. You can send a signal now to observe rollback" << endl;
  sleep(5);
# endif
  
  // write the END marker to the log.
  logger.log(tid, LEND);
  
  // Now tell server we are done.
  trq reply;
  if (! ask_tpm(trq::END, 0, reply))
    {
      cerr << "libtransaction: monitor will not let you end! or it died fatal error! " << endl;
      throw TE_ABORT;
    }
}

// Implements a binary exponential backoff sleep.
void
Transaction::backoff_sleep()
{
  unsigned int sleeptime = (unsigned int)0x1 << last_sleep++; 
  if (sleeptime > MAXTIMEOUT)
    {
      cerr << "libtransaction: MAX timeout reached!" << endl;
      abort();
    }
  
  cerr << "libtransaction: sleeping " << sleeptime << " seconds before restarting." << endl;
  sleep(sleeptime);
}

// restart this transaction after a while
void
Transaction::restart()
{

  // tell the server about this
  trq rq;
  if (! ask_tpm (trq::RESTART, 0, rq) )
    {
      cerr << "libtransaction: server does not allow me to restart! " << endl;
      abort();
    }
  
  this->backoff_sleep();

  error_occured = true;
  throw TE_RESTART;
}

Transaction::~Transaction()
{
  if ( error_occured == false )	        // everything went as planned.
    commit();			
  
  // Release any newly allocated blocks in the blockmap
  map<int, mblock>::iterator p = blockmap.begin();
  while(p != blockmap.end())
    {
      if ((p->second.oldval != p->second.data) && (p->second.oldval !=NULL))
	delete p->second.oldval;
      
      if (p->second.data != NULL)
	delete (p->second).data;

      ++p;
    }
  close(storefd);
  close(indexfd);
  tcp.closeSocket(sockfd);	// close comminication with monitor
}
