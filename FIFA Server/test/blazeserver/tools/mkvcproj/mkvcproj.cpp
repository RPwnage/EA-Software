#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <string>

#ifdef WIN32
#define CRLF "\n"
#else
#define CRLF "\r\n"
#endif

const char* gProjectTemplate =
    "Project(\"{%s}\") = \"%s\", \"%s.vcproj\", \"{%s}\"" CRLF
    "EndProject" CRLF;

char *gVSSolutionFromatVersion = nullptr;
char *gVSProjectFormatVersion = nullptr;

class DirNode
{
public: 
    typedef std::map<std::string,DirNode*> DirMap;
    typedef std::vector<std::string> FileList;
            
    static void setGendir(const char* gendir)
    {
        if (gendir == nullptr)
            sGendir[0] = '\0';
        else
        {
            if (strstr(gendir, "./") == gendir)
                gendir += 2;
            strcpy(sGendir, gendir);
            size_t len = strlen(sGendir);
            if (sGendir[len-1] != '/')
            {
                sGendir[len] = '/';
                sGendir[len+1] = '\0';
            }
        }
    }

    DirNode(DirNode* parent = nullptr)
        : mParent(parent)
    {       
    }       
        
    void addFile(const char* path)
    {
        if (path == nullptr)
            return; 

        const char* fullpath = path;

        // Strip of any leading ./ to ensure all files are rooted at the same level
        if (strstr(path, "./") == path)
            path += 2;

        if (strstr(path, sGendir) == path)
            path += strlen(sGendir);

        FileList components;
        char component[256];
        int compIdx = 0;
        for(int i = 0; path[i] != '\0'; i++)
        {
            if ((path[i] == '/') || (path[i] == '\\'))
            {
                component[compIdx] = '\0';
                components.push_back(component);
                compIdx = 0;
            }
            else
                component[compIdx++] = path[i];
        }   
        if (compIdx > 0)
        {
            component[compIdx] = '\0';
            components.push_back(component);
        }
    
        if (components.size() > 0)
        {
            DirNode* curr = this;
            for(size_t i = 0; i < components.size() - 1; i++)
            {
                curr = curr->addSubdir(components[i]);
            }
            curr->_addFile(fullpath);
        }
    }

    const DirMap& getSubdirs() const { return mSubdirs; }
    const FileList& getFiles() const { return mFiles; }

private:
    DirNode* mParent;
    DirMap mSubdirs;
    FileList mFiles;

    static char sGendir[256];

    DirNode* addSubdir(const std::string& dir)
    {
        DirNode* node;
        DirMap::iterator find = mSubdirs.find(dir);
        if (find != mSubdirs.end())
            node = find->second;
        else
        {
            node = new DirNode(this);
            mSubdirs[dir] = node;
        }
        return node;
    }

    void _addFile(const std::string& file)
    {
        mFiles.push_back(file);
    }
};

char DirNode::sGendir[256];

// Replace all occurences of the searchChar with the replaceChar in a copy of source.
static void replaceChar(const char* source, char* dest, int destSize, char searchChar, char replaceChar)
{
    int numCopied = 0; 
    while ((numCopied < (destSize-1)) && (*source != '\0'))
    {
        if (*source == searchChar)
        {
            *dest = replaceChar;
        }
        else
        {
            *dest = *source;
        }
        source++;
        dest++;
        numCopied++;
    }
    *dest = 0;
}

