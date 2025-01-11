#!/bin/bash

./myFTclient -r -a 127.0.0.1 -p 12000 -f folder1/file_bo.txt -o prova_read/file_bo.txt > "test_out/read_test1_f1.txt" 2>&1

./myFTclient -r -a 127.0.0.1 -p 12000 -f folder1/file_prova.txt -o prova_read/file_prova.txt > "test_out/read_test1_f2.txt" 2>&1

./myFTclient -r -a 127.0.0.1 -p 12000 -f folder1/folder4/file2.txt -o prova_read/file2.txt > "test_out/read_test1_f3.txt" 2>&1

wait

echo "test completato"