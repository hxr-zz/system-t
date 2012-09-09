CC=g++
IFLAGS= -I.
CFLAGS= -c

all:   tpmd initstore recover libtransaction.a tests
	./initstore

tests: serial_tests concurrent_tests

serial_tests: st1 st2 st3 st4 st5
concurrent_tests: ct1 ct2 ct3

# concurrent tests
ct3: ct3.cc libtransaction.a
	$(CC) $(IFLAGS) ct3.cc libtransaction.a -o ct3

ct2: ct2.cc libtransaction.a
	$(CC) $(IFLAGS) ct2.cc libtransaction.a -o ct2

ct1: ct1.cc libtransaction.a
	$(CC) $(IFLAGS) ct1.cc libtransaction.a -o ct1

# serial tests
st5: st5.cc libtransaction.a
	$(CC) $(IFLAGS) st5.cc libtransaction.a -o st5

st4: st4.cc libtransaction.a
	$(CC) $(IFLAGS) st4.cc libtransaction.a -o st4

st3: st3.cc libtransaction.a
	$(CC) $(IFLAGS) st3.cc libtransaction.a -o st3

st2: st2.cc libtransaction.a
	$(CC) $(IFLAGS) st2.cc libtransaction.a -o st2

st1: st1.cc libtransaction.a
	$(CC) $(IFLAGS) st1.cc libtransaction.a -o st1


initstore: initstore.cc config.h
	$(CC) $(IFLAGS) initstore.cc -o initstore

recover: recover.cc LogManager.o
	$(CC) $(IFLAGS) recover.cc Transaction.o Transport.o LogManager.o -o recover

tpmd: Monitor.o tpmd.cc Transaction.o Transport.o LogManager.o
	$(CC) $(IFLAGS) tpmd.cc Monitor.o Transport.o LogManager.o -o tpmd

libtransaction.a: Transaction.o Transport.o LogManager.o
	ar rcs libtransaction.a Transaction.o Transport.o LogManager.o

Transaction.o: Transaction.cc Transaction.hh config.h Transport.hh LogManager.o Transport.o
	$(CC) $(IFLAGS) $(CFLAGS) Transaction.cc -o Transaction.o 

LogManager.o: LogManager.cc LogManager.hh config.h
	$(CC) $(IFLAGS) $(CFLAGS) LogManager.cc -o LogManager.o

Monitor.o: Monitor.cc Monitor.hh config.h 
	$(CC) $(IFLAGS) $(CFLAGS) Monitor.cc -o Monitor.o 

Transport.o: Transport.hh Transport.cc
	$(CC) $(IFLAGS) $(CFLAGS) Transport.cc -o Transport.o

clean:  bclean
	rm -f datastore.db dataindex.ix datalog.log transactions.log
	rm -f tpmd initstore recover libtransaction.a
	rm -f st1 st2 st3 st4 st5 ct1 ct2 ct3 concurrent_tests serial_tests
	rm -f *.o 

bclean:
	rm -f *~

test:	atran btran ctran dtran etran dtran 
	./t1.sh
	./t2.sh
	./t3.sh

