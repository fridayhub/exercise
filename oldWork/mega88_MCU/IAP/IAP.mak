CC = iccavr
LIB = ilibw
CFLAGS =  -e -D__ICC_VERSION=722 -D_EE_EXTIO -DATMega1280  -l -g -MLongJump -MHasMul -MEnhanced -Wf-use_elpm 
ASFLAGS = $(CFLAGS) 
LFLAGS =  -g -e:0x20000 -ucrtatmega.o -bfunc_lit:0xe4.0x20000 -dram_end:0x21ff -bdata:0x200.0x21ff -dhwstk_size:30 -beeprom:0.4096 -fihx_coff -S2
FILES = iap.o 

IAP:	$(FILES)
	$(CC) -o IAP $(LFLAGS) @IAP.lk   -lcatm128
iap.o: C:\iccv7avr\include\iom88v.h C:\iccv7avr\include\_iom88to328v.h C:\iccv7avr\include\macros.h C:\iccv7avr\include\AVRdef.h
iap.o:	iap.c
	$(CC) -c $(CFLAGS) iap.c
