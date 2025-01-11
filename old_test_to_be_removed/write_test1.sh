#!/bin/bash

./myFTclient -w -a 127.0.0.1 -p 12000 -f file_prova.txt -o prova_write/file_prova.txt > "test_out/write_test1_f1.txt" 2>&1

./myFTclient -w -a 127.0.0.1 -p 12000 -f file_prova.txt -o prova_write/file_prova2.txt > "test_out/write_test1_f2.txt" 2>&1

./myFTclient -w -a 127.0.0.1 -p 12000 -f file2.txt -o prova_write/file2.txt > "test_out/write_test1_f3.txt" 2>&1

wait

echo "test completato"