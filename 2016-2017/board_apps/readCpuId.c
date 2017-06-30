#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


int main()
{
    char buf1[10], buf2[10];
    char bufUid[20];
    int fd = 0, i = 0, j = 0, fdr;

    memset(bufUid, 0, sizeof(bufUid));

    fd = open("/sys/fsl_otp/HW_OCOTP_CFG0", O_RDONLY);
    if(fd < 0){
        perror("Open /sys/fsl_otp/HW_OCOTP_CFG0");
        return 0;
    }

    read(fd, buf1, 10);
    for(i = 0, j = 2; j < 10; i++, j++)
        bufUid[i] = buf1[j];
    //printf("read id  1 is :%s\n", buf1);
    close(fd);

    
    fd = open("/sys/fsl_otp/HW_OCOTP_CFG1", O_RDONLY);
    if(fd < 0){
        perror("Open /sys/fsl_otp/HW_OCOTP_CFG2");
        return 0;
    }

    read(fd, buf2, 10);
    for(j = 2; j < 10; i++, j++)
        bufUid[i] = buf2[j];
    //printf("read id  2 is :%s\n", buf2);
    printf("read cpuID   is :%s\n", bufUid);
    close(fd);

    fdr = open("/etc/cpuid", O_RDWR|O_CREAT);
    if (fdr < 0)
    {
        printf("Can't open config file to read mac\n");
        return 0;
    }
    write(fdr, bufUid, strlen(bufUid));
    close(fdr);

}
