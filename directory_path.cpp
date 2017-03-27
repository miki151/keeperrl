#include "stdafx.h"
#include "directory_path.h"
#include "file_path.h"

DirectoryPath::DirectoryPath(const std::string& p) : path(p) {}

FilePath DirectoryPath::file(const std::string& f) const {
  return FilePath(*this, f);
}

DirectoryPath DirectoryPath::subdirectory(const std::string& s) const {
  return DirectoryPath(path + "/" + s);
}

bool DirectoryPath::exists() const {
  return boost::filesystem::is_directory(path.c_str());
}

void DirectoryPath::createIfDoesntExist() const {
  if (!exists())
    boost::filesystem::create_directories(path.c_str());
}

vector<FilePath> DirectoryPath::getFiles() const {
  vector<FilePath> ret;
  if (DIR* dir = opendir(path.c_str())) {
    while (dirent* ent = readdir(dir))
      if (!boost::filesystem::is_directory(path + ent->d_name))
        ret.push_back(FilePath(*this, ent->d_name));
    closedir(dir);
  }
  return ret;
}

const char* DirectoryPath::get() const {
  return path.c_str();
}

std::ostream& operator <<(std::ostream& d, const DirectoryPath& path) {
  return d << path.get();
}
