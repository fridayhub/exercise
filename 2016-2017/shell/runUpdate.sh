#!/bin/sh
#ps | grep demo.sh| grep -v grep
#if [ $? -eq 0 ]
#then
#    killall demo.sh
#fi
#
#ps | grep SteamSensor | grep -v grep
#if [ $? -eq 0 ]
#then
#    killall SteamSensor
#fi
pkill demo.sh
pkill SteamSensor

#rm -f /data/SteamSensor
#rm -f /data/*.so
#rm -f /data/Tanda.conf
#rm -f /data/Tanda.conf.default
#cp -f GST5000.so /data
#cp -f demo.sh /data/
cp -f SteamSensor /data/
#cp -f Tanda3016_485.so /data/
#cp -f Tanda3016_232.so /data/
#cp -f Tanda050.so /data/
#cp -f Tanda.conf /data/
#cp -f JADE_BIRD11S_new.so /data/
#cp -f JADE_BIRD11S_old.so /data/
#cp -f ModbusWater.so /data
#cp -f Tanda_485_test.so /data
#cp -f Tanda_232_test.so /data
cp -f Xinhaosi.so /data

#Update soft version
sed  -i 's/^SoftVersion.*$/SoftVersion=v1.16/g' /data/Tanda.conf
#cp -f Tanda.conf /data/Tanda.conf.default
#cp -f config.cgi /usr/local/boa/cgi-bin/
#cp -f config.html /usr/local/boa/config.html
#cp -f index.html /usr/local/boa/index.html
#cp -f login.cgi /usr/local/boa/cgi-bin/login.cgi
#chmod +x /usr/local/boa/cgi-bin
#rm -f /etc/ipChange
#rm -f /etc/httpPost
#cp -f ipChange /etc/ipChange
#cp -f httpPost /etc/httpPost
#chmod +x /etc/ipChange
#chmod +x /etc/httpPost
#cp -f root /etc/crontabs/root
#cp -f readCpuId /usr/bin/readCpuId
#sed -i '$a #save cpuid' /etc/rc.d/rc.local
#sed -i '$a /usr/bin/readCpuId' /etc/rc.d/rc.local
#sed -i '$a sync' /etc/rc.d/rc.local
#sed -i '$a chmod 777 /etc/cpuid' /etc/rc.d/rc.local
#cp -f zImage /media/mmcblk1p1/
#cp -f imx6ul-14x14-evk.dtb /media/mmcblk1p1/


chmod 777 /data/*
