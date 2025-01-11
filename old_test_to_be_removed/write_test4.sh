#!/bin/bash

./myFTclient -w -a 127.0.0.1 -p 12000 -f file_prova.txt -o prova_write/dir2/file_prova.txt > "test_out/write_test4_f1.txt" 2>&1 &

./myFTclient -w -a 127.0.0.1 -p 12000 -f file_prova.txt -o prova_write/dir2/file_prova.txt > "test_out/write_test4_f2.txt" 2>&1 &

./myFTclient -w -a 127.0.0.1 -p 12000 -f file2.txt -o prova_write/dir2/file_prova.txt > "test_out/write_test4_f3.txt" 2>&1 &

wait

echo "test completato"