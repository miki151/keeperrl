#include "stdafx.h"
#include "file_path.h"
#include "debug.h"
#include "util.h"

#ifndef _MSC_VER
# include <sys/types.h>
# include <sys/stat.h>
#else
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
#endif

FilePath FilePath::fromFullPath(const std::string& path) {
  return FilePath(split(path, {'/'}).back(), path);
}

const char* FilePath::getPath() const {
  return fullPath.c_str();
}

const char* FilePath::getFileName() const {
  return filename.c_str();
}

void FilePath::erase() const {
  remove(getPath());
}

time_t FilePath::getModificationTime() const {
  struct stat buf;
  stat(getPath(), &buf);
  return buf.st_mtime;
}

bool FilePath::exists() const {
#ifdef _MSC_VER
  auto attribs = GetFileAttributesA(fullPath.c_str());
  if(attribs == INVALID_FILE_ATTRIBUTES)
    return false;
  else
    return (attribs & FILE_ATTRIBUTE_DIRECTORY) == 0;
#elif defined(WINDOWS)
  struct _stat buf;
  _stat(fullPath.c_str(), &buf);
  return S_ISREG(buf.st_mode);
#else
  struct stat buf;
  if (lstat(fullPath.c_str(), &buf) != 0)
    return false;
  return S_ISREG(buf.st_mode);
#endif
}

bool FilePath::hasSuffix(const string& suf) const {
  return filename.size() >= suf.size() && filename.substr(filename.size() - suf.size()) == suf;
}

FilePath FilePath::withSuffix(const string& suf) const {
  return FilePath(filename + suf, fullPath + suf);
}

FilePath FilePath::changeSuffix(const string& current, const string& newSuf) const {
  CHECK(hasSuffix(current));
  return FilePath(
      filename.substr(0, filename.size() - current.size()) + newSuf,
        fullPath.substr(0, fullPath.size() - current.size()) + newSuf);
}

optional<string> FilePath::readContents() const {
  ifstream in;
  in.open(fullPath);
  if (!in.good())
    return none;
  stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

FilePath::FilePath(const DirectoryPath& dir, const string& f) : filename(f), fullPath(dir.getPath() + "/"_s + f) {
}

FilePath::FilePath(string name, string path) : filename(std::move(name)), fullPath(std::move(path)) {
}

bool FilePath::operator==(const FilePath& rhs) const {
  return fullPath == rhs.fullPath;
}

std::ostream& operator <<(std::ostream& d, const FilePath& path) {
  return d << path.getPath();
}

bool FilePath::isAbsolute() const {
  return isAbsolutePath(fullPath.c_str());
}

FilePath FilePath::absolute() const {
  return FilePath::fromFullPath(getAbsolute(fullPath.data()));
}

void FilePath::copyTo(FilePath to) const {
  CHECK(exists());
  ifstream in(fullPath, std::ios::binary);
  ofstream out(to.fullPath, std::ios::binary);
  out << in.rdbuf();
}
