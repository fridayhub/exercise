
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "Power_Checker.h"

int init()
{
	int fd;
	unsigned char key_val;
	
	fd = open("/dev/buttons", O_RDONLY);
	return fd;
}

int do_check(int fd)
{
	int key_val;
	if (fd < 0)
	{
		printf("can't open!\n");
		return -1;
	}

	read(fd, &key_val, 1);
	//printf("key_val = 0x%x\n", key_val);
	
	return key_val;
}
/* thirddrvtest 
  */
int CheckPower_test()
{
        int ret = 0;
	ret = init();
	while (1)
	{
		ret = do_check(ret);
	}
	
	return 0;
}

