KERN_DIR = /home/hakits/shen_cloud/linux-2.6.39.3


all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order

obj-m	+= gpio.o
