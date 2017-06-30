#/bin/sh

ps -ef |grep update.sh |grep -v grep
if [ $? -eq 0 ]
then
    exit
fi

UPDATEFILE="/data/update/tandaUpdate.tar.gz"
MD5SUM="/data/update/tandaUpdate.tar.gz.md5sum"
INSTALLDIR="/data"
UPDATEDIR="/data/update"
#file check
cd $UPDATEDIR
/data/bin/md5sum --check $MD5SUM
if [ $? -eq 0 ];then
    echo "$UPDATEFILE OK!"
else
    echo "$UPDATEFILE not match !!"
    echo 1 > /data/uplog
    exit
fi

tar -xzvf tandaUpdate.tar.gz
if [ $? -ne 0 ];then
    echo 2 > /data/uplog
    exit
fi
cd $UPDATEDIR
cd tandaUpdate
/bin/sh runUpdate.sh
if [ $? -ne 0 ];then
    echo 3 > /data/uplog
    exit
fi
   
sync

echo 0 > /data/uplog
sleep 5
echo 5 > /data/uplog
cd $UPDATEDIR
rm -rf tanda*
sync
reboot
