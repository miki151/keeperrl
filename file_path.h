#pragma once

#include "directory_path.h"

class FilePath {
  public:
  static FilePath fromFullPath(const string& path);
  const char* getPath() const;
  const char* getFileName() const;
  time_t getModificationTime() const;
  bool exists() const;
  bool hasSuffix(const string&) const;
  FilePath changeSuffix(const string& current, const string& newSuf) const;
  optional<string> readContents() const;
  void erase() const;

  bool operator==(const FilePath&) const;

  bool isAbsolute() const;
  FilePath absolute() const;

  void copyTo(FilePath) const;

  private:
  friend class DirectoryPath;
  FilePath(const DirectoryPath& d, const string& f);
  FilePath(string filename, string fullPath);

  string filename;
  string fullPath;
};


extern std::ostream& operator <<(std::ostream& d, const FilePath&);
