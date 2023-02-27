#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <devctl.h>
#include <vector>
#include "bbs.h"
#include <signal.h>

#define VEC_SIZE 1024
std::vector<std::uint32_t> vec(VEC_SIZE);
size_t current=-1;
bool circle = false;

void push(std::vector<std::uint32_t> & vec, size_t& current, std::uint32_t number){
	current++;
	if( current > vec.size() ){
		circle = true;
		current =0;
	}
	vec[current]=number;
}


void sigint_handler(int sig) {
	size_t end_index = current;
	if (circle == true)
		end_index = vec.size() - 1;
    for(size_t i = 0; i< end_index; i++)
    	std::cout << vec[i] << "; ";
    std::cout << std::endl;

    exit(0);
}

int main( int argc, char **argv )
{
    // open a connection to the server (fd == coid)
    int fd = open("/dev/cryptobbs", O_RDWR);
    if(fd < 0)
    {
        std::cerr << "E: unable to open server connection: " << strerror(errno ) << std::endl;
        return EXIT_FAILURE;
    }

    bbs::BBSParams params;
    params.p = 3;
    params.q = 263;
    params.seed = 866;

    int commandSetResult = devctl(fd, MY_DEVCTL_SET_PARAM, &params, sizeof(bbs::BBSParams), NULL);
    if(commandSetResult != EOK)
    {
        std::cerr << "E: unable to set generator parameters: " << strerror(errno ) << std::endl;
        return EXIT_FAILURE;
    }

    std::uint32_t number;

    signal(SIGINT, sigint_handler);

    commandSetResult = devctl(fd, MY_DEVCTL_START, &number, sizeof(number), NULL);
    while (commandSetResult == EOK) {
    	push(vec, current, number);

    	commandSetResult = devctl(fd, MY_DEVCTL_START, &number, sizeof(number), NULL);
    };

    close(fd);

    return EXIT_SUCCESS;
}