static void outputFiles(FILE* dst, const DirNode* node, int depth, const char* relativeOffsetDir)
{
    const DirNode::FileList& files = node->getFiles();

    const DirNode::DirMap& subdirs = node->getSubdirs();
    DirNode::DirMap::const_iterator i = subdirs.begin();
    DirNode::DirMap::const_iterator e = subdirs.end();
    for(; i != e; ++i)
    {
        /* Previously a filter was created for each sub dir only if the parent dir contained files.
        * Not only is this competely illogical but if multiple components have a sub dir
        * with the same name (eg. stress), duplicate filters get added to the vcproj. */
        /*if (files.size() > 0)
        {*/
            fprintf(dst, "%*s<Filter Name=\"%s\">\n", depth, "\t", i->first.c_str());
            outputFiles(dst, i->second, depth + 1, relativeOffsetDir);
            fprintf(dst, "%*s</Filter>\n", depth, "\t");
        /*}
        else
        {
            outputFiles(dst, i->second, depth + 1, relativeOffsetDir);
        }*/
    }

    DirNode::FileList::const_iterator fi = files.begin();
    DirNode::FileList::const_iterator fe = files.end();
    for(; fi != fe; ++fi)
    {
        char srcPath[1024];
        if (relativeOffsetDir != nullptr)
        {
            sprintf(srcPath, "%s/%s", relativeOffsetDir, fi->c_str());
        }
        else
        {
            strcpy(srcPath, fi->c_str());
        }
        char relativePath[1024];
        replaceChar(srcPath, relativePath, 1024, '/', '\\');

        fprintf(dst, "%*s<File RelativePath=\"%s\"></File>\n", depth, "\t", relativePath);
    }
}

static void addFiles(FILE* dst, char** files, int count, const char* relativeOffsetDir)
{
    DirNode* root = new DirNode();
    for(int i = 0; i < count; i++)
        root->addFile(files[i]);

    outputFiles(dst, root, 2, relativeOffsetDir);
}

static void replaceVariable(char* buf, char* find, size_t findlen, const char* replacement)
{
    char tmp[8192];

    strcpy(tmp, find + findlen);
    strcpy(find, replacement);
    strcat(find, tmp);
}

static void replaceArgs(char* buf, char* find, size_t findlen, char** args, int numArgs)
{
    char tmp[8192];

    strcpy(tmp, find + findlen);
    strcpy(find, "\0");
    int i;
    for(i=0; i<numArgs; i++) {
        strcat(find,args[i]);
        strcat(find," ");
    }
    strcat(find, tmp);
}

static int usage(const char* prg)
{
    fprintf(stderr, "Usage: %s -vVSVERSION -s -m makefiles... -c configfiles... -P projectFileDir -p projectFiles... -d defaultProject -o outputFile\n", prg);
    fprintf(stderr, "Usage: %s -vVSVERSION -p -t templatefile -p projectFile -m makeTarget -mak maketype -c configfiles -o outputFile -g gendir -r relativeOffsetDir -s srcfiles...\n",
            prg);
    fprintf(stderr, "Usage: %s -vVSVERSION -u -t templatefile -p destFile -w workingdir -args commandarg1 [commandarg2]...\n",
            prg);
    fprintf(stderr, "Where VSVERSION is 2005 or 2008.\n");
    return 1;
}

static void generateUuid(char* buf, int idx)
{
    // Possibly replace this with proper UUID generation in the future.
    sprintf(buf, "00000000-0000-0000-0000-%012X", idx);
}

