#!/bin/bash

./myFTclient -r -a 127.0.0.1 -p 12000 -f folder1/folder4/file2.txt -o rw_test/dir/file2.txt > "test_out/rw_test2_f1.txt" 2>&1 &

./myFTclient -w -a 127.0.0.1 -p 12000 -f file_prova.txt -o folder1/folder4/file2.txt > "test_out/rw_test2_f2.txt" 2>&1 &

wait

echo "test completato"