#!/bin/sh
source /etc/profile
echo "Start complie app code....."
cd code
make
mv app/httpPost app/ipChange ../etc
mv app/macDemo ../usr/bin/
mv app/keyEvent ../etc/
mv app/readCpuId ../usr/bin/
chmod +x ../etc/keyEvent ../etc/ipChange
mv -f cgi/config.cgi cgi/login.cgi cgi/waterconfig.cgi ../usr/local/boa/cgi-bin/
cp -f cgi/Tanda.conf ../data/
cp -f cgi/Tanda.conf.default ../data/Tanda.conf.default
cp -f cgi/config.html ../usr/local/boa/
cp -f cgi/index.html ../usr/local/boa/
cp -f cgi/watersys.html ../usr/local/boa/
cp -f cgi/ipconfig.cgi ../usr/local/boa/cgi-bin/
cp -f cgi/ipconfig.html ../usr/local/boa/

cd ../
chmod 777 usr/local/boa/cgi-bin/*
chmod +x etc/httpPost etc/ipChange usr/bin/macDemo
echo " "
echo "app and cgi updata ok"
echo " "

mknod dev/console c 5 1
mknod dev/null c 1 3
mknod dev/tty c 5 0
chown root:root -R *
chmod 666 dev/console
chmod 666 dev/null
chmod 666 dev/tty
chmod 777 etc
chmod 777 tmp
chmod 777 -R data
chmod 777 -R usr/local/boa/
if	[ -f ./rootfs.tar.bz2 ]; then
	rm rootfs.tar.bz2
fi
tar -jcvf rootfs.tar.bz2 * --exclude=pack-rootfs.sh --exclude=rootfs.tar.gz --exclude=code --exclude=createSD
