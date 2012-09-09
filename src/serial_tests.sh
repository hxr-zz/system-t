#!/bin/sh

# This script executes all serial tests

echo '------------ serial test #1 - Allocates a few blocks ------- '
./st1

echo
echo '------------ serial test #2 - allocate, read, write,'
echo  'read after allocate, write, transactions in a loop -----'
./st2

echo
echo '----------- serial test #3 - Read, write, Release,'
echo 'Release after allocate, read after write -------- '
./st3

echo
echo '---------- serial test #4 - Write after read (Lock Upgrade) - Extra credit. ---'
./st4

echo
echo '---------- serial test #5 - Abort a tranaction by calling T.abort() ---------'
./st5
