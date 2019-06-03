#pragma once

#include "stdafx.h"
#include "my_containers.h"

class FilePath;

class DirectoryPath {
  public:
  explicit DirectoryPath(const string&);

  FilePath file(const string&) const;
  DirectoryPath subdirectory(const string& s) const;
  bool exists() const;
  void createIfDoesntExist() const;
  void removeRecursively() const;
  vector<FilePath> getFiles() const;
  vector<string> getSubDirs() const;
  const char* getPath() const;

  private:
  friend class FilePath;
  friend ostream& operator << (ostream& d, const DirectoryPath& path);
  string path;
};

extern ostream& operator <<(ostream& d, const DirectoryPath& path);
