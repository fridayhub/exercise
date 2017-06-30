#/bin/sh
IPFILE=/data/modelip.conf
IPFILEBK=/data/.tmpip.conf
RCLOCAL=/etc/rc.d/rc.local

read_config_file()
{
    #read config file
    while read line;
    do
        eval "$line"
    done < $IPFILE 
}

change_to_static()
{

    #sed -i '2s/^/#&/g' /var/spool/cron/crontabs/root
    sed -i 's/.*ipChange.*/\#\*\/2\ \*\ \*\ \*\ \*\ \/etc\/ipChange/' /etc/crontabs/root
    #del
    #sed '2s/^.//' /var/spool/cron/crontabs/root
    sed -i 's/.*ipChange.*/\#\/etc\/ipChange\ \&/' /data/demo.sh
    sed -i 's/.*net\.sh.*/#\/bin\/sh\ \/etc\/net.sh\ \&/g' $RCLOCAL
    sed -i "s/.*ifconfig.*/\ifconfig\ eth0\ $ipaddress\ netmask\ $netmask\ up/g" $RCLOCAL 
    #ifconfig eth0 $ipaddress netmask $netmask up
    sed -i '/route/d' $RCLOCAL
    sed -i '$a\route\ add\ default\ gw\ '$gateway'' $RCLOCAL
    sed -i "1s/^names.*/nameserver\ $dns1/" /etc/resolv.conf
    sed -i "2s/^names.*/nameserver\ $dns2/" /etc/resolv.conf
}

change_to_dynamic()
{
    #cp -p /etc/crontabs/root /var/spool/cron/crontabs
    sed -i 's/.*ipChange.*/\*\/2\ \*\ \*\ \*\ \*\ \/etc\/ipChange/' /etc/crontabs/root
    sed -i 's/.*ipChange.*/\/etc\/ipChange\ \&/' /data/demo.sh
    sed -i 's/.*net\.sh.*/\/bin\/sh\ \/etc\/net.sh\ \&/g' $RCLOCAL 
    sed -i 's/.*ifconfig.*/ifconfig\ eth0:0\ 192.168.2.40\ netmask\ 255.255.255.0\ up/g' $RCLOCAL
    sed -i '/route/d' $RCLOCAL
}

while :
do
    sleep 1
    if [ ! -f $IPFILEBK ]
    then
        cat $IPFILE > $IPFILEBK
    fi

    cmp $IPFILEBK $IPFILE

    if [ $? -eq 0 ]
    then
        echo "same file not modified" >/dev/null
    else
        echo "file has changed"
        read_config_file
        if [ $autoip -eq 0 ];
        then
            change_to_static
            rm -f $IPFILEBK
            sync && sleep 1 
            reboot
        elif [ $autoip -eq 1 ];
        then
            change_to_dynamic
            rm -f $IPFILEBK
            sync && sleep 1
            reboot
        fi
    fi

done
