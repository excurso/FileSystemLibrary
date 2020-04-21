/******************************************************************************
FileSystemLibrary - A C++11 file system library mainly based on C functions
                    to increase portability

Copyright (C) 2019-2020 Waldemar Zimpel <hspp@utilizer.de>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*******************************************************************************/


#include "FileSystem.h"

/*static*/ string
FileSystem::
getCleanPath(string path)
{
    if (!isAbsolutePath(path)) return path;

    const bool target_is_directory = path.back() == DIR_SEP[0];

#ifdef WIN
    const string PARTITION = path.substr(0, 2);
    path = path.substr(2);
#endif

    DataContainer<string> dirs = String::split(path, DIR_SEP);
    path.clear();

    for (auto itr = dirs.begin(); itr != dirs.end(); ++itr)
        if (itr->empty() || *itr == ".")
            dirs.erase(itr--);
        else if (*itr == "..")
            dirs.erase(itr--, itr--+1);

#ifdef WIN
    path = PARTITION;
#endif

    if (dirs.empty())
        path += DIR_SEP;
    else
        for (const auto &dir : dirs)
            path.append(DIR_SEP).append(dir);

    if (target_is_directory)
        path += DIR_SEP;

    return path;
}

/*static*/ string
FileSystem::
getRelativePath(string path1, string path2)
{
    if (!isAbsolutePath(path1) || !isAbsolutePath(path2)) return string();

#ifdef WIN
    const string PARTITION = path1.substr(0, 2);

    // Check, if both paths use the same partition and remove partition
    // from the path temporarily
    if (path1.front() == path2.front()) {
        path1 = path1.substr(2, path1.length()-2);
        path2 = path2.substr(2, path2.length()-2);
    } else return string();
#endif

    const bool path2_is_file = path2.back() != '/';

    DataContainer<string> dirs_path1(String::split(path1, DIR_SEP));
    DataContainer<string> dirs_path2(String::split(path2, DIR_SEP));

    for (auto itr = dirs_path1.rbegin()+1; itr != dirs_path1.rend()+1; ++itr)
        if (itr.base()->empty()) dirs_path1.erase(itr.base());

    for (auto itr = dirs_path2.rbegin()+1; itr != dirs_path2.rend()+1; ++itr)
        if (itr.base()->empty()) dirs_path2.erase(itr.base());

    string file_name;

    if (isFile(path1)) dirs_path1.pop_back();

    if (path2_is_file) {
        file_name = dirs_path2.back();
        dirs_path2.pop_back();
    }

    path1.clear();

#ifdef WIN
    path1 = PARTITION;
#endif

    if (dirs_path1.size() >= dirs_path2.size()) {
        for (size_t i = 0; i != dirs_path1.size(); ++i) {
            if (dirs_path2.size() > i && dirs_path1[i] == dirs_path2[i]) continue;
            path1 += "..";
            path1 += DIR_SEP;

            for (; i != dirs_path1.size(); ++i) {
                path1 += dirs_path2[i];
                path1 += DIR_SEP;
            }

            break;
        }
    } else {
        for (size_t i = 0; i != dirs_path2.size(); ++i) {
            if (dirs_path1.size() > i && dirs_path1[i] == dirs_path2[i]) continue;
            path1 += dirs_path2[i] + DIR_SEP;
        }
    }

    if (path2_is_file) path1 += file_name;

    return path1;
}

/*static*/ DataContainer<string>
FileSystem::
getDirectoryContents(const string &path)
{
    DataContainer<string> entries;
    dirent *entry;

    const auto dir = opendir(path.data());

    while (bool(entry = readdir(dir))) {
        if (strcmp(static_cast<char*>(entry->d_name), ".") != 0 &&
            strcmp(static_cast<char*>(entry->d_name), "..") != 0) {

            entries.emplace_back(path + DIR_SEP + static_cast<char*>(entry->d_name));
        }
    }

    closedir(dir);
    delete entry;

    return entries;
}

/*static*/ bool
FileSystem::
copyFile(const string &input_path, const string &output_path)
{
    FILE * const from_fp = fopen(input_path.data(), "re");

    if (from_fp != nullptr) {
        constexpr size_t BUFFER_LENGTH = 4096;
        char buf[BUFFER_LENGTH];
        size_t cnt = 0;

        FILE * const to_fp = fopen(output_path.data(), "we");

        while (bool(cnt = fread(static_cast<char*>(buf), 1, BUFFER_LENGTH, from_fp))) {
            fwrite(static_cast<char*>(buf), 1, cnt, to_fp);
        }

        fclose(to_fp);
        fclose(from_fp);

        return true;
    }

    return false;
}

/*static*/ bool
FileSystem::
createPath(string path, string &fail_path) {
    if (!isAbsolutePath(path)) return false;

#ifdef WIN
#define PARTITION partition +
    const string partition = path.substr(0, 2);
    path = path.substr(2);

    if (path.empty()) path = DIR_SEP;
#else
#define PARTITION
#endif

    DataContainer<string> directories = String::split(path, DIR_SEP);

    path.clear();

    if (directories.empty())
        path.append(DIR_SEP);
    else {
        for (auto itr = directories.begin(); itr != directories.end(); ++itr) {
            if (itr->empty()) {
                directories.erase(itr--);
                continue;
            }

            path.append(DIR_SEP + *itr);

            if (exists(PARTITION path)) {
                if (!isDir(PARTITION path)) {
                    fail_path = PARTITION path;
                    return false;
                }

                continue;
            }

            if (!createDirectory(PARTITION path)) {
                fail_path = PARTITION path;
                return false;
            }
        }
    }

    return true;

#undef PARTITION
}

/*static*/ bool
FileSystem::
readFile(const string &path, string &content)
{
    content.clear();

    const int64_t f_size = getFileSize(path);

    if (f_size != -1) {
        content.reserve(static_cast<uint32_t>(f_size));
    }

    ifstream file(path);
    string line;

    if (file.good()) {
        while (getline(file, line)) {
            content += line;
            content += '\n';
        }

        file.close();
        return true;
    }

    return false;
}

/*static*/ bool
FileSystem::
writeFile(const string &path, const string &content, ofstream::openmode mode, time_t last_modified_time)
{
    ofstream file;
    file.open(path, mode);

    if (file.good()) {
        file << content;
        file.close();

        return true;
    }

    return false;
}

/*static*/ bool
FileSystem::
isRemoteAddress(const string &addr)
{
    if (string(addr.begin(), addr.begin()+2) == "//") return true;

    string protocol;

    for (auto itr = addr.begin(); itr != addr.end(); ++itr) {
        if (bool(isalnum(*itr))) {
            protocol += *itr;
            continue;
        }

        if (protocol != "file" && string(itr, itr+3) == "://") {
            return true;
        }

        break;
    }

    return false;
}

/*static*/ string
FileSystem::
getFileExtension(string file_path)
{
    file_path = getBaseName(file_path);

    auto itr = file_path.end();

    while (itr != file_path.begin() && *itr != DIR_SEP[0])
        if (*--itr == '.')
            return string(itr, file_path.end());

    return string();
}

/*static*/ string
FileSystem::
getParentPath(const string &path)
{
    string parent_path = path;

    if (parent_path.back() == DIR_SEP[0])
        parent_path.pop_back();

    parent_path = string(path.begin(), find(path.rbegin(), path.rend(), DIR_SEP[0]).base());

    return parent_path;
}
