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
// Implementation of a Log manager

# include <LogManager.hh>
# include <iostream>
# include <sstream>
# include <list>
# include <map>

LogManager::LogManager()
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

  if ( (logfd = open(LOGFILE, O_RDWR | O_SYNC)) == -1 )
    {
      cerr << __FILE__ << ":Cannot open the log file " << LOGFILE << endl;
      exit(1);
    }
  
  
  logstream.open(ALOGFILE, fstream::out | fstream::app);
  if (logstream.fail())
    {
      cerr << __FILE__ << ":Cannot open the ascii log file " << ALOGFILE << endl;
      exit(1);
    }      
}

LogManager::~LogManager()
{
  close(storefd);
  close(indexfd);
  close(logfd);
  logstream.close();
}

void
LogManager::log(trid_t tid, logAction action, int block, char oldindex, void *oldval)
{
  logrecord lgr;
  bzero(&lgr, sizeof(lgr));
  
  logstream << tid << "\t";
  lgr.tid = tid;
  
  logstream << action_char(action) << "\t";
  lgr.action = action;
  
  if (block != 0)
    {
      logstream << block << "\t";
      lgr.block = block;
    }

  logstream << oldindex << "\t";
  lgr.oindex = oldindex;
  
  
  if (oldval != NULL)
    {
      logstream << val_string(oldval) << "\t";

      byte *ptr = (byte *)oldval;
      for(int i=0; i<BLOCKSIZE; ++i)
	lgr.oldval[i] = ptr[i];
    }
  
  logstream << "\n";
  logstream << flush;

  if (::write(logfd, &lgr, sizeof(lgr)) == -1)
    {
      cerr << "Could not write log file!" << endl;
      exit(1);
    }
}

void
LogManager::rollback(trid_t tid)
{
  cout << "Logmanager starts to rollback transaction # " << tid << endl;
  // collect all log records of this trn
  
  list<logrecord> log;
  int lfd;

  if ( (lfd = open(LOGFILE, O_RDONLY)) == -1 )
    {
      cerr << __FILE__ << ":Cannot open the log file " << LOGFILE << endl;
      exit(1);
    }

  lseek(lfd, 0, SEEK_END);
  bool done = false;
  
  while(!done)
    {
      logrecord lgr;
      lseek(lfd, -sizeof(lgr), SEEK_CUR);

      if (::read(lfd, &lgr, sizeof(lgr)) == -1)
	{
	  cerr << "Could not read log record! " << endl;
	  exit(1);
	}

      if ( lgr.tid == tid)
	{
	  if ( lgr.action == LEND)
	    {
	      cout << "End has already been written to this transaction. Nothing to rollback. " << endl;
	      return;
	    }

	  if (lgr.action == LBEGIN)
	    done = true;
	  
	  log.push_back(lgr);
	}

      lseek(lfd, -sizeof(lgr), SEEK_CUR); // reset postion.
    }

  undo_rec(log);		// This function undoes all actions in this log record

  // Now write an ABORT to the log to indicate that server rolled back this trn online.

  cout << "Writing an ABORT record to log " << endl;
  lseek(logfd,0,SEEK_END);
  
  logrecord abrec;
  bzero(&abrec, sizeof(abrec));
  
  abrec.tid = log.front().tid;
  abrec.action = LABORT;


  if(::write(logfd, &abrec, sizeof(abrec)) == -1)
    {
      cerr << "Error writing to log!" << endl;
      exit(1);
    }

  logstream << log.front().tid << "\t";
  logstream << 'T';
  logstream << '\n';
  logstream << flush;

  ::close(lfd);
}