static int generateSolutionFile(int argc, char** argv)
{
    char** makefiles = nullptr;
    int makefileCount = 0;
    char** projects = nullptr;
    int projectsCount = 0;
    char* defaultProject = "";
    char* outputFile = nullptr;
    char uuids[256][40];
    char solutionUuid[40];
    char* projectFileDir = nullptr;
    char** cfgfiles = nullptr;
    int cfgfileCount = 0;

    for(int arg = 3; arg < argc; arg++)
    {
        if (strcmp(argv[arg], "-m") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            makefiles = &argv[arg];
            while ((arg < argc) && (argv[arg][0] != '-'))
            {
                makefileCount++;
                arg++;
            }
            arg--;
        }
        else if (strcmp(argv[arg], "-P") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            projectFileDir = argv[arg];
        }
        else if (strcmp(argv[arg], "-p") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            projects = &argv[arg];
            while ((arg < argc) && (argv[arg][0] != '-'))
            {
                projectsCount++;
                arg++;
            }
            arg--;
        }
        else if (strcmp(argv[arg], "-d") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            defaultProject = argv[arg];
        }
        else if (strcmp(argv[arg], "-o") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            outputFile = argv[arg];
        }
        else if (strcmp(argv[arg], "-c") == 0)
        {
            // Count the config files
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            cfgfiles = &argv[arg];
            while ((arg < argc) && (argv[arg][0] != '-'))
            {
                cfgfileCount++;
                arg++;
            }
            arg--;
        }
    }

    if ((projects == nullptr) || (outputFile == nullptr))
        return usage(argv[0]);

    FILE* dst = fopen(outputFile, "w");
    if (dst == nullptr)
    {
        fprintf(stderr, "Failed to create solution file '%s'\n", outputFile);
        exit(1);
    }

    generateUuid(solutionUuid, -1);

    if (strcmp(gVSSolutionFromatVersion, "9.00") == 0)
    {
        fprintf(dst, "Microsoft Visual Studio Solution File, Format Version 9.00" CRLF);
        fprintf(dst, "# Visual Studio 2005" CRLF);
    }
    else if (strcmp(gVSSolutionFromatVersion, "10.00") == 0)
    {
        fprintf(dst, "Microsoft Visual Studio Solution File, Format Version 10.00" CRLF);
        fprintf(dst, "# Visual Studio 2008" CRLF);
    }
    else
    {
        fprintf(stderr, "We only support VS Solution File version 9.00 or 10.00." CRLF);
        exit(1);
    }
    
    // Ensure that the default project is listed first to Visual Studio will make it the default
    for(int i = 0; i < projectsCount; i++)
    {
        if (strcmp(projects[i], defaultProject) == 0)
        {
            generateUuid(uuids[i], i);
            char vcproj[256];
            if (projectFileDir != nullptr)
                sprintf(vcproj, "%s/%s", projectFileDir, projects[i]);
            else
                strcpy(vcproj, projects[i]);
            char relativePath[1024];
            replaceChar(vcproj, relativePath, 1024, '/', '\\');
            fprintf(dst, gProjectTemplate, solutionUuid, projects[i], relativePath, uuids[i]);
            break;
        }
    }

    // Output everything but the default project
    for(int i = 0; i < projectsCount; i++)
    {
        if (strcmp(projects[i], defaultProject) != 0)
        {
            generateUuid(uuids[i], i);
            char vcproj[256];
            if (projectFileDir != nullptr)
                sprintf(vcproj, "%s/%s", projectFileDir, projects[i]);
            else
                strcpy(vcproj, projects[i]);
            char relativePath[1024];
            replaceChar(vcproj, relativePath, 1024, '/', '\\');
            fprintf(dst, gProjectTemplate, solutionUuid, projects[i], relativePath, uuids[i]);
        }
    }

    // This is a counter for all of the extra projects like make files and configurations.
    int extraProjects = projectsCount;

    // Output a project for the makefiles
    char makefilesUuid[40];
    char tmpUuid[40];
    strcpy(tmpUuid, "2150E333-8FDC-42A3-9474-1A3956D46DE8");  // This is the uuid for a Visual Studio Folder Project
    generateUuid(makefilesUuid, extraProjects);
    fprintf(dst, "Project(\"{%s}\") = \"Makefiles\", \"Makefiles\", \"{%s}\"" CRLF
        "\tProjectSection(SolutionItems) = preProject" CRLF, tmpUuid, makefilesUuid);
    for(int i = 0; i < makefileCount; i++)
    {
        char relativePath[1024];
        replaceChar(makefiles[i], relativePath, 1024, '/', '\\');
        fprintf(dst, "\t\t%s = %s" CRLF, relativePath, relativePath);
    }
    fprintf(dst, "\tEndProjectSection" CRLF
        "EndProject" CRLF);

    // Increment the next project.
    extraProjects++;

    // Output a project for the configuration files
    if(cfgfileCount > 0) {
        char cfgfilesUuid[40];
        strcpy(tmpUuid, "2150E333-8FDC-42A3-9474-1A3956D46DE8");  // This is the uuid for a Visual Studio Folder Project
        generateUuid(cfgfilesUuid, extraProjects);
        fprintf(dst, "Project(\"{%s}\") = \"etc\", \"etc\", \"{%s}\"" CRLF
            "\tProjectSection(SolutionItems) = preProject" CRLF, tmpUuid, cfgfilesUuid);
        for(int i = 0; i < cfgfileCount; i++)
        {
            char relativePath[1024];
            replaceChar(cfgfiles[i], relativePath, 1024, '/', '\\');
            printf("%s\n",relativePath);
            fprintf(dst, "\t\t%s = %s" CRLF, relativePath, relativePath);
        }
        fprintf(dst, "\tEndProjectSection" CRLF
            "EndProject" CRLF);
    }

    // Global section

    fprintf(dst, "Global" CRLF);
    fprintf(dst, "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution" CRLF);
    fprintf(dst, "\t\tDebug|Win32 = Debug|Win32" CRLF);
    fprintf(dst, "\t\tRelease|Win32 = Release|Win32" CRLF);
    fprintf(dst, "\tEndGlobalSection" CRLF);
    fprintf(dst, "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution" CRLF);
    for(int i = 0; i < projectsCount; i++)
    {
        fprintf(dst, "\t\t{%s}.Debug|Win32.ActiveCfg = Debug|Win32" CRLF, uuids[i]);
        fprintf(dst, "\t\t{%s}.Debug|Win32.Build.0 = Debug|Win32" CRLF, uuids[i]);
        fprintf(dst, "\t\t{%s}.Release|Win32.ActiveCfg = Release|Win32" CRLF, uuids[i]);
        fprintf(dst, "\t\t{%s}.Release|Win32.Build.0 = Release|Win32" CRLF, uuids[i]);
    }
    fprintf(dst, "\tEndGlobalSection" CRLF);
    fprintf(dst, "\tGlobalSection(SolutionProperties) = preSolution" CRLF);
    fprintf(dst, "\t\tHideSolutionNode = FALSE" CRLF);
    fprintf(dst, "\tEndGlobalSection" CRLF);
    fprintf(dst, "EndGlobal" CRLF);
    fclose(dst);
    return 0;
}

