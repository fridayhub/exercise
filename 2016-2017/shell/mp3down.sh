#!/usr/bin/bash

num=`seq -f %02.0f 40`
mkdir TOEFL词汇
cd TOEFL词汇
for i in $num
do
    basurl=http://download.dogwood.com.cn/online/toeflch/WordList${i}.mp3
    echo $basurl
    wget $basurl
done
