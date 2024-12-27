#!/bin/bash

./myFTclient -r -a 127.0.0.1 -p 12000 -f folder1/file_prova.txt -o rw_test/file_prova.txt > "test_out/rw_test1_f1.txt" 2>&1

./myFTclient -w -a 127.0.0.1 -p 12000 -f file2.txt -o folder1/file_prova.txt > "test_out/rw_test1_f2.txt" 2>&1

wait

echo "test completato"