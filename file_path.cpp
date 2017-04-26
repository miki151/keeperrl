#include "stdafx.h"
#include "file_path.h"
#include "debug.h"
#include "util.h"
#include <sys/types.h>
#include <sys/stat.h>

const char* FilePath::getPath() const {
  return fullPath.c_str();
}

const char* FilePath::getFileName() const {
  return filename.c_str();
}

time_t FilePath::getModificationTime() const {
  struct stat buf;
  stat(getPath(), &buf);
  return buf.st_mtim.tv_sec;
}

bool FilePath::hasSuffix(const string& suf) const {
  return filename.size() >= suf.size() && filename.substr(filename.size() - suf.size()) == suf;
}

FilePath FilePath::changeSuffix(const string& current, const string& newSuf) const {
  CHECK(hasSuffix(current));
  return FilePath(dir, filename.substr(0, filename.size() - current.size()) + newSuf);
}

FilePath::FilePath(const DirectoryPath& d, const string& f) : dir(d), filename(f), fullPath(dir.get() + "/"_s + f) {
}

std::ostream&operator <<(std::ostream& d, const FilePath& path) {
  return d << path.getPath();
}
