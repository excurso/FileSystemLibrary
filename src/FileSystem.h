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


#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define DECL_EXPORT __declspec(dllexport)
#  define DECL_IMPORT __declspec(dllimport)
#ifndef WIN
#define WIN
#endif
#else
#  define DECL_EXPORT __attribute__((visibility("default")))
#  define DECL_IMPORT __attribute__((visibility("default")))
#endif

#ifdef FILESYSTEM_LIBRARY
#  define FILESYSTEM_EXPORT DECL_EXPORT
#else
#  define FILESYSTEM_EXPORT DECL_IMPORT
#endif

#include "../../StringLibrary/src/String.h"
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

#ifdef WIN
#include <windows.h>
#define DIR_SEP "\\"
#else
#define DIR_SEP "/"
#endif

class FILESYSTEM_EXPORT FileSystem
{
public:
    static DataContainer<string>
    getDirectoryContents(const string &path);

    static inline bool
    exists              (const string &path),
    isReadable          (const string &path),
    isWritable          (const string &path),
    isFile              (const string &path),
    isDir               (const string &path),
    rename              (const string &current_path, const string &new_path),
#ifdef WIN
    createDirectory     (const string &path),
#else
    createDirectory     (const string &path, mode_t mode = 0775),
    createPipe			(const string &path),
#endif
	isEmptyDir          (const string &path),
    deleteFile			(const string &path),
    isAbsolutePath      (const string &path);

    static bool
    createPath          (string path, string &fail_path),
    copyFile            (const string &source_path, const string &target_path),
    readFile            (const string &path, string &content),
    writeFile           (const string &path, const string &content,
                         ofstream::openmode mode = ios::out | ios::trunc,
                         time_t last_modified_time = 0),
    isRemoteAddress(const string &addr);

    static inline string
    getBaseName         (string path);

    static string
    getFileExtension    (string file_path),
    getParentPath       (const string &path),
    getCleanPath        (string path),
    getRelativePath     (string path1, string path2);

    static inline int64_t
    getFileSize         (const string &file_path);
};

/*static*/ inline bool
FileSystem::
isFile(const string &path)
{
    struct stat path_stat {};
    stat(path.data(), &path_stat);

    return S_ISREG(path_stat.st_mode) != 0;
}

/*static*/ inline bool
FileSystem::
isDir(const string &path)
{
    struct stat path_stat {};
    stat(path.data(), &path_stat);

    const int res = S_ISDIR(path_stat.st_mode);
    return res != 0;
}

/*static*/ inline bool
FileSystem::
exists(const string &path)
{
    return access(path.data(), F_OK) == 0;
}

/*static*/ inline bool
FileSystem::
isReadable(const string &path)
{
    return access(path.data(), R_OK) == 0;
}

/*static*/ inline bool
FileSystem::
isWritable(const string &path)
{
    const auto &result = access(path.data(), W_OK);
    return result == 0;
}

/*static*/ inline string
FileSystem::
getBaseName(string path)
{
    if (path.back() == DIR_SEP[0]) path.erase(path.end()-1);
    return string(find(path.rbegin(), path.rend(), DIR_SEP[0]).base(), path.end());
}

#ifdef WIN
/*static*/ inline bool
FileSystem::
createDirectory(const string &path)
{
    return mkdir(path.data()) == 0;
}
#else
/*static*/ inline bool
FileSystem::
createDirectory(const string &path, mode_t mode)
{
    return mkdir(path.data(), mode) == 0;
}
#endif

/*static*/ inline bool
FileSystem::
isEmptyDir(const string &path)
{
    char cnt = 0;
    DIR *dir = opendir(path.data());

    while (bool(readdir(dir))) {
        if (++cnt > 2) {
            closedir(dir);
            return false;
        }
    }

    return true;
}

/*static*/ inline bool
FileSystem::
deleteFile(const string &path)
{
    return remove(path.data()) == 0;
}

/*static*/ inline bool
FileSystem::
isAbsolutePath(const string &path)
{
#ifdef WIN
    return !path.empty() && isalpha(path.front()) && path.at(1) == ':';
#else
    return !path.empty() && path.front() == DIR_SEP[0];
#endif
}

/*static*/ inline int64_t
FileSystem::
getFileSize(const string &file_path)
{
    struct stat st {};

    if (stat(file_path.data(), &st) == 0) {
        return st.st_size;
    }

    return -1;
}

/*static*/ inline bool
FileSystem::
rename(const string &current_path, const string &new_path)
{
    return std::rename(current_path.data(), new_path.data()) == 0;
}

#ifndef WIN
/*static*/ inline bool
FileSystem::
createPipe(const string &path)
{
	return mkfifo(path.data(), 0666) == 0;
}
#endif

#endif // FILESYSTEM_H
