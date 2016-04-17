/*

FileName:

Author:

Description:

Version:

FunctionList:

History:
    <author>  <time>  <version>  <desc>
 
*/

#include "power_checker.h"
#include "tcp_server.h"
#include "self_controller.h"
#include "uart_controller.h"
#include "led_beep_driver.h"

int main()
{
    //do some thing
	printf("<<<%s\n", __func__);
	int ret = 0;
    
    //initial alarm uart
    uart_init();

    //led and beep init
    driver_initial();
    
    //green led always on
    ret = Green_led(1);
    if( ret != 0 )
    {
        perror("led_beep fd");
    }

    //start power check thread
	ret = run_check_thread();

    //start tcp server
	ret = run_tcp_server();

}
