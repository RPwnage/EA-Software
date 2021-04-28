import SimpleCodeStripper

SimpleCodeStripper.Strip(
        sourceSubdirectories = ['include', 'source'],
        additionalSourceFoldersToSkip = ['doc'],
        folderComponentsToExcludeFromCopy=['build', 'scripts'],
        filesToCopy = ['.\scripts\initialize.xml']
        )
