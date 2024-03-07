/*
   miniunz.c
   Version 1.01e, February 12th, 2005

   Copyright (C) 1998-2005 Gilles Vollant
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

//#ifdef unix
#ifndef _WIN32
# include <unistd.h>
# include <utime.h>
#else
# include <filesystem>
#endif
/*#else
# include <direct.h>
# include <io.h>
#endif*/

#include "unzip.h"

#undef WIN32

#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)
#define MAXFILENAME (256)

#ifdef WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif

#include "util.h"
/*
  mini unzip, demo of unzip package

  usage :
  Usage : miniunz [-exvlo] file.zip [file_to_extract] [-d extractdir]

  list the file in the zipfile, and print the content of FILE_ID.ZIP or README.TXT
    if it exists
*/


/* change_file_date : change the date/time of a file
    filename : the filename of the file where date/time must be modified
    dosdate : the new date at the MSDos format (4 bytes)
    tmu_date : the SAME new date at the tm_unz format */
void change_file_date(
    const char *filename,
    uLong dosdate,
    tm_unz tmu_date)
{
#ifdef WIN32
  HANDLE hFile;
  FILETIME ftm,ftLocal,ftCreate,ftLastAcc,ftLastWrite;

  hFile = CreateFile(filename,GENERIC_READ | GENERIC_WRITE,
                      0,nullptr,OPEN_EXISTING,0,nullptr);
  GetFileTime(hFile,&ftCreate,&ftLastAcc,&ftLastWrite);
  DosDateTimeToFileTime((WORD)(dosdate>>16),(WORD)dosdate,&ftLocal);
  LocalFileTimeToFileTime(&ftLocal,&ftm);
  SetFileTime(hFile,&ftm,&ftLastAcc,&ftm);
  CloseHandle(hFile);
#else
#ifdef unix
  struct utimbuf ut;
  struct tm newdate;
  newdate.tm_sec = tmu_date.tm_sec;
  newdate.tm_min=tmu_date.tm_min;
  newdate.tm_hour=tmu_date.tm_hour;
  newdate.tm_mday=tmu_date.tm_mday;
  newdate.tm_mon=tmu_date.tm_mon;
  if (tmu_date.tm_year > 1900)
      newdate.tm_year=tmu_date.tm_year - 1900;
  else
      newdate.tm_year=tmu_date.tm_year ;
  newdate.tm_isdst=-1;

  ut.actime=ut.modtime=mktime(&newdate);
  utime(filename,&ut);
#endif
#endif
}


/* mymkdir and change_file_date are not 100 % portable
   As I don't know well Unix, I wait feedback for the unix portion */

optional<string> mymkdir(
    const char* dirname)
{
#ifdef _MSC_VER
    std::error_code err;
    if(!std::filesystem::create_directory(dirname, err))
#elif !defined(WINDOWS)
    if (mkdir (dirname,0775) != 0)
#else
    if (mkdir (dirname) != 0)
#endif
      return string(strerror(errno));
    else
      return none;
}

int makedir (
    const char *newdir)
{
  char *buffer ;
  char *p;
  int  len = (int)strlen(newdir);

  if (len <= 0)
    return 0;

  buffer = (char*)malloc(len+1);
  strcpy(buffer,newdir);

  if (buffer[len-1] == '/') {
    buffer[len-1] = '\0';
  }
  if (!mymkdir(buffer))
    {
      free(buffer);
      return 1;
    }

  p = buffer+1;
  while (1)
    {
      char hold;

      while(*p && *p != '\\' && *p != '/')
        p++;
      hold = *p;
      *p = 0;
      if (!!mymkdir(buffer) && (errno == ENOENT))
        {
          printf("couldn't create directory %s\n",buffer);
          free(buffer);
          return 0;
        }
      if (hold == 0)
        break;
      *p++ = hold;
    }
  free(buffer);
  return 1;
}

