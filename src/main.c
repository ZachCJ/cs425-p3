#include "lab.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef TEST
#define main main_exclude
#endif

bool parseCommandlineArguments(int argc,const char *argv[]) {
    for(int i = 0; i < argc; i++) {

    }
}

/**
 * 
 * 
 * Command Line Usage:  
 *      myapp info <address>/<prefix>
 *      myapp split <network>/<prefix> <count>
 *      myapp lookup <table-file> <address>...
 *      
 */
int main(int argc,const char *argv[])
{
    return 0;
}