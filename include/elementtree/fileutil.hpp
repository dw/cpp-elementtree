
#ifndef FILEUTIL_HPP
#define FILEUTIL_HPP

#include <string>
#include <vector>


std::string get_file_contents(const std::string &filename);
void get_path_list(std::vector<std::string> &out, const char *dirname);
void decompress(const std::string &path, std::string &out);


#endif
