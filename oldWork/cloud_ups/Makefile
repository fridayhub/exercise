server:main.o power_checker.o
server:power_checker.c main.c
	arm-linux-gcc main.c tcp_server.c uart_controller.c self_controller.c power_checker.c led_beep_driver.c -o server -lpthread
uart_app:uart_app.o check_ip.o
	arm-linux-gcc uart_app.c check_ip.c -o uart_app
clean:
	rm -rf *.o server uart_app
