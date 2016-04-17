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
#include "led_beep_driver.h"

void* led_twinkle(void* data)
{
    int ret;
    while(1)
    {
        ret = Beep_alarm(1);
        ret = Red_led(1);
        usleep(80000);
        ret = Red_led(0);
        usleep(80000);
    }
}

void* uart_net_alarm(void* data)
{  
    int ret = 0;
    int de_fd;  //open file get delay time
    char delay_time[10];
    int del_time;
    char *buf = "power off\n";

    de_fd = open("/etc/delay_alarm", O_RDONLY);
    memset(delay_time, 0, sizeof(delay_time));
    ret = read(de_fd, delay_time, 10);
    printf("Read delay date is: %s\n", delay_time);
    del_time = atoi(delay_time);
    printf("trans data is:%d\n", del_time);

    while(1)
    {
        sleep(del_time);
        if(do_check() == 0x00)
        {   
            ret = do_tcp_alarm();		
            ret = do_uart_alarm(buf);
            if(ret == strlen(buf)) 
            printf("uart send data number is %d\n", ret);
            // pthread_exit(NULL);
        }

    }
}


void* fn_check(void* data)
{
	
	int ret = 0;
	unsigned char val;
    int flag = 1;  //check led pthread

   // val = do_check();
	//printf("<<<%s---%x\n", __func__, val);
    
    pthread_t led_id;
    pthread_attr_t led_attr;
    pthread_attr_init(&led_attr);
    pthread_attr_setdetachstate(&led_attr, PTHREAD_CREATE_DETACHED);

    pthread_t net_id;
    pthread_attr_t net_attr;
    pthread_attr_init(&net_attr);
    pthread_attr_setdetachstate(&net_attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&net_id, &net_attr, &uart_net_alarm, NULL);
    if(ret != 0){
        perror("uart_net_alarm pthread_create");
    }


    while(1)
    {
        usleep(5000);	
        //for power checker
        val = do_check();
        //printf("%s val_1 = %x\n", __func__, val);
        //printf("val = %x\n", val);
    
        if(val == 0x00)
        {
		    ret = do_self_alarm();
            usleep(50000);
	        val = do_check();
	        //printf("%s val_2 = %x\n", __func__, val);

	        if(0x00 == val)
			{   
                if(flag == 1)
                {
                    ret = pthread_create(&led_id, &led_attr, &led_twinkle, NULL);
                    if(ret != 0){
                        perror("led pthread_create");
                    }
                                       flag = 0;
                }
	        }
        }
		else
		{
		    ret = Beep_alarm(0);
			ret = Red_led(1);
            flag = 1;
            usleep(5000); 
            ret = pthread_cancel(led_id);
            if(ret == 0)
            {
                printf("led pthread canceled\n");
            }
         /*   usleep(5000);
            ret = pthread_cancel(net_id);
            if(ret == 0)
            {
                printf("led pthread canceled\n");
            } */

		}
    }
}

int run_check_thread (void)
{

    //for power checker
	int ret = 0;
	printf("<<<%s\n", __func__);


    pthread_t id;
    pthread_attr_t id_attr;
    pthread_attr_init(&id_attr);
    pthread_attr_setdetachstate(&id_attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&id, &id_attr, &fn_check, NULL);
	printf(">>>%s ret:%d\n", __func__, ret);

    return ret;

}

/* thirddrvtest 
  */
int CheckPower_test()
{
    int ret = 0;

	while (1)
	{

		ret = do_check();

	}
	
	return 0;
}

