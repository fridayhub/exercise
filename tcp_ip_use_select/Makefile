server:server.o Power_Checker.o Driver.o
server:Power_Checker.c server.c
	arm-linux-gcc server.c Power_Checker.c Driver.c -o server -lpthread
.PHONY:clean
clean:
	rm -rf *.o server
