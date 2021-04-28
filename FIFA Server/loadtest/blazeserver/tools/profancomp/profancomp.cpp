#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef WIN32
#include <sys/stat.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "framework/lobby/platform/platform.h"
#include "framework/lobby/profan.h"

namespace profancomp
{

int profancomp_main(int argc, char **argv)
{
    ProfanRef *pRef;
    int32_t iFd;
    struct stat Stat;
    char *pBuf;
    int32_t iRead;
    int32_t iTotal;
    int32_t iFlags = 0;
    int32_t iRc;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s input-file output-file\n", argv[0]);
        exit(1);
    }

#ifdef WIN32
    iFlags |= O_BINARY;
#endif

    // Open, read, and parse the import file
    iFd = open(argv[1], O_RDONLY | iFlags);
    if (iFd < 0)
    {
        perror("Failed to open source file");
        exit(2);
    }
    if (fstat(iFd, &Stat) != 0)
    {
        perror("Failed to determine source file length");
        exit(3);
    }

    pBuf = new char[Stat.st_size+1];
    iTotal = 0;
    while (iTotal < Stat.st_size)
    {
        iRead = read(iFd, pBuf + iTotal, Stat.st_size - iTotal);
        if (iRead <= 0)
            break;
        iTotal += iRead;
    }
    if (iTotal != Stat.st_size)
    {
        perror("Unable to read source file contents");
        exit(4);
    }
    pBuf[Stat.st_size] = '\0';
    close(iFd);

    pRef = ProfanCreate();
    ProfanImport(pRef, pBuf);

    // Write the compiled state machine to the output file
    iFd = open(argv[2], O_WRONLY | O_CREAT | iFlags, 0644);
    if (iFd < 0)
    {
        perror("Failed to open output file");
        exit(5);
    }

    if (ProfanSave(pRef, iFd) < 0)
    {
        perror("Error saving filter state");
        exit(6);
    }
    close(iFd);
    ProfanDestroy(pRef);

    // Attempt to reload the compiled file
    iFd = open(argv[2], O_RDONLY | iFlags);
    if (iFd < 0)
    {
        perror("Failed to re-open output file for validation");
        exit(7);
    }

    pRef = ProfanCreate();
    iRc = ProfanLoad(pRef, iFd);
    if (iRc < 0)
    {
        fprintf(stderr, "Failed to reload output file: %d\n", iRc);
        exit(8);
    }

    return (0);
}

}
