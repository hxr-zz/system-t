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
// Implementation of the Transaction Processing Monitor

# include <Monitor.hh>
# include <iostream>
# include <stdlib.h>

TransactionMonitor::TransactionMonitor()
{
  port = TPM_PORT;
  sockfd = tcp.createSocket();
  next_tid = 1;
}

TransactionMonitor::~TransactionMonitor()
{
  tcp.closeSocket(sockfd);
}

void
TransactionMonitor::setPort(unsigned int port)
{
  this->port = port;
}

// set the ball rolling
void
TransactionMonitor::start()
{
  tcp.bindSocket(sockfd, port);
  tcp.listenSocket(sockfd, 10);

  // start multiplexing the incoming connections
  int maxfd = sockfd;
  int clientfd = -1;

  trq scratch_trq;
  int ready;

  fd_set readset, everyone;

  FD_ZERO(&everyone);
  FD_ZERO(&readset);

  FD_SET(sockfd, &everyone);

  while ( true )
    {
      readset = everyone;

      if ( (ready = select ( maxfd+1, &readset, NULL, NULL, NULL )) == -1)
	{
	  perror("Failed on select() call!");
	  exit(1);
	}

      if ( FD_ISSET(sockfd, &readset))
	{
	  //A new process has connected to the TPM
	  clientfd = tcp.acceptSocket(sockfd);
	  FD_SET(clientfd, &everyone);

	  clients.push_front(clientfd);
	  
	  maxfd = clientfd > maxfd ? clientfd : maxfd;

	  if (--ready <= 0)
	    continue;
	}
	  
      // Iterate thru all of our clients and check if anyone is talking to us
      list<int> :: iterator p = clients.begin();

      while ( p!= clients.end())
	{

	  if ( FD_ISSET (*p, &readset) )
	    {
	      //client wants to say something
	      if ( tcp.readSocket(*p, scratch_trq) == 0 )
		{
		  //client closed connection on us! it may have died
		  client_disconnect(*p); // process this event
		  tcp.closeSocket(*p);
		  FD_CLR(*p, &everyone);
		}
	      else
		{
		  process_clientrq(scratch_trq, *p); // process client's request.
		}

	      if (--ready <= 0)	// This is an optimization. 
		break;
	    }

	  ++p;			// check next client
	}
    }
}

void
TransactionMonitor::client_disconnect(int fd)
{
  // Has this client initiated a transaction ?
  map<int,trid_t>::iterator p = transactions.find(fd);

  if ( p == transactions.end())
      return;

  map<trid_t, tstat>::iterator q = status.find(p->second);

  if ( q == status.end())
    return;

  cout << "Transaction " << q->first << " has disconnected. " << endl;

  // Check the status of this transaction. If commit started, we need to do a log rollback
  if (q->second.stat == tstat::COMMITTING)
    {
# ifdef OBSERVE_RECOVERY
      cout << "A transaction died during COMMIT phase. Recovery testing is enabled." \
	   << "I am dying too. Run ./recover to recover the datastore and restart the monitor" << endl;
      raise(SIGKILL);
#endif      
      logger.rollback(q->first);
    }
  // For all other states, it is ok to flush and close
  flush_transaction(p->second);
  transactions.erase(p);	// this is the final step
}

void
TransactionMonitor::process_clientrq(trq& t, int who)
{
  // Check if this transaction had been preempted
  if (t.request != trq::BEGIN)
    {
      map<trid_t, tstat> :: iterator p = status.find(t.tid);

      if (p != status.end())
	{
	  if (p->second.stat == tstat::PREEMPTED)
	    {
	      trq die_trq = { t.pid, t.tid, t.request, trq::DIE, t.block_no };
	      reply(die_trq, who);
	      return;
	    }
	}
    }
	   
  switch (t.request)
    {
    case trq::BEGIN:
      {
	//trid_t tid = t.pid + random();
	trid_t tid   = next_tid++; // assign and increment
	transactions[who] = tid;
	tstat stat = { t.pid, 0, time(NULL), tstat::BEGUN };
	status[tid] = stat;
	cout << "Assigned new TID : "<< tid << " to process " << t.pid << endl;
	//send a reply with the transaction ID
	trq ok_trq = {tid, t.pid, t.request, trq::OK, 0};
	tcp.writeSocket(who, ok_trq);
      }
      break;

    case trq::ALLOCATE:
      this->allocate_rq(t, who);
      break;

    case trq::READ:
      this->read_rq(t, who);
      break;

    case trq::WRITE:
      this->write_rq(t, who);
      break;

    case trq::RELEASE:
      this->release_rq(t, who);
      break;

    case trq::RESTART:		// client wants to restart
      this->restart_rq(t, who);
      break;
      
    case trq::ABORT:		// client wants to abort
      this->abort_rq(t, who);
      break;

    case trq::COMMIT:		// client starts to commit
      this->commit_rq(t, who);
      break;
      
    case trq::END:		// client finished committing
      this->end_rq(t, who);
      break;

    default:
      break;
    }
}

