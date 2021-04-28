import SimpleCodeStripper

SimpleCodeStripper.Strip(
        sourceSubdirectories = ['include', 'source', 'tools'],
        folderComponentsToExcludeFromCopy=['build', 'test']
        )

