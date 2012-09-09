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
// A log manager for this transaction processing system. This logger is able to
// log actions to the logfile and also rollback the database to its previous state
// by reading log actions performed by a particular transaction.

# ifndef LOGMANAGER_HH
# define LOGMANAGER_HH

# include <config.h>
# include <string>
# include <fstream>
# include <list>

using namespace std;
enum logAction { LREAD, LWRITE, LALLOCATE, LABORT, LRELEASE, LBEGIN, LEND, LCOMMIT };  

struct logrecord
{
  trid_t tid;			// Transaction ID
  logAction action;		// What action has been performed
  int block;			// Block affected in this transaction
  char oindex;			// old Index file entry for this block
  byte oldval[BLOCKSIZE];       // oldvalue in case of write and release
};
  
class LogManager
{
private:
  fstream logstream;		// Ascii log stream.
  int storefd;			// The datastore
  int indexfd;			// Index file
  int logfd;			// Binary log file
  char action_char(logAction action);
  string val_string(void *val);
  void undo_rec(list<logrecord>& log);
  void write_abort_rec(trid_t tid);
public:

  LogManager();
  ~LogManager();

  void log(trid_t tid, logAction action, int block=0, char oldindex=' ', void *oldval=NULL);
  void rollback(trid_t tid);	// rollback a transaction

  // This is a FULL recovery function.
  void doRecovery(char *logfile=LOGFILE);
};

# endif
