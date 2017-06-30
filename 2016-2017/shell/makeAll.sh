#!/bin/sh  
MAINDIR=application/linux
list_alldir(){  
    for sofile in `ls -a $1`  
    do  
        if [ x"$sofile" != x"." -a x"$sofile" != x".." ];then  
            if [ -d "$1/$sofile" ];then  
                echo "$1/$sofile" 
                cd $1/$sofile
                #rm *.so
                if [ -f "Makefile" ];then
                    make clean && make BUILD=release -j$2
                    if [ $? -ne 0 ];then
                        exit
                    fi
                    mv -f *.so /somedir
                fi
                cd - 
                list_alldir "$1/$sofile" $2 
            fi  
        fi  
    done  
} 
cd $MAINDIR
make clean && make -j$1
cp -f somedir to somedir 
cd -
list_alldir ./application/src/Protocol_So $1
