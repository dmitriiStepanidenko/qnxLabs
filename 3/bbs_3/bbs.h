/*
 * bbs.h
 *
 *  Created on: 23.02.2023
 *      Author: user
 */

#ifndef BBS_H_
#define BBS_H_

#include <cstdint>
#include <devctl.h>
#include <sys/types.h>
#include <unistd.h>

namespace bbs {

const int SET =1;
const int GET = 2;

struct BBSParams
{
    std::uint32_t seed;
    std::uint32_t p;
    std::uint32_t q;
};


#define MY_DEVCTL_SET_PARAM __DIOT(_DCMD_MISC, 1, bbs::BBSParams)
#define MY_DEVCTL_START __DIOF(_DCMD_MISC, 2, int)

}




#endif /* BBS_H_ */
