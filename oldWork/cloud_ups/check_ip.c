#include<stdio.h>
#include <arpa/inet.h>

int if_a_string_is_a_valid_ipv4_address(const char *str)
{
    struct in_addr addr;
    int ret;
    volatile int local_errno;

    int errno = 0;
    ret = inet_pton(AF_INET, str, &addr);
    local_errno = errno;
    if (ret > 0)
        printf("\"%s\" is a IPv4 address\n", str);
    else if (ret < 0)
        printf("EAFNOSUPPORT: %d\n", strerror(local_errno));
    else
        printf("\"%s\" is not a valid IPv4 address\n", str);
    return ret;
}


