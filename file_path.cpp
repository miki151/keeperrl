#include "stdafx.h"
#include "file_path.h"
#include "debug.h"
#include "util.h"
#include <sys/types.h>
#include <sys/stat.h>

FilePath FilePath::fromFullPath(const std::string& path) {
  return FilePath(split(path, {'/'}).back(), path);
}

const char* FilePath::getPath() const {
  return fullPath.c_str();
}

const char* FilePath::getFileName() const {
  return filename.c_str();
}

time_t FilePath::getModificationTime() const {
  struct stat buf;
  stat(getPath(), &buf);
  return buf.st_mtime;
}

bool FilePath::hasSuffix(const string& suf) const {
  return filename.size() >= suf.size() && filename.substr(filename.size() - suf.size()) == suf;
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

FilePath::FilePath(const DirectoryPath& dir, const string& f) : filename(f), fullPath(dir.get() + "/"_s + f) {
}

FilePath::FilePath(const std::string& name, const std::string& path) : filename(name), fullPath(path) {

}

bool FilePath::operator==(const FilePath &rhs) const { return fullPath == rhs.fullPath; }

std::ostream&operator <<(std::ostream& d, const FilePath& path) {
  return d << path.getPath();
}