static bool addIncludePath(const char* relativeOffsetDir, char includes[][256], int count, char* path)
{
    char relativePath[256];
    if (relativeOffsetDir != nullptr)
    {
        char outputFilePath[256];
        sprintf(outputFilePath, "%s/%s", relativeOffsetDir, path);
        replaceChar(outputFilePath, relativePath, sizeof(relativePath), '/', '\\');
    }
    else
        replaceChar(path, relativePath, sizeof(relativePath), '/', '\\');

    bool found = false;
    for(int p = 0; (p < count) && !found; p++)
    {
        if (strcmp(includes[p], relativePath) == 0)
            found = true;
    }
    if (!found)
    {
        strcpy(includes[count], relativePath);
        return true;
    }
    return false;
}

//Generate Visual Studio user settings (.user) file
//Usage: -u -t <template> -p <userfile> -w <workingdir> -args <arg1> [<arg2>] ...
static int generateUserFile(int argc, char**argv)
{
    char* templateFile = nullptr;
    char* userFile = nullptr;
    char* workingDir = nullptr;
    char emptyString[1] = {'\0'};
    char** commandArgs = nullptr;
    int numArgs = 0;

    //Parse command args
    for(int arg=3; arg < argc; arg++)
    {
        if(strcmp(argv[arg],"-w") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            workingDir = argv[arg];
        }
        else if(strcmp(argv[arg],"-t") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            templateFile = argv[arg];
        }
        else if(strcmp(argv[arg],"-p") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            userFile = argv[arg];
        }
        else if(strcmp(argv[arg],"-args") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);

            //Args must be placed at the end
            commandArgs = &argv[arg];
            while ((arg < argc))
            {
                numArgs++;
                arg++;
            }
            arg--;
        }
    }

    if ((templateFile == nullptr) || (userFile == nullptr))
    {
        return usage(argv[0]);
    }

    //Open template file
    FILE* src = fopen(templateFile, "r");
    if(src == nullptr)
    {
        fprintf(stderr, "Failed to open template file '%s'\n", templateFile);
        exit(1);
    }

    FILE* dst = fopen(userFile, "w");
    if (dst == nullptr)
    {
        fprintf(stderr, "Failed to open target user settings file '%s'\n", userFile);
    }

    char buf[8192];
    while (fgets(buf, sizeof(buf), src) != nullptr)
    {
        char* find = nullptr;
        
        do {
            find = strstr(buf, "%WORKINGDIR%");
            if (find != nullptr)
                if(workingDir != nullptr)
                    replaceVariable(buf, find, strlen("%WORKINGDIR%"), workingDir);
                else
                    replaceVariable(buf, find, strlen("%WORKINGDIR%"), emptyString);
        } while(find != nullptr);

        do {
            find = strstr(buf, "%CMDARGS%");
            if (find != nullptr)
                if(numArgs != 0)
                    replaceArgs(buf, find, strlen("%CMDARGS%"), commandArgs, numArgs);
                else
                    replaceVariable(buf, find, strlen("%CMDARGS%"), emptyString);
        } while(find != nullptr);

        do {
            find = strstr(buf, "%VSPROJECTVERSION%");
            if (find != nullptr)
                replaceVariable(buf, find, strlen("%VSPROJECTVERSION%"), gVSProjectFormatVersion);
        } while(find != nullptr);

        fputs(buf, dst); 
    }

    fclose(src);
    fclose(dst);
    return 0;
}

