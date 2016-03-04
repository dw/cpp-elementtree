
#include <cassert>
#include <dirent.h> // opendir()
#include <fstream>

#include <zlib.h>

#include "fileutil.hpp"


std::string get_file_contents(const std::string &filename)
{
    std::ifstream fp(filename.c_str(), std::ios::in | std::ios::binary);
    if(! fp) {
        return std::string();
    }

    std::string out;
    fp.seekg(0, std::ios::end);
    out.resize(fp.tellg());
    fp.seekg(0, std::ios::beg);
    fp.read(&out[0], out.size());
    return out;
}


void get_path_list(std::vector<std::string> &out, const char *dirname)
{
    DIR *dir = opendir(dirname);
    assert(dir);

    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {
        if(ent->d_name[0] == '.') {
            continue;
        }
        out.push_back(std::string());
        std::string &s = out.back();
        s.append(dirname);
        s.append("/");
        s.append(ent->d_name);
    }

    closedir(dir);
}


void decompress(const std::string &path, std::string &out)
{
    gzFile gfp = gzopen(path.c_str(), "r");
    assert(gfp);

    std::string buf(1048576, '\0');

    for(;;) {
        int ret = gzread(gfp, reinterpret_cast<void *>(&buf[0]), buf.size());
        if(ret == -1 || ret == 0) {
            break;
        }
        out.append(buf, 0, ret);
    }
    gzclose(gfp);
}
