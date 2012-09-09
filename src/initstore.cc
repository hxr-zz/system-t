# include <iostream>
# include <fstream>
# include <config.h>
using namespace std;

int
main(int argc, char **argv)
{
  // create the datastore file.
  ofstream store(STOREFILE);

  cout << "Initializing " << NBLOCKS << " blocks of size " << BLOCKSIZE << " bytes...";
  for (int i=0; i<NBLOCKS; ++i)
    for (int j=0; j<BLOCKSIZE;  ++j)
      store<<"0";

  cout << "done." << endl;
  store.close();

  // create an index file.
  ofstream idx (INDEXFILE);

  cout << "Initializing dataindex...";
  for (int i=1; i<=NBLOCKS; ++i)
    idx<<"U";

  cout << "done." << endl;
  idx.close();

  ofstream log (LOGFILE);
  log.close();

  ofstream alog(ALOGFILE);
  alog.close();

  cout << "created ascii and binary logfiles." << endl;
}
