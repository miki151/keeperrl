#include "stdafx.h"
#include "directory_path.h"
#include "file_path.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "dirent.h"

DirectoryPath::DirectoryPath(const std::string& p) : path(p) {}

FilePath DirectoryPath::file(const std::string& f) const {
  return FilePath(*this, f);
}

DirectoryPath DirectoryPath::subdirectory(const std::string& s) const {
  return DirectoryPath(path + "/" + s);
}

static bool isDirectory(const string& path) {
  struct stat path_stat;
  if (stat(path.c_str(), &path_stat))
    return false;
  else
    return S_ISDIR(path_stat.st_mode);
}

bool DirectoryPath::exists() const {
  return isDirectory(get());
}

#ifndef WINDOWS
#include <sys/stat.h>
#include <sys/types.h>
void DirectoryPath::createIfDoesntExist() const {
  if (!exists()) {
    USER_CHECK(!mkdir(path.data(), 0750)) << "Unable to create directory \"" + path + "\": " + strerror(errno);
  }
}
#else

implement

#endif


static bool isRegularFile(const string& path) {
  struct stat path_stat;
  if (stat(path.c_str(), &path_stat))
    return false;
  else
    return S_ISREG(path_stat.st_mode);
}

vector<FilePath> DirectoryPath::getFiles() const {
  vector<FilePath> ret;
  if (DIR* dir = opendir(path.data())) {
    while (dirent* ent = readdir(dir))
      if (isRegularFile(path + "/" + ent->d_name))
        ret.push_back(FilePath(*this, ent->d_name));
    closedir(dir);
  }
  return ret;
}

const char* DirectoryPath::get() const {
  return path.data();
}

std::ostream& operator <<(std::ostream& d, const DirectoryPath& path) {
  return d << path.get();
}