void
TransactionMonitor::allocate_rq(trq& rq, int whosent)
{
  trq ok_trq = { rq.pid, rq.tid, rq.request, trq::OK, rq.block_no }, \
    nok_trq  = { rq.pid, rq.tid, rq.request, trq::NOT_OK, rq.block_no};
  
  // check if anyone is holding this block already
  if ( can_write (rq.tid, rq.block_no) )
    {
      // make an entry for this block
      set_writer(rq.block_no, rq.tid);
      update_status(rq.tid, rq.block_no, tstat::ALLOCATED);
      reply(ok_trq, whosent);
    }
  else
    {
      reply (nok_trq, whosent);
    }
}

void
TransactionMonitor::read_rq(trq& rq, int whosent)
{
  trq ok_trq = { rq.pid, rq.tid, rq.request, trq::OK, rq.block_no }, \
    nok_trq  = { rq.pid, rq.tid, rq.request, trq::NOT_OK, rq.block_no};

  if ( can_read (rq.tid, rq.block_no) )
    {
      set_reader ( rq.block_no, rq.tid );
      update_status (rq.tid, rq.block_no, tstat::READING); 
      reply (ok_trq, whosent);
    }
  else
    {
      reply (nok_trq, whosent);
    }
}

void
TransactionMonitor::write_rq(trq& rq, int whosent)
{
  trq ok_trq = { rq.pid, rq.tid, rq.request, trq::OK, rq.block_no }, \
    nok_trq  = { rq.pid, rq.tid, rq.request, trq::NOT_OK, rq.block_no};

  // Special Case !! If this is a write after read,
  // and this transaction is the only reader, we can allow an upgrade!
  bool grant = false;
  bool sole_reader = false;
  if ( (sole_reader = check_sole_reader(rq.tid, rq.block_no)) == true)
    {
    grant = true;
    }
  else
    {
      grant = can_write (rq.tid, rq.block_no);
    }

  if (grant)
    {
      if (sole_reader)
	remove_reader(rq.block_no, rq.tid);
      
      set_writer (rq.block_no, rq.tid);
      update_status(rq.tid, rq.block_no, tstat::WRITING);
      reply(ok_trq, whosent);
    }
  else
    {
      reply (nok_trq, whosent);
    }
}

void
TransactionMonitor::release_rq(trq& rq, int whosent)
{
    trq ok_trq = { rq.pid, rq.tid, rq.request, trq::OK, rq.block_no }, \
    nok_trq  = { rq.pid, rq.tid, rq.request, trq::NOT_OK, rq.block_no};

  if ( can_write(rq.tid, rq.block_no) )
    {
      set_writer (rq.block_no, rq.tid);
      update_status(rq.tid, rq.block_no, tstat::RELEASING);
      reply(ok_trq, whosent);
    }
  else
    {
      reply (nok_trq, whosent);
    }
}

void
TransactionMonitor::abort_rq(trq& rq, int whosent)
{
  trq ok_trq = { rq.pid, rq.tid, rq.request, trq::OK, rq.block_no }, \
    nok_trq  = { rq.pid, rq.tid, rq.request, trq::NOT_OK, rq.block_no};
  // change the status
  map<trid_t, tstat> :: iterator p = status.find(rq.tid);

  if ( p != status.end() )
    {
      p->second.stat = tstat::ABORTING;
      reply(ok_trq, whosent);
    }
  else
    reply(nok_trq, whosent);
}