optional<string> do_extract_currentfile(
    unzFile uf,
    const string& targetDir)
{
    char filename_inzip[256];
    char* filename_withoutpath;
    char* p;
    int err=UNZ_OK;
    FILE *fout=nullptr;
    void* buf;
    uInt size_buf;

    unz_file_info file_info;
    uLong ratio=0;
    err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),nullptr,0,nullptr,0);

    if (err!=UNZ_OK)
    {
        return "error " + toString(err) + " with zipfile in unzGetCurrentFileInfo";
    }

    size_buf = WRITEBUFFERSIZE;
    buf = (void*)malloc(size_buf);
    if (buf==nullptr)
    {
        return "Error allocating memory"_s;
    }

    p = filename_withoutpath = filename_inzip;
    while ((*p) != '\0')
    {
        if (((*p)=='/') || ((*p)=='\\'))
            filename_withoutpath = p+1;
        p++;
    }

    if ((*filename_withoutpath)=='\0')
    {
        auto dir = targetDir + "/" + filename_inzip;
//        if (auto err =
            mymkdir(dir.data());
//                )
  //        return "Failed to create directory \""_s + dir + "\"";
    }
    else
    {
        const char* write_filename;
        int skip=0;

        write_filename = filename_inzip;

        err = unzOpenCurrentFile(uf);
        if (err!=UNZ_OK)
        {
            return "error " + toString(err) + " with zipfile in unzOpenCurrentFile";
        }

        if ((skip==0) && (err==UNZ_OK))
        {
            auto filename = targetDir + "/" + write_filename;
            fout=fopen(filename.data(),"wb");

            /* some zipfile don't contain directory alone before file */
            if ((fout==nullptr) && (filename_withoutpath!=(char*)filename_inzip))
            {
                char c=*(filename_withoutpath-1);
                *(filename_withoutpath-1)='\0';
                makedir((targetDir + "/" + write_filename).data());
                *(filename_withoutpath-1)=c;
                fout=fopen(filename.data(),"wb");
            }

            if (fout==nullptr)
            {
                return "error creating file "_s + filename + ": " + string(strerror(errno));
            }
        }

        if (fout!=nullptr)
        {
            do
            {
                err = unzReadCurrentFile(uf,buf,size_buf);
                if (err<0)
                {
                    return "error " + toString(err) + " with zipfile in unzReadCurrentFile";
                }
                if (err>0)
                    if (fwrite(buf,err,1,fout)!=1)
                    {
                        return "error in writing extracted file"_s;
                    }
            }
            while (err>0);
            if (fout)
                    fclose(fout);

            /*if (err==0)
                change_file_date(write_filename,file_info.dosDate,
                                 file_info.tmu_date);*/
        }

        if (err==UNZ_OK)
        {
            err = unzCloseCurrentFile (uf);
            if (err!=UNZ_OK)
            {
                return "error " + toString(err) + " with zipfile in unzCloseCurrentFile";
            }
        }
        else
            unzCloseCurrentFile(uf); /* don't lose the error */
    }

    free(buf);
    return none;
}


optional<string> do_extract(
    unzFile uf,
    const char* targetDir)
{
    uLong i;
    unz_global_info gi;
    int err;
    FILE* fout=nullptr;

    err = unzGetGlobalInfo (uf,&gi);
    if (err!=UNZ_OK)
        return "error " + toString(err) + " with zipfile in unzGetGlobalInfo";

    for (i=0;i<gi.number_entry;i++)
    {
        if (auto err = do_extract_currentfile(uf, targetDir))
            return err;

        if ((i+1)<gi.number_entry)
        {
            err = unzGoToNextFile(uf);
            if (err!=UNZ_OK) {
                return "error " + toString(err) + " with zipfile in unzGoToNextFile";
            }
        }
    }

    return none;
}

optional<string> unzip(const string& zipfilename, const char* dirname)
{
    int i;
    int opt_do_list=0;
    int opt_do_extract=1;
    int opt_overwrite=0;
    int opt_extractdir=0;
    unzFile uf=nullptr;

#    ifdef USEWIN32IOAPI
    zlib_filefunc_def ffunc;
#    endif

#    ifdef USEWIN32IOAPI
    fill_win32_filefunc(&ffunc);
    uf = unzOpen2(zipfilename.data(),&ffunc);
#    else
    uf = unzOpen(zipfilename.data());
#    endif

    if (uf==nullptr)
    {
        return "Cannot open " + zipfilename;
    }

    return do_extract(uf,dirname);
    unzCloseCurrentFile(uf);

    return none;
}
