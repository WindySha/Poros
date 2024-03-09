//
// Created by WindySha
//

#pragma once

#include<sys/types.h>
#include<sys/stat.h>

namespace poros {
    int mk_all_dir(const char *dir);

    int copy_file(const char* src_path, const char* dst_path);
}