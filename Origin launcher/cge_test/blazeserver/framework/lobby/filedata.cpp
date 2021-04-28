/*
 * FileData.c
 *
 * Version 1.0
 *
 * Revision History:
 *     19-Apr-2000  GWS  Initial version
 */

/*
 This module implements simple data file access, Entire files are loaded
 and stored as single units. It also supports timing for basic caching.
 */

// Includes
#include "framework/blazedefines.h"
#include <EAStdC/EASprintf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
#include <io.h>
#define access _access
#define F_OK 0
#else
#include <unistd.h>     // access
#endif

#include "platform/platform.h"

#include "filedata.h"


/*
 * Setup module state (none for this module)
 *
 * entry: none
 * exit: bogus reference pointer
 */
FileDataRef *FileDataCreate()
{
    return((FileDataRef *)1);
}


/*
 * Destroy module state (none for this module)
 *
 * entry: reference pointer
 * exit: none
 */
void FileDataDestroy(FileDataRef *ref)
{
    return;
}


/*
 * Load in a data file from disk
 *
 * entry: fname=filename, size=return pointer for size (nullptr=ignore)
 * exit: pointer to file data (must call FileDataFree to release)
 */
void *FileDataLoad(FileDataRef *ref, const char *fname, int32_t *size)
{
    char *data;
    FILE *fd;
    
    // validate filename
    if ((fname == nullptr) || (fname[0] == 0))
        return(nullptr);

    // default return value
    if (size != nullptr)
        *size = -1;

    // open the file
    fd = fopen(fname, "rb");
    if (fd == nullptr)
        return(nullptr);

    // get the size
    fseek(fd, 0, SEEK_END);
    long sz = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    if (sz < 0) {
        fclose(fd);
        return(nullptr);
    }
    size_t fsize = (size_t)sz;

    // allocate storage
    data = (char *)malloc(fsize+1);
    if (data == nullptr) {
        fclose(fd);
        return(nullptr);
    }

    // read in the data
    if (fread(data, 1, fsize, fd) != fsize) {
        fclose(fd);
        free(data);
        return(nullptr);
    }

    // terminate the data
    data[fsize] = 0;

    // return the data
    fclose(fd);
    if (size != nullptr)
        *size = fsize;
    return(data);
}


/*
 * Release memory from prior load call
 *
 * entry: memory pointer
 * exit: none
 */
void FileDataFree(FileDataRef *ref, void *data)
{
    if (data != nullptr)
        free(data);
    return;
}


/*
 * Save a file to disk
 *
 * entry: fname=filename, data=data pointer, size=length of data
 * exit: number of bytes saved or negative for error
 */
int32_t FileDataSave(FileDataRef *ref, const char *fname, const void *data, int32_t size)
{
    FILE *fd;

    // validate filename
    if ((fname == nullptr) || (fname[0] == 0))
        return(-1);

    // allow auto-sizing
    if (size < 0)
        size = strlen((const char *)data);

    // open file for writing
    fd = fopen(fname, "wb");
    if (fd == nullptr)
        return(-1);

    // write the data
    size = fwrite(data, 1, size, fd);

    // return the results
    fclose(fd);
    return(size);
}


/*
 * Append a record to an existing file
 *
 * entry: fname=filename, data=data pointer, size=length of data
 * exit: number of bytes saved or negative for error
 */
int32_t FileDataAppend(FileDataRef *ref, const char *fname, const void *data, int32_t size, int32_t clamp)
{
    FILE *fd;

    // validate filename
    if ((fname == nullptr) || (fname[0] == 0))
        return(-1);

    // allow auto-sizing
    if (size < 0)
        size = strlen((const char *)data);

    // open file for writing
    fd = fopen(fname, "ab");
    if (fd == nullptr)
        return(-1);

    // see if data will fit
    if ((clamp > 0) && (ftell(fd) > clamp)) {
        fclose(fd);
        return(-2);
    }

    // write the data
    size = fwrite(data, 1, size, fd);

    // return the results
    fclose(fd);
    return(size);
}


/*
 * Archive an existing file
 *
 * entry: srcname=source filename (with path info), arcname=archive file (with path info)
 * exit: zero=success, negative=error
 */
int32_t FileDataArchive(FileDataRef *ref, const char *srcname, const char *arcname)
{
    int32_t revn;
    char date[16];
    const char *s;
    char pattern[256];
    char dstname[256];
    time_t epoch = time(nullptr);
    struct tm *tm;
    
    tm = localtime( &epoch );  // Convert the current time to the local time

    // verify that source exists
    if (access(srcname, F_OK) < 0)
        return(-1);

    // create the datestamp
    sprintf(date, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);

    // create the target pattern
    strcpy(pattern, arcname);
    s = strrchr(arcname, '.');
    if (s != nullptr)
        sprintf(strrchr(pattern, '.'), "%s%s", "-%s%02d", s);
    else
        sprintf(strchr(pattern, 0), "%s", "-%s%02d");

    // attempt to do this 100 times
    for (revn = 1; revn < 100; ++revn) {
        // setup target name
        EA::StdC::Sprintf(dstname, pattern, date, revn);
        // rename if target does not exist
        if (access(dstname, F_OK) < 0) {
            // attempt to move the file
            return(rename(srcname, dstname) >= 0 ? 0 : -2);
        }
    }

    // no open slots
    return(-3);
}



