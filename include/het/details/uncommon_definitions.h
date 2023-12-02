//
// Created by yuri on 10/14/21.
//

#ifndef HETLIB_UNCOMMON_DEFINITIONS_H
#define HETLIB_UNCOMMON_DEFINITIONS_H

#include "generated/version_config.h"

#ifndef AT
  #define STRINGIFY(x) #x
  #define TOSTRING(x) STRINGIFY(x)
  #define AT __FILE__ ":" TOSTRING(__LINE__) ": "
#endif

#include <iostream>

#endif // HETLIB_UNCOMMON_DEFINITIONS_H