// This function wades through the entire log and recovers the datastore.
// It will use it's own logfile copy for recovery. You can use a different
// one for max concurrency.
void
LogManager::doRecovery(char *logfile)
{
  cout << "Checking integrity of datastore.." << endl;
  map<trid_t, list<logrecord> *> bts;	// broken transactions
  int nBts = 0;			// start with zero broken transactions

  int lfd;
  logrecord lgr;
  bzero(&lgr, sizeof(lgr));
  
  if ( (lfd = open(logfile, O_RDONLY)) == -1 ) 
    {
      cerr << __FILE__ << ":Cannot open the log file " << logfile << endl;
      exit(1);
    }

  while( ::read( lfd, &lgr, sizeof(lgr)) == sizeof(lgr) )
    {
      // If tid is new in the map, it will be added a new list will be made.
      map<trid_t, list<logrecord>* > :: iterator p = bts.find(lgr.tid);
      if ( p == bts.end())
	bts[lgr.tid] = new list<logrecord>;
      
      // If this record is a COMMIT or an ABORT, we can throw the list away!
      if (lgr.action == LEND || lgr.action == LABORT)
	{
	  delete bts[lgr.tid];	// delete the list
	  bts.erase(lgr.tid);
	  continue;
	}
      
      bts[lgr.tid]->push_back(lgr);
      bzero(&lgr, sizeof(lgr));	// reset for next time.
    }

  // Finished collecting broken transactions. Report them and start rollback each record
  cout << bts.size() << " broken transaction(s)." << endl;
  if (bts.size() > 0 )
    {
      cout << "Fixing store.. " << endl;
      map<trid_t,list<logrecord>* > :: iterator p = bts.begin();
      while (p != bts.end())
	{
	  undo_rec (*(p->second));

	  // Write an ABORT at the end of the log for this record
	  lseek(logfd,0,SEEK_END);
	  logstream.seekp(0,ios_base::end);
	  cout << "writing ABORT to log." << endl;
	  write_abort_rec(p->second->front().tid);
	  
	  p++;
	}
    }
	  
  ::close(lfd);			// This is the last step.
}

void
LogManager::undo_rec(list<logrecord>& log)
{
  cout << "UNDO records collected.." << endl;
  list<logrecord> :: iterator p = log.begin();
  while( p != log.end())
    {
      cout << p->tid << "\t";
      cout << action_char(p->action) << endl;

      switch(p->action)
	{
	case LWRITE:
	  // write back the old value in the datastore.
	  cout << "Undoing a WRITE operation on block " << p->block << endl;
	  lseek(storefd, ((p->block)-1)*BLOCKSIZE, SEEK_SET);

	  if (::write(storefd, p->oldval, BLOCKSIZE) == -1)
	    {
	      cerr << "Error restoring datastore! " << endl;
	      exit(1);
	    }
	  break;

	case LALLOCATE:
	  {
	    cout << "Undoing an ALLOCATE operation on block " << p->block << endl;
	    char unalloc = 'U';
	    // Roll back an allocate
	    lseek(indexfd, (p->block)-1, SEEK_SET);
	    
	    if(::write(indexfd, &unalloc, 1) == -1)
	      {
		cerr<< " Error restoring dataindex!" << endl;
		exit(1);
	      }

	    // print the oldval
	    break;
	  }
	  
	case LRELEASE:
	  cout << " Undoing a RELEASE operation on block " << p->block << endl;
	  // write back the old index into the index file
	  lseek(indexfd, (p->block)-1, SEEK_SET);

	  if (::write(indexfd, &(p->oindex), 1) == -1)
	    {
	      cerr << "Error restoring data index!" << endl;
	      exit(1);
	    }

	  if (p->oindex == 'A')
	    {
	      lseek(storefd, ((p->block)-1)*BLOCKSIZE, SEEK_SET);

	      if (::write(storefd, p->oldval, BLOCKSIZE) == -1)
		{
		  cerr<< "Error restroing data store!" << endl;
		}
	    }

	default:
	  break;
	}
      p++;			// next log record
    }

}

void
LogManager::write_abort_rec(trid_t tid)
{
  // Write an abort record at the current postion of the log file.
  logrecord abrec;
  bzero(&abrec, sizeof(abrec));
  
  abrec.tid = tid;
  abrec.action = LABORT;


  if(::write(logfd, &abrec, sizeof(abrec)) == -1)
    {
      cerr << "Error writing to log!" << endl;
      exit(1);
    }

  logstream << tid << "\t";
  logstream << 'T';
  logstream << '\n';
  logstream << flush;
}

char
LogManager::action_char(logAction action)
{
  switch(action)
    {
    case LREAD: return 'R';
    case LWRITE: return 'W';
    case LALLOCATE: return 'A';
    case LABORT: return 'T';
    case LRELEASE: return 'S';
    case LBEGIN: return 'B';
    case LEND: return 'E';
    case LCOMMIT: return 'C';
    }
}

string
LogManager::val_string(void *val)
{
  stringstream ss;
  byte *ptr = (byte *)val;
  
  for(int i=0; i<BLOCKSIZE; ++i)
      ss << ptr[i];

  return ss.str();
}