void
TransactionMonitor::commit_rq(trq& rq, int whosent)
{
  trq ok_trq = { rq.pid, rq.tid, rq.request, trq::OK, rq.block_no }, \
    nok_trq  = { rq.pid, rq.tid, rq.request, trq::NOT_OK, rq.block_no};
  // change the status
  map<trid_t, tstat> :: iterator p = status.find(rq.tid);

  if ( p != status.end() )
    {
      p->second.stat = tstat::COMMITTING;
      reply(ok_trq, whosent);
    }
  else
    reply(nok_trq, whosent);
  
}

void
TransactionMonitor::restart_rq(trq& rq, int whosent)
{
  trq ok_trq = { rq.pid, rq.tid, rq.request, trq::OK, rq.block_no }, \
    nok_trq  = { rq.pid, rq.tid, rq.request, trq::NOT_OK, rq.block_no};
  // change the status
  map<trid_t, tstat> :: iterator p = status.find(rq.tid);

  if ( p != status.end() )
    {
      p->second.stat = tstat::RESTARTING;
      reply(ok_trq, whosent);
      cout << "Process " << rq.pid << " transaction " << rq.tid << " wants to restart" <<endl;
    }
  else
    reply(nok_trq, whosent);
}

void
TransactionMonitor::end_rq(trq& rq, int whosent)
{
  // Transaction has finished committing successfully.
  trq ok_trq = { rq.pid, rq.tid, rq.request, trq::OK, rq.block_no }, \
    nok_trq  = { rq.pid, rq.tid, rq.request, trq::NOT_OK, rq.block_no};

  map<trid_t, tstat> :: iterator p = status.find(rq.tid);

  if ( p != status.end() )
    {
      if ( p->second.stat == tstat::COMMITTING )
	{
	  p->second.stat = tstat::ENDED;
	  reply(ok_trq, whosent);
	  cout << "Process " << rq.pid << " transaction " << rq.tid << " committed successfully" <<endl;
	  return;
	}
    }

  reply(nok_trq, whosent);
}

void
TransactionMonitor::reply(trq& t, int towho)
{
  tcp.writeSocket(towho, t);
}


// This function is called when there is an R_LOCK request
bool
TransactionMonitor::can_read (trid_t tid, int blockno)
{
  map<int, blockholders>::iterator p = blockmap.find(blockno);

  if ( p == blockmap.end() )	// no block in map; sure you can read
    return true;


  if (p->second.writer == 0)	// no writers. no conflict. go ahead.
    return true;

  // hmm there is a writer guy. check if tid wins over him
  trid_t writer = p->second.writer;
  map<trid_t, tstat>::iterator q = status.find(writer);

  if (q == status.end())
    {
      //clearly something went horribly wrong.
      cerr << "You have some brain dead code in here. " << endl;
      exit(1);
    }

  if (q->second.stat == tstat::COMMITTING) // we do NOT EVER preempt committing transactions
    return false;


  if ( YOUNGER(writer,tid) )
    {
      q->second.stat = tstat::PREEMPTED;
      p->second.writer = 0;	// as far as we are concerned, no writer for this block.
      return true;
    }
       
  if (difftime(time(NULL), q->second.started) > TIMESLICE) // writer is older, but run out of time
    {
      q->second.stat = tstat::PREEMPTED;
      p->second.writer = 0;
      return true;
    }
  else
    {
      return false;
    }
   
}


