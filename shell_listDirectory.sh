#!/bin/bash

foreachd()
{
    for file in $1/*
    do
        if [ -d $file ]
        then
            echo $file "Directory"
            foreachd $file
        elif [ -f $file ]
        then
            echo $file
        fi
    done
}

if [[ "x$1" == 'x' ]] #说明没有参数，遍历当前目录
then
    foreachd "."
else
    foreachd "$1"
fi
