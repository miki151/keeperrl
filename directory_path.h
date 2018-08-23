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
  vector<FilePath> getFiles() const;

  private:
  friend class FilePath;
  friend ostream& operator << (ostream& d, const DirectoryPath& path);
  const char* get() const;
  string path;
};

extern ostream& operator <<(ostream& d, const DirectoryPath& path);
