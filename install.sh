#!/bin/bash

cd ./tag_service/systbl_hack && make
make load
cd ../ && make
make load
dmesg | grep 'SYSCALL TABLE HACKING SYSTEM\|TAG-SERVICE\|tag-device-driver'
# shellcheck disable=SC2046
mknod /dev/mydev c $(cat /sys/module/tag_service/parameters/major_number) 0
# shellcheck disable=SC2028
echo "setup done succesfully\n"