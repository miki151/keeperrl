#include "stdafx.h"
#include "directory_path.h"
#include "file_path.h"
#ifndef _MSC_VER
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include "dirent.h"
#else
#include <filesystem>
#endif

DirectoryPath::DirectoryPath(string p) : path(std::move(p)) {
}

FilePath DirectoryPath::file(const std::string& f) const {
  return FilePath(*this, f);
}

DirectoryPath DirectoryPath::subdirectory(const std::string& s) const {
  return DirectoryPath(path + "/" + s);
}

#ifndef _MSC_VER
static bool isDirectory(const string& path) {
  struct stat path_stat;
  if (stat(path.c_str(), &path_stat))
    return false;
  else
    return S_ISDIR(path_stat.st_mode);
}
#else
static bool isDirectory(const string& path) {
  std::error_code err;
  return std::filesystem::is_directory(path, err);
}
#endif

bool DirectoryPath::exists() const {
  return isDirectory(getPath());
}

void DirectoryPath::createIfDoesntExist() const {
  if (!exists()) {
#ifdef _MSC_VER
    std::error_code err;
    USER_CHECK(!std::filesystem::create_directory(path.data(), err));
#elif !defined(WINDOWS)
    USER_CHECK(!mkdir(path.data(), 0750)) << "Unable to create directory \"" + path + "\": " + strerror(errno);
#else
    USER_CHECK(!mkdir(path.data())) << "Unable to create directory \"" + path + "\": " + strerror(errno);
#endif
  }
}

void DirectoryPath::removeRecursively() const {
  if (exists()) {
    for (auto file : getFiles())
      remove(file.getPath());
    for (auto subdir : getSubDirs())
      subdirectory(subdir).removeRecursively();
    #ifdef _MSC_VER
      std::error_code err;
      std::filesystem::remove(getPath(), err);
    #else
      rmdir(getPath());
    #endif
  }
}

static bool isRegularFile(const string& path) {
#ifndef _MSC_VER
  struct stat path_stat;
  if (stat(path.c_str(), &path_stat))
    return false;
  else
    return S_ISREG(path_stat.st_mode);
#else
  std::error_code err;
  return std::filesystem::is_regular_file(path, err);
#endif
}

vector<FilePath> DirectoryPath::getFiles() const {
#ifndef _MSC_VER
  vector<FilePath> ret;
  if (DIR* dir = opendir(path.data())) {
    while (dirent* ent = readdir(dir))
      if (isRegularFile(path + "/" + ent->d_name))
        ret.push_back(FilePath(*this, ent->d_name));
    closedir(dir);
  }
  return ret;
#else
  vector<FilePath> res;
  for(const auto& entry : std::filesystem::directory_iterator{path.data()}) {
    if(entry.is_regular_file())
      res.push_back(FilePath(entry.path().filename().string(), entry.path().string()));
  }
  return res;
#endif
}

vector<string> DirectoryPath::getSubDirs() const {
#ifndef _MSC_VER
  vector<string> ret;
  if (DIR* dir = opendir(path.data())) {
    while (dirent* ent = readdir(dir))
      if (isDirectory(path + "/" + ent->d_name) && strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
        ret.push_back(ent->d_name);
    closedir(dir);
  }
  return ret;
#else
  vector<string> res;
  std::error_code err;
  for(const auto& entry : std::filesystem::directory_iterator{path.data(), err}) {
    if(entry.is_directory())
      res.push_back(entry.path().filename().string());
  }
  return res;
#endif
}

const char* DirectoryPath::getPath() const {
  return path.data();
}

std::ostream& operator <<(std::ostream& d, const DirectoryPath& path) {
  return d << path.getPath();
}

bool isAbsolutePath(const char* str) {
#ifdef WINDOWS
  if ((str[0] >= 'a' && str[0] <= 'z') || (str[0] >= 'A' && str[0] <= 'Z'))
    if (str[1] == ':' && (str[2] == '/' || str[2] == '\\'))
      return true;
#else
  if (str[0] == '/')
    return true;
#endif
  return false;
}

string getAbsolute(const char* path) {
  if (isAbsolutePath(path))
    return path;
  // TODO: this is not exactly right if paths contain dots (../../)
  return DirectoryPath::current().getPath() + string("/") + path;
}

DirectoryPath DirectoryPath::current() {
#ifndef _MSC_VER
  char buffer[2048];
  char* name = getcwd(buffer, sizeof(buffer) - 1);
  CHECK(name && "getcwd error");
  return DirectoryPath(name);
#else
  return DirectoryPath(std::filesystem::current_path().string());
#endif
}

optional<string> DirectoryPath::copyRecursively(DirectoryPath to) {
  to.createIfDoesntExist();
  for (auto file : getFiles())
    file.copyTo(to.file(file.getFileName()));
  for (auto dir : getSubDirs()) {
    DirectoryPath subTo = to.subdirectory(dir);
    DirectoryPath subFrom = subdirectory(dir);
    subFrom.copyRecursively(subTo);
  }
  return none;
}

bool DirectoryPath::isAbsolute() const {
  return isAbsolutePath(path.c_str());
}

DirectoryPath DirectoryPath::absolute() const {
  return DirectoryPath(getAbsolute(path.data()));
}