static int generateProjectFile(int argc, char** argv)
{
    char* templateFile = nullptr;
    char* vcprojFile = nullptr;
    char* makeTarget = nullptr;
    char* outputFile = nullptr;
    char** srcs = nullptr;
    int numSrcs = 0;
    char** cfgs = nullptr;
    int numCfgs = 0;
    char** incFiles = nullptr;
    int incFileCount = 0;
    char* gendir = "";
    char* relativeOffsetDir = nullptr;
    char* makefiletype = nullptr;

    for(int arg = 3; arg < argc; arg++)
    {
        if (strcmp(argv[arg], "-s") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            srcs = &argv[arg];
            while ((arg < argc) && (argv[arg][0] != '-'))
            {
                numSrcs++;
                arg++;
            }
            arg--;
        }
        else if (strcmp(argv[arg], "-c") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            cfgs = &argv[arg];
            while ((arg < argc) && (argv[arg][0] != '-'))
            {
                numCfgs++;
                arg++;
            }
            arg--;
        }
        else if (strcmp(argv[arg], "-mak") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            makefiletype = argv[arg];
        }
        else if (strcmp(argv[arg], "-I") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            incFiles = &argv[arg];
            while ((arg < argc) && (argv[arg][0] != '-'))
            {
                incFileCount++;
                arg++;
            }
            arg--;
        }
        else if (strcmp(argv[arg], "-g") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            gendir = argv[arg];
        }
        else if (strcmp(argv[arg], "-r") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            relativeOffsetDir = argv[arg];
        }
        else if (strcmp(argv[arg], "-m") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            makeTarget = argv[arg];
        }
        else if (strcmp(argv[arg], "-p") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            vcprojFile = argv[arg];
        }
        else if (strcmp(argv[arg], "-t") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            templateFile = argv[arg];
        }
        else if (strcmp(argv[arg], "-o") == 0)
        {
            arg++;
            if (arg == argc)
                return usage(argv[0]);
            outputFile = argv[arg];
        }
    }


    if ((templateFile == nullptr) || (vcprojFile == nullptr) || (makeTarget == nullptr)
            || (outputFile == nullptr) || (numSrcs == 0))
    {
        return usage(argv[0]);
    }

    // Generate vcproj files

    DirNode::setGendir(gendir);

    // Setup include paths
    char includes[256][256];
    int includeCount = 0;
    char includePaths[8192];
    for(int i = 0; i < incFileCount; i++)
    {
        if (addIncludePath(relativeOffsetDir, includes, includeCount, incFiles[i]))
            includeCount++;
    }
    includePaths[0] = '\0';
    char* ip = includePaths;
    bool first = true;
    for(int i = 0; i < includeCount; i++)
    {
        if (first)
        {
            ip += sprintf(ip, "%s", includes[i]);
            first = false;
        }
        else
        {
            ip += sprintf(ip, ";%s", includes[i]);
        }
    }

    FILE* src = fopen(templateFile, "r");
    if (src == nullptr)
    {
        fprintf(stderr, "Failed to open template file '%s'\n", templateFile);
        exit(1);
    }

    FILE* dst = fopen(vcprojFile, "w");
    if (dst == nullptr)
    {
        fprintf(stderr, "Failed to open target vcproj file '%s'\n", vcprojFile);
    }

    char buf[8192];
    while (fgets(buf, sizeof(buf), src) != nullptr)
    {
        char* find = strstr(buf, "%TARGET%");
        if (find != nullptr)
        {
            if (relativeOffsetDir != nullptr)
            {
                char outputFilePath[256];
                sprintf(outputFilePath, "%s/%s", relativeOffsetDir, outputFile);
                replaceVariable(buf, find, strlen("%TARGET%"), outputFilePath);
            }
            else
            {
                replaceVariable(buf, find, strlen("%TARGET%"), outputFile);
            }
        }

        find = strstr(buf, "%MAKE_TARGET%");
        if (find != nullptr)
        {
            replaceVariable(buf, find, strlen("%MAKE_TARGET%"), makeTarget);
        }

        find = strstr(buf, "%INCLUDE_PATHS%");
        if (find != nullptr)
        {
            replaceVariable(buf, find, strlen("%INCLUDE_PATHS%"), includePaths);
        }

        find = strstr(buf, "%VSPROJECTVERSION%");
        if (find != nullptr)
        {
            replaceVariable(buf, find, strlen("%VSPROJECTVERSION%"), gVSProjectFormatVersion);
        }

        fputs(buf, dst);
        if (strstr(buf, "<Files>") != nullptr)
        {
            char makefilePath[1024];
            char relativeMakefilePath[1024];
            if (strcmp(makefiletype, "component") == 0)
            {
                // build the path and sanitize slashes for windows
                sprintf(makefilePath, "%s\\component\\%s\\%s.mak", relativeOffsetDir, makeTarget, makeTarget);
                replaceChar(makefilePath, relativeMakefilePath, 1024, '/', '\\');
            }
            else
            {
                // build the path and sanitize slashes for windows
                sprintf(makefilePath, "%s\\make\\%s\\%s.mak", relativeOffsetDir, makefiletype, makeTarget);
                replaceChar(makefilePath, relativeMakefilePath, 1024, '/', '\\');
            }

            // Add the make file
            fprintf(dst, "\t<File RelativePath=\"%s\"></File>\n", relativeMakefilePath);

            if (numCfgs > 0)
            {
                // Add the config files as specified in the make file CFG :=
                fprintf(dst, "\t<Filter Name=\"etc\">\n");
                addFiles(dst, cfgs, numCfgs, relativeOffsetDir);
                fprintf(dst, "\t</Filter>\n");
            }

            // Add the source files
            addFiles(dst, srcs, numSrcs, relativeOffsetDir);
        }
    }
    fclose(src);
    fclose(dst);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3)
        exit(usage(argv[0]));

    if (strcmp(argv[1], "-v2005") == 0)
    {
        gVSSolutionFromatVersion = "9.00";
        gVSProjectFormatVersion = "8.00";
    }
    else if (strcmp(argv[1], "-v2008") == 0)
    {
        gVSSolutionFromatVersion = "10.00";
        gVSProjectFormatVersion = "9.00";
    }
    else
        exit(usage(argv[0]));
        
    if (strcmp(gVSSolutionFromatVersion, "9.00") != 0 && strcmp(gVSSolutionFromatVersion, "10.00") != 0)
    {
        printf("We support only Visual Studio project file versions 9 and 10.\n");
        exit(1);
    }
    
    if (strcmp(argv[2], "-s") == 0)
        return generateSolutionFile(argc, argv);
    else if (strcmp(argv[2], "-p") == 0)
        return generateProjectFile(argc, argv);
    else if (strcmp(argv[2], "-u") == 0)
        return generateUserFile(argc, argv);

    exit(usage(argv[0]));
}

