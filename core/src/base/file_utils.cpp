//
// Created by Windysha.
//

#include <dirent.h>
#include<cstdio>
#include<cerrno>
#include<unistd.h>
#include <cstring>
#include "file_utils.h"
#include"logging.h"
#include <string>
#include <fstream>
#include <asm-generic/fcntl.h>

namespace poros {

    static int mk_dir(const char *dir) {
        DIR *mydir = nullptr;
        if ((mydir = opendir(dir)) == nullptr)
        {
            int ret = mkdir(dir, 0777);
            if (ret != 0) {
                return -1;
            }
            LOGD("%s created sucess!/n", dir);
        }
        return 0;
    }

    int mk_all_dir(const char *dir) {
        // LOGD(" mk_all_dir for path: %s", dir);
        int i, len;
        char str[512];
        strncpy(str, dir, 512);
        len = strlen(str);
        for (i = 0; i < len; i++) {
            if (str[i] == '/') {
                str[i] = '\0';
                mk_dir(str);
                str[i] = '/';
            }
        }
        if (len > 0) {
            mk_dir(str);
        }
        return 0;
    }

    int file_xcopy_stod(const char *source_dir, const char *destination_dir) {
        FILE *infile = fopen(source_dir, "rb");
        if (infile == NULL) {
            return -1;
        }
        FILE *outfile = fopen(destination_dir, "wb");
        if (outfile == NULL) {
            fclose(infile);
            return -1;
        }
        //开始读写
        const int FLUSH_NUM = 1024 * 1024;
        char *ch = new char[FLUSH_NUM];
        if (ch == NULL) {
            fclose(infile);
            fclose(outfile);
            return -1;
        }
        int ret;
        while (!feof(infile)) {
            ret = fread(ch, 1, FLUSH_NUM, infile);
            fwrite(ch, 1, ret, outfile);
        }
        delete[]ch;
        fclose(infile);
        fclose(outfile);
        return 0;
    }

    int copy_file(const char*src_path, const char*dst_path) {
        // LOGD("copy_file from %s to dir: %s", src_path, dst_path);

        if (access(src_path, R_OK) != 0) {
            LOGE("file  %s do not exist, please check it! ", src_path);
        }

        std::ifstream src(src_path, std::ios::binary);
        std::ofstream dst(dst_path, std::ios::binary);
        dst << src.rdbuf();
        src.close();
        dst.close();
        return 0;
    }
}