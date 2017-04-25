#pragma once

#include "directory_path.h"

class FilePath {
  public:
  const char* getPath() const;
  const char* getFileName() const;
  time_t getModificationTime() const;
  bool hasSuffix(const string&) const;
  FilePath changeSuffix(const string& current, const string& newSuf) const;

  private:
  friend class DirectoryPath;
  FilePath(const DirectoryPath& d, const string& f);

  DirectoryPath dir;
  string filename;
  string fullPath;
};


extern std::ostream& operator <<(std::ostream& d, const FilePath&);
