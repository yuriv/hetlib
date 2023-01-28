//
// Created by yuri on 10/14/21.
//

#ifndef HETLIB_UNCOMMON_DEFINITIONS_H
#define HETLIB_UNCOMMON_DEFINITIONS_H

#include "version_config.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__) ": "

#include <iostream>

#define HET_UNUSED(x) ((void)x)

#define LOGD std::cout

#endif // HETLIB_UNCOMMON_DEFINITIONS_H