// This function is called when there is a W_LOCK request.
bool
TransactionMonitor::can_write(trid_t tid, int blockno)
{
  map<int, blockholders>::iterator p = blockmap.find(blockno);

  if ( p == blockmap.end() )	// no block in map
    {
      return true;
    }

  if (p->second.writer == 0)
    {
      // check for readers!
      if (p->second.readers.size() == 0) // no readers either! go go go.
	return true;

      // Yes, there are some readers.
      // Gentlemen, we are looking for a reader OLDER than tid and NOT expired his timeslice
      // and NOT presently committing.
      
      bool found = false;
      list<trid_t>::iterator r = p->second.readers.begin();

      while( r != p->second.readers.end())
	{
	  // get status of *r
	  map<trid_t, tstat> :: iterator s = status.find(*r);
	  
	  if ( (s->second.stat != tstat::COMMITTING) \
	       && OLDER(*r , tid) \
	       && (difftime(time(NULL),s->second.started) <= TIMESLICE))
	    {
	      return false;	// Yes! the holy grail has been found! you can't write!
	    }

	  r++;			// next reader.
	}

      // If we reached here, there is no transaction that CANNOT be preempted.
      list<trid_t>::iterator k = p->second.readers.begin();

      while( k != p->second.readers.end() )
	{
	  update_status(*k, 0, tstat::PREEMPTED);
	  k++;
	}

      p->second.readers.clear(); // no more readers for this block.
      return true;		// huh kicked out all readers. Go ahead and write sir.
    }

  // Yes, there is a writer. See if tid wins over it.
  trid_t writer = p->second.writer;
  map<trid_t, tstat>::iterator q = status.find(writer);

  if (q == status.end())
    {
      //clearly something went horribly wrong.
      cerr << "You have some brain dead code in here. " << endl;
      exit(1);
    }

  if (q->second.stat == tstat::COMMITTING) // we do NOT EVER preempt committing transactions
    return false;

  if ( YOUNGER(writer,tid) )
    {
      q->second.stat = tstat::PREEMPTED;
      p->second.writer = 0;
      return true;
    }
       
  if (difftime(time(NULL), q->second.started) > TIMESLICE) // writer is older, but run out of time
    {
      q->second.stat = tstat::PREEMPTED;
      p->second.writer = 0;
      return true;
    }
  else
    {
      return false;
    }
}

void
TransactionMonitor::set_writer(int blockno, trid_t tid)
{
  map<int, blockholders>::iterator p = blockmap.find(blockno);

  if ( p == blockmap.end())
    {
      blockholders bh;
      bh.writer = tid;
      blockmap[blockno] = bh;
    }
  else
    {
      (p->second).writer = tid;
    }

  cout<<"Transaction #" << tid << " is a writer for block # " << blockno << endl;

}

void
TransactionMonitor::set_reader(int blockno, trid_t tid)
{
  map<int, blockholders>::iterator p = blockmap.find(blockno);

  if ( p == blockmap.end())
    {
      blockholders bh;
      bh.readers.push_front(tid);
      blockmap[blockno] = bh;
    }
  else
    {
      (p->second).readers.push_front(tid);
    }

  cout<<"Transaction #" << tid << " is a reader for block # " << blockno << endl;
}

bool
TransactionMonitor::check_sole_reader(trid_t tid, int block)
{
  map<int, blockholders> :: iterator p = blockmap.find(block);

  if (p == blockmap.end())
    return false;

  if (p->second.readers.size() > 1)
    return false;

  // check if the sole reader is pid
  if ( tid == p->second.readers.front() )
    return true;
  else
    return false;
}

void 
TransactionMonitor::remove_reader(int block, trid_t tid)
{
  // Remove a reader from the block's readers list
  map<int, blockholders> :: iterator p = blockmap.find(block);

  if (p == blockmap.end())
    return;

  p->second.readers.remove(tid);
}


void
TransactionMonitor::update_status(trid_t tid, int blockno, tstat::status_t s)
{
  map<trid_t, tstat>::iterator p = status.find(tid);

  if ( p != status.end())
    {
      p->second.block_no = blockno;
      p->second.stat = s;
    }

}

// Remove all references to this transaction on the monitor
void
TransactionMonitor::flush_transaction(trid_t tid)
{
  // Get the PID and remove from blockmap;
  cout << "Flushing transaction.." << tid << endl;
  map<trid_t, tstat> :: iterator p = status.find(tid);

  if (p == status.end())
    return;

  // search blockmap
  map<int, blockholders> :: iterator k = blockmap.begin();

  while ( k != blockmap.end())
    {
      if (k->second.writer == tid)
	{
	  k->second.writer = 0;
	}
      else
	{
	  k->second.readers.remove(tid);
 	}
      
      ++k; 			// next block
    }

  // done purging blockmap. Now remove the transaction.
  status.erase(p);
}
