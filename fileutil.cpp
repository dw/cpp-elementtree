
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
        out.emplace_back();
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


bool decompress_str(const std::string &s, std::string &out)
{
    unsigned have;
    std::string buf;
    buf.resize(65536);

    z_stream strm;
    strm.zalloc = NULL;
    strm.zfree = NULL;
    strm.opaque = NULL;
    strm.avail_in = s.size();
    char *ss = const_cast<char *>(s.data());
    strm.next_in = reinterpret_cast<Bytef *>(ss);

    if(inflateInit(&strm) != Z_OK) {
        return false;
    }

    int ret;
    do {
        do {
            strm.avail_out = buf.size();
            strm.next_out = reinterpret_cast<Bytef *>(&buf[0]);
            ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret) {
            case Z_STREAM_ERROR:
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&strm);
                return false;
            }
            out.append(buf.data(), buf.size() - strm.avail_out);
        } while(strm.avail_out == 0);
    } while(strm.avail_in && ret != Z_STREAM_END);

    inflateEnd(&strm);
    assert(Z_STREAM_END == ret);
    return true;
}
