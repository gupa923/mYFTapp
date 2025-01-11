#!/bin/bash

./myFTclient -l -a 127.0.0.1 -p 12000 -f folder1/ > "test_out/list_test2_f1.txt" 2>&1 &

./myFTclient -l -a 127.0.0.1 -p 12000 -f prova_write/ > "test_out/list_test2_f2.txt" 2>&1 & 

./myFTclient -l -a 127.0.0.1 -p 12000 -f myRemoteRoot/ > "test_out/list_test2_f3.txt" 2>&1 &

wait

echo "test completato"