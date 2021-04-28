/*
 * FileData.h
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

#ifndef __filedata_h
#define __filedata_h

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FileDataRef FileDataRef;

/*
 * Setup module state (none for this module)
 *
 * entry: none
 * exit: bogus reference pointer
 */
FileDataRef *FileDataCreate();

/*
 * Destroy module state (none for this module)
 *
 * entry: reference pointer
 * exit: none
 */
void FileDataDestroy(FileDataRef *ref);

/*
 * Load in a data file from disk
 *
 * entry: fname=filename, size=return pointer for size (nullptr=ignore)
 * exit: pointer to file data (must call FileDataFree to release)
 */
void *FileDataLoad(FileDataRef *ref, const char *fname, int32_t *size);

/*
 * Release memory from prior load call
 *
 * entry: memory pointer
 * exit: none
 */
void FileDataFree(FileDataRef *ref, void *data);

/*
 * Save a file to disk
 *
 * entry: fname=filename, data=data pointer, size=length of data
 * exit: number of bytes saved or negative for error
 */
int32_t FileDataSave(FileDataRef *ref, const char *fname, const void *data, int32_t size);

/*
 * Append a record to an existing file
 *
 * entry: fname=filename, data=data pointer, size=length of data
 * exit: number of bytes saved or negative for error
 */
int32_t FileDataAppend(FileDataRef *ref, const char *fname, const void *data, int32_t size, int32_t clamp);

/*
 * Archive an existing file
 *
 * entry: srcname=source filename (with path info), arcname=archive file (with path info)
 * exit: zero=success, negative=error
 */
int32_t FileDataArchive(FileDataRef *ref, const char *srcname, const char *arcname);

#ifdef __cplusplus
}
#endif

#endif /* __filedata_h */
