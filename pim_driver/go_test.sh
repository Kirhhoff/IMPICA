#!/bin/bash

mknod /dev/pimbt c 70 0
insmod ./test-driver.ko
chmod 666 /dev/pimbt