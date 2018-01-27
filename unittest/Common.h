/*************************************************************************
  Copyright (C) 2018 Tencent Inc.
  All rights reserved.

  > File Name: Common.h
  > Author: sevenyou
  > Mail: sevenyou@tencent.com
  > Created Time: Sat 27 Jan 2018 03:14:09 PM CST
  > Description:
 ************************************************************************/

#ifndef COMMON_H
#define COMMON_H
#pragma once

#include <stdlib.h>

#include <string>

namespace qcloud_cos {

std::string GetEnv(const std::string& env_var_name) {
    char const* tmp = getenv(env_var_name.c_str());
    if (tmp == NULL) {
            return "NOT_EXIST_ENV_" + env_var_name;
        }

    return std::string(tmp);
}

}
#endif // COMMON_H
