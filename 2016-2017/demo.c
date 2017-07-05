#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>


void daemonize(const char *cmd){
    int i,fd0,fd1,fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    umask(0); //1.调用umask()设置文件创建时的权限规则

    if(getrlimit(RLIMIT_NOFILE, &rl) < 0){
        perror("getrlimit failed");
        exit(1);
    }

    if((pid = fork()) <0 ){
        perror("fork failed");
        exit(1);
    }else if(pid != 0) //2.调用fork, 然后使父进程exit
        exit(0);

    setsid(); //3.调用setsid创建一个新会话

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGHUP, &sa, NULL) <0){
        perror("sigaction failed");
        exit(1);
    }
    if((pid = fork())<0){
        perror("fork failed");
        exit(1);
    }else if(pid != 0)
        exit(0);

    if(chdir("/") < 0){  //4.将系统根目录设置为当前工作目录
        perror("chdir error");
        exit(1);
    }

    if(rl.rlim_max == RLIM_INFINITY) //5.关闭所有打开的文件描述符, 包括stdin/stdout/stderr
        rl.rlim_max = 1024;
    for(i = 0; i<rl.rlim_max; i++)
        close(i);

    fd0=open("/dev/null", O_RDWR); //6.由于上面已经关闭所有文件描述符, 所以新建文件描述符时会从0开始计数, 于是fd0=0,依次fd1=1,fd2=2
    fd1=dup(0);
    fd2=dup(0);

    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if(fd0 != 0 || fd1 != 1 || fd2 != 2){
        syslog(LOG_ERR, "unexpected file description %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
}

int main(){
   daemonize("demo.log");
   while(1){
       syslog(LOG_INFO, "demo log info");
       sleep(1);
   }
   return 0;
}
