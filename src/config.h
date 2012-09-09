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
// Configuration options for Transactional store.

# ifndef CONFIG_H
# define CONFIG_H

// some standard includes
# include <sys/types.h>
# include <unistd.h>
# include <fcntl.h>

# define BLOCKSIZE  4096	/* Change this before compilation if you need a different blocksize */
# define NBLOCKS    30
# define MAXTIMEOUT 32		/* Maximum timeout for a transaction */
# define TIMESLICE  4		/* This is the time slice allocated to each transaction */

# define ALOGFILE   "transactions.log"
# define LOGFILE    "datalog.log"
# define STOREFILE  "datastore.db"
# define INDEXFILE  "dataindex.ix"

# define TPM_PORT    2983
# define TPM_HOST    "localhost"

// A transaction ID type.
typedef unsigned long trid_t;

// A byte type for memory blocks
typedef unsigned char byte;


// UNCOMMENT these two lines if you want to observe rollback and/or recovery
// See README for more information

//# define OBSERVE_ROLLBACK
//# define OBSERVE_RECOVERY



# endif
