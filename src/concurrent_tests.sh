#!/bin/sh
echo
echo '****** Current tests setup ****'
echo '==============================='
echo 
echo 'Transaction #1 read locks blocks #2 and #3 and write locks block #1'
echo 'Transaction #2 read locks blocks #2 and #3 concurrently and sleeps for a long time'
echo 'Transaction #3 asks for a write lock on #1 and #2'
echo
echo 'The affect is that trn #3 restarts a few times until trn #1 releases its locks'
echo 'trn #2 will be preempted by the monitor since it was taking too long to complete'
echo 'trn #3 will be granted locks after #1 completes'
echo
echo 'Hit <Enter> to start concurrent tests'
read $k

./ct1&
sleep 2
./ct2&
sleep 2
./ct3&


