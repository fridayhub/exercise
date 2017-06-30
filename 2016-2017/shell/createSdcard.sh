#!/bin/bash

# Determine the absolute path to the executable
# EXE will have the PWD removed so we can concatenate with the PWD safely
PWD=`pwd`
EXE=`echo $0 | sed s=$PWD==`
EXEPATH="$PWD"/"$EXE"
export PATH=/home/hakits/imx6ul/OKMX6ulUserDiskLinux-20160407/Tools/fsl-linaro-toolchain/bin:$PATH
clear
cat << EOM

################################################################################

This script will create a bootable SD card from custom or pre-built binaries.

The script must be run with root permissions and from the bin directory of
the SDK

Example:
 $ sudo ./create-sdcard.sh

Formatting can be skipped if the SD card is already formatted and
partitioned properly.

################################################################################

EOM

AMIROOT=`whoami | awk {'print $1'}`
if [ "$AMIROOT" != "root" ] ; then

	echo "	**** Error *** must run script with sudo"
	echo ""
	exit
fi

# find the avaible SD cards
echo " "
echo "Availible Drives to write images to: "
echo " "
ROOTDRIVE=`mount | grep 'on / ' | awk {'print $1'} |  cut -c6-8`
echo "#  major   minor    size   name "
cat /proc/partitions | grep -v $ROOTDRIVE | grep '\<sd.\>' | grep -n ''
echo " "

ENTERCORRECTLY=0
while [ $ENTERCORRECTLY -ne 1 ]
do
	read -p 'Enter Device Number: ' DEVICEDRIVENUMBER
	echo " "
	DEVICEDRIVENAME=`cat /proc/partitions | grep -v 'sda' | grep '\<sd.\>' | grep -n '' | grep "${DEVICEDRIVENUMBER}:" | awk '{print $5}'`

	DRIVE=/dev/$DEVICEDRIVENAME
	DEVICESIZE=`cat /proc/partitions | grep -v 'sda' | grep '\<sd.\>' | grep -n '' | grep "${DEVICEDRIVENUMBER}:" | awk '{print $4}'`


	if [ -n "$DEVICEDRIVENAME" ]
	then
		ENTERCORRECTLY=1
	else
		echo "Invalid selection"
	fi

	echo ""
done

echo "$DEVICEDRIVENAME was selected"
#Check the size of disk to make sure its under 16GB
if [ $DEVICESIZE -gt 17000000 ] ; then
cat << EOM

################################################################################

		**********WARNING**********

	Selected Device is greater then 16GB
	Continuing past this point will erase data from device
	Double check that this is the correct SD Card

################################################################################

EOM
	ENTERCORRECTLY=0
	while [ $ENTERCORRECTLY -ne 1 ]
	do
		read -p 'Would you like to continue [y/n] : ' SIZECHECK
		echo ""
		echo " "
		ENTERCORRECTLY=1
		case $SIZECHECK in
		"y")  ;;
		"n")  exit;;
		*)  echo "Please enter y or n";ENTERCORRECTLY=0;;
		esac
		echo ""
	done

fi

echo ""



DRIVE=/dev/$DEVICEDRIVENAME

echo "Checking the device is unmounted"
#unmount drives if they are mounted
unmounted1=`df | grep '\<'$DEVICEDRIVENAME'1\>' | awk '{print $1}'`
unmounted2=`df | grep '\<'$DEVICEDRIVENAME'2\>' | awk '{print $1}'`
unmounted3=`df | grep '\<'$DEVICEDRIVENAME'3\>' | awk '{print $1}'`
if [ -n "$unmounted1" ]
then
	echo " unmounted ${DRIVE}1"
	sudo umount -f ${DRIVE}1
fi
if [ -n "$unmounted2" ]
then
	echo " unmounted ${DRIVE}2"
	sudo umount -f ${DRIVE}2
fi
if [ -n "$unmounted3" ]
then
	echo " unmounted ${DRIVE}3"
	sudo umount -f ${DRIVE}3
fi
echo ""
# check to see if the device is already partitioned
SIZE1=`cat /proc/partitions | grep -v 'sda' | grep '\<'$DEVICEDRIVENAME'1\>'  | awk '{print $3}'`
SIZE2=`cat /proc/partitions | grep -v 'sda' | grep '\<'$DEVICEDRIVENAME'2\>'  | awk '{print $3}'`
SIZE3=`cat /proc/partitions | grep -v 'sda' | grep '\<'$DEVICEDRIVENAME'3\>'  | awk '{print $3}'`
SIZE4=`cat /proc/partitions | grep -v 'sda' | grep '\<'$DEVICEDRIVENAME'4\>'  | awk '{print $3}'`
echo "${DEVICEDRIVENAME}1  ${DEVICEDRIVENAME}2   ${DEVICEDRIVENAME}3"
echo $SIZE1 $SIZE2 $SIZE3 $SIZE4
echo ""

