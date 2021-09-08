#!/bin/bash

cd ./tag_service && make unload
cd ./systbl_hack && make unload
cd ../  && make clean


dmesg | grep 'SYSCALL TABLE HACKING SYSTEM\|TAG-SERVICE\|tag-device-driver'

echo "removed succesfully\n"