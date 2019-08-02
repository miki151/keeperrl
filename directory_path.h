#pragma once

#include "stdafx.h"
#include "my_containers.h"

class FilePath;

bool isAbsolutePath(const char* path);

class DirectoryPath {
  public:
  explicit DirectoryPath(string);

  FilePath file(const string&) const;
  DirectoryPath subdirectory(const string& s) const;
  bool exists() const;
  bool make();

  void createIfDoesntExist() const;
  void removeRecursively() const;
  vector<FilePath> getFiles() const;
  vector<string> getSubDirs() const;
  const char* getPath() const;

  static DirectoryPath current();
  static optional<string> copyFiles(DirectoryPath from, DirectoryPath to, bool recursive);

  bool isAbsolute() const;
  DirectoryPath absolute() const;

  private:
  friend class FilePath;
  friend ostream& operator << (ostream& d, const DirectoryPath& path);
  string path;
};

extern ostream& operator <<(ostream& d, const DirectoryPath& path);
