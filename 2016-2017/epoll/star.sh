#!/bin/bash

for i in `seq 10000`:
do
    echo $i
    ./client&
done