PARTITION="0"
if [ -n "$SIZE1"  ] ; then
	if  [ "$SIZE1" -gt "72000" ]
	then
		PARTITION=1
		PARTS=1
	fi
else
	echo "SD Card is not correctly partitioned"
	PARTITION=0
	PARTS=0
fi


#Partition is found
if [ "$PARTITION" -eq "1" ]
then
cat << EOM

################################################################################

   Detected device has $PARTS partitions already

   Re-partitioning will allow the choice of 1 partitions

################################################################################

EOM

	ENTERCORRECTLY=0
	while [ $ENTERCORRECTLY -ne 1 ]
	do
		read -p 'Would you like to re-partition the drive anyways [y/n] : ' CASEPARTITION
		echo ""
		echo " "
		ENTERCORRECTLY=1
		case $CASEPARTITION in
		"y")  echo "Now partitioning $DEVICEDRIVENAME ...";PARTITION=0;;
		"n")  echo "Abort partitioning";
				exit ;;
		*)  echo "Please enter y or n";ENTERCORRECTLY=0;;
		esac
		echo ""
	done

fi



PARTITION=1
#Section for partitioning the drive

#create only 1 partitions
if [ "$PARTITION" -eq "1" ]
then

# Set the PARTS value as well
PARTS=1
cat << EOM

################################################################################

		Now making 1 partitions

################################################################################

EOM
dd if=/dev/zero of=$DRIVE bs=1024 count=1

SIZE=`fdisk -l $DRIVE | grep Disk | awk '{print $5}'`

echo DISK SIZE - $SIZE bytes

CYLINDERS=`echo $SIZE/255/63/512 | bc`

#sfdisk -D -H 255 -S 63 -C $CYLINDERS $DRIVE << EOF
#,,0x0C,*
sfdisk --force -uM $DRIVE << EOF
10,500,0x0C
EOF

cat << EOM

################################################################################

		Partitioning Boot

################################################################################
EOM
	mkfs.vfat -F 32 -n "TandaBoot" ${DRIVE}1
fi




#Add directories for images
export START_DIR=$PWD
export PATH_TO_SDBOOT=boot
#export PATH_TO_SDROOTFS=rootfs
export PATH_TO_TMP_DIR=$START_DIR/tmp


echo " "
echo "Mount the partitions "
rm -rf $PATH_TO_SDBOOT
mkdir $PATH_TO_SDBOOT
#mkdir $PATH_TO_SDROOTFS

sudo mount -t vfat ${DRIVE}1 boot/
#sudo mount -t ext3 ${DRIVE}2 rootfs/



echo " "
echo "Emptying partitions "
echo " "
sudo rm -rf  $PATH_TO_SDBOOT/*
#sudo rm -rf  $PATH_TO_SDROOTFS/*

cat << EOM
################################################################################

	Copying files now... will take minutes

################################################################################

Copying boot partition
EOM



echo "untar update.tar.bz2 to boot partition"
sudo tar xjvf update.tar.bz2 -C ${PATH_TO_SDBOOT} 
echo "Buring th u-boot.imx to sdcard"
dd if=/dev/zero of=${DRIVE} bs=1k seek=384 conv=fsync count=129
dd if=boot/bin/u-boot.imx of=${DRIVE} bs=1k seek=1 conv=fsync

echo ""
echo "Syncing...."
echo ""
sync
sync
sync

cp -f rootfs.tar.bz2 ${PATH_TO_SDBOOT}/system/
cp -f imx6ul-14x14-evk.dtb ${PATH_TO_SDBOOT}/system/
cp -f zImage ${PATH_TO_SDBOOT}/system/
sync
sync
sync
echo "Update done"


echo " "
echo "Un-mount the partitions "
sudo umount -f $PATH_TO_SDBOOT
#sudo umount -f $PATH_TO_SDROOTFS


echo " "
echo "Remove created temp directories "
#sudo rm -rf $PATH_TO_TMP_DIR
#sudo rm -rf $PATH_TO_SDROOTFS
sudo rm -rf $PATH_TO_SDBOOT


echo " "
echo "Operation Finished"
echo " "
