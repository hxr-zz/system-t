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
// This is a general purpose Transaction processing monitor
// Requires TCP/IP network stack to be installed along with POSIX.1b
// programming interface.

# ifndef MONITOR_HH
# define MONITOR_HH

# include <config.h>
# include <trq.hh>
# include <Transport.hh>
# include <list>
# include <map>
# include <limits.h>
# include <time.h>
# include <LogManager.hh>

# ifdef OBSERVE_RECOVERY
# include <signal.h>		// for raise(SIGKILL)
# endif

using namespace std;


// A younger transaction always has a higher transaction ID.
# define YOUNGER(t1, t2)  (t1 > t2)
# define OLDER(t1,t2) (t1 < t2)

//Transaction status structure. Records status of transactions.
struct tstat
{
  enum status_t
    { BEGUN, READING, WRITING, ALLOCATED,
      RELEASING, ABORTING, RESTARTING,
      COMMITTING, ENDED, PREEMPTED
    };
  
  pid_t  pid;			        // Who is running this transaction
  int block_no;			        // currently affected block
  time_t started;		        // when did this transaction start?
  status_t stat;		        // What is the transaction status
};

// Stores the state of a block.
struct blockholders
{
  list<trid_t> readers;		// which transactions are reading this block
  trid_t writer;		// which transaction is writing this block
  blockholders() { writer=0; }
};

class TransactionMonitor
{
private:
  int sockfd;
  unsigned int port;
  list<int> clients;		        // list of currently active clients.

  trid_t next_tid;		        // next available TID
  
  map<int, trid_t> transactions;        // Maps client fd's to their transaction IDs.
  map<trid_t, tstat> status;            // status of each live transaction.
  map<int, blockholders> blockmap;	// Readers and writers.

  TransportLayer tcp;		        // The tcp layer provides all communication
  LogManager logger;		        // Logger is used for logbased recovery
  
  void reply(trq& t, int clientfd);     // send a reply to client process.
  void process_clientrq(trq& t, int clientfd); 
  void client_disconnect(int fd);	// A client has disconnected. Check consistency.

  bool can_write(trid_t t, int b);      // check if t can write b
  bool can_read (trid_t  t, int b);      // check if t can read  b  

  void 	set_reader(int b, trid_t tid);	// set tid as a reader to block b
  void 	set_writer(int b, trid_t tid);  // set tid as a writer to block b

  bool check_sole_reader(trid_t tid, int block);
  void remove_reader(int block, trid_t tid);
  
  void update_status(trid_t tid, int block, tstat::status_t s);	// update the status of a transaction
  void flush_transaction(trid_t tid);

  // Request Handlers
  void allocate_rq(trq& t, int whosent);
  void read_rq(trq& t, int whosent);
  void write_rq(trq& t, int whosent);
  void release_rq(trq& t, int whosent);
  void abort_rq(trq& t, int whosent);
  void end_rq(trq& t, int whosent);
  void commit_rq(trq& t, int whosent);
  void restart_rq(trq& t, int whosent);
  
public:

  TransactionMonitor();
  ~TransactionMonitor();
  
  void start ();		// start the monitor
  void setPort(unsigned int port);
};

# endif
