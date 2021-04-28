import org.antlr.runtime.*;
import org.antlr.runtime.tree.*;
import org.antlr.stringtemplate.*;

import java.io.*;
import java.util.*;
import java.util.Map.Entry;

import java.nio.file.Path;
import java.nio.file.Files;
import java.nio.file.attribute.BasicFileAttributes;

class PathGroupLoaderMulti extends PathGroupLoader
{
    PathGroupLoaderMulti(List<String> _dirs)
    {
        super("", null);    
        dirs = _dirs;
    }
}

public class TdfComp 
{

    static ArrayList<String> mBaseIncludePaths = new ArrayList<String>();
    static ArrayList<TdfFileInfo> mIncludePaths = new ArrayList<TdfFileInfo>();
    static TreeMap<String, TdfFileInfo> mIncludePathMap = new TreeMap<String, TdfFileInfo>(Collections.reverseOrder());
    static String mProtoRootDir = new String();

    static void addIncludePath(String inputFilename) throws Exception
    {
        if (inputFilename != null)
        {
            String[] items = inputFilename.split("!");
        
            String file = null;
            String packageName = null;
            if (items.length == 1)
            {
                file = items[0];
            }
            else if (items.length == 2)
            {
                packageName = items[0];
                file = items[1];
            }
            else
            {
                throw new Exception("Invalid format for specified include path");
            }
            
            File f = new File(file);
            if (f.isFile())
                f = f.getParentFile();
        
            while (f != null)
            {
                String path = f.getCanonicalPath();
                if (mIncludePathMap.containsKey(path) || isStrictBaseInludePathPrefix(path))
                {
                    // if include path already known or of it backs up beyond the base include path(s) we bail
                    break;
                }
                mIncludePathMap.put(path, new TdfFileInfo(path, packageName));
                f = f.getParentFile();
            }
        }
    }

    static boolean isStrictBaseInludePathPrefix(String path)
    {
        // exact matches are not prefixes
        if (mBaseIncludePaths.contains(path))
            return false;
        for (String basePath : mBaseIncludePaths)
        {
            if (basePath.startsWith(path))
            {
                debugPrintln("skipping path: " + path + ", which is a strict prefix of include path: " + basePath);
                return true;
            }
        }
        return false;
    }
    
    static TdfFileInfo findFile(String inputFilename) throws IOException
    {
        TdfFileInfo found = null;
        
        boolean printDebug = false;
        
        //Go through each path and try the filename out.
        for (Map.Entry<String, TdfFileInfo> e : mIncludePathMap.entrySet())
        {
            File f = new File(e.getValue().path, inputFilename);
            if (f.exists())
            {
                String path = f.getCanonicalPath().replace('\\', '/');
                found = new TdfFileInfo(path, e.getValue().packageName);
                if (mCurrentFileSubSet.add(path))
                {
                    if (printDebug)
                    {
                        System.out.println("New File: " + path + " successfully matched pattern: " 
                            + inputFilename + " --->");
                    }
                    return found;
                }
                if (printDebug)
                {
                    System.out.println("File: " + path + " matching pattern: " 
                        + inputFilename + " has already been included. Skipping circular include reference.");
                }
            }
            else if (printDebug)
            {
                System.out.println("Dir: " + e.getValue().path + " does not contain a file matching pattern: " 
                    + inputFilename + ".");
            }
        }
        
        if (found != null)
        {
            if (printDebug)
            {
                System.out.println("File: " + found.path + " matching pattern: " 
                        + inputFilename + " has already been included.");
            }
        }
        else
        {
            File file = new File(inputFilename);
            if (file.exists())
            {
                String path = file.getCanonicalPath().replace('\\', '/');
                mCurrentFileSubSet.add(path);
                return new TdfFileInfo(path, null);
            }
        }

        return found;
    }
    
    static String getRelativePath(String path)
    {
        return path.replace(mCurrentWorkingDir, "");
    }
    
    static String getRelativeProtoPath(String fullPath, String relativeTdfPath)
    {
        
        // Check if the tdf belongs to blazeserver. If yes, add the mProtoRootDir for blazeserver tdfs.
        // Otherwise, simply use the relative tdf path (-/gen/ folder convention) to get a proto. 
        String[] includeDirs = {"/blazeserver/customcode/", "/blazeserver/customcomponent/", "/blazeserver/proxycomponent/", "/blazeserver/component/", "/blazeserver/framework/"};

        for (String includeDir : includeDirs) 
        {           
            if (fullPath.lastIndexOf(includeDir) != -1)
            {
                String newPath = fullPath.substring(fullPath.lastIndexOf(includeDir)+1, fullPath.length()).replace("\\", "/");
                newPath = newPath.replace("blazeserver/","").replace("/gen/", "/");
                if (mProtoRootDir.length() != 0)
                    newPath = mProtoRootDir + "/" + newPath;
                return newPath;
            }  
        }
       
       return relativeTdfPath.replace("/gen/", "/");
    }

    static HashSet<String> mVisitedFileSet = new HashSet<String>();
    static HashSet<String> mCurrentFileSubSet = new HashSet<String>();
    static int mNumSpaces = 0;
    public static TdfParseOutput parseTdfFile(String inputFilename, String contents, boolean topLevel) throws Exception
    {
        CharStream stream;
        String resolvedFilename = inputFilename;
        TdfParseOutput result = new TdfParseOutput();
        
        if (contents == null)
        {
            TdfFileInfo tdfInfo = findFile(inputFilename);
            if (tdfInfo == null)
            {
                throw new FileNotFoundException("Cannot find file: " + inputFilename + 
                        " in following search paths: " + mIncludePathMap.keySet());
            }

            resolvedFilename = tdfInfo.path;
            result.packageName = tdfInfo.packageName;
            result.resolvedFileName = resolvedFilename; // The full file path on the system
            result.relativeTdfPath = inputFilename; // as specified in tdf   
            result.relativeProtoPath = getRelativeProtoPath(resolvedFilename, inputFilename);

            String prefix = "";
            if (mDebug)
                prefix = mNumSpaces + new String(new char[mNumSpaces]).replace("\0", " ");
            
            //If we've already visited this file, just bail on it.  No reason to see it again.
            // It seems that we end up visiting toplevel files more than once by parsers like TdfTypeFixup, bleh!
            if (!topLevel && mVisitedFileSet.contains(resolvedFilename))
            {
                debugPrintln(prefix + "Early out on file: " + tdfInfo.path + " matching pattern: " 
                            + inputFilename + " has already been included.");
                return result;
            }
            
            debugPrintln(prefix + "Visit file: " + tdfInfo.path + " matching pattern: " 
                        + inputFilename);
            
            mVisitedFileSet.add(resolvedFilename);
            
            // this might add a new search path
            addIncludePath(resolvedFilename);
            stream = new ANTLRFileStream(resolvedFilename);
        }
        else
        {
            stream = new ANTLRStringStream(contents);
        }
        
        ++mNumSpaces;
        
        TdfGrammarLexer lexer = new TdfGrammarLexer(stream);
        result.tokens = new CommonTokenStream(lexer);
        TdfGrammarParser parser = new TdfGrammarParser(result.tokens);
        parser.setTreeAdaptor(new HashTreeAdaptor());
        
        // This:
        //         String relativeFilePath = new File(inputFilename).getPath().replace('\\', '/');
        // 'almost' works(for blaze and arson source), but it doesn't for HTML generating a href links... bleh!
        // 
        // We have to build the output file directory path based on the relationship between CWD and the input files!
        // this is stupid as it limits us to having a single root dir, and also requires that we run typecomp in a parent dir
        // of the INPUT files even though we support processing absolute file paths, and should allow files to be pulled from ANYWHERE!
        // The proper solution would be to just put a special .tdfanchor file in each 'root' directory thereby telling the tool
        // how to compute a relative path given any particular TDF file.
        // Instead we are currently using this crappy mechanism...
        String relativeFilePath = getRelativePath(resolvedFilename);
        
        // For some stupid reason both result and parser.file() take same file arguments!
        result.tree = (CommonTree) parser.file(relativeFilePath,  
                topLevel, mIsClient, mInputParamMap).getTree(); 

        --mNumSpaces;
        
        return result;
    }
    
    public static String getRelativeOutputDir(File outputFile)
    {
        String parentDir = outputFile.getParent();
        if (parentDir != null)
        {
            // Remove any inner 'gen' folders.
            // (Do not remove the top folder, even if it is named 'gen'.)
            String result =  (parentDir + File.separator).replace(File.separator + "gen" + File.separator, File.separator);
            return result.substring(0, result.length()-1).replace("\\", "/");
        }

        return "";
    }

    static TdfSymbolTable allSymbols = new TdfSymbolTable();
    static TreeMap<String, HashToken> includeFileNodes = new TreeMap<String, HashToken>();
    static TreeMap<String, String> mTemplateBatchMap = new TreeMap<String, String>();
    static TreeMap<String, StringTemplateGroup> mTemplateGroupMap = new TreeMap<String, StringTemplateGroup>();
    // NOTE: technically inputFilename should be null if contents != null(and vice versa), but currently we seem to be using both sometimes.
    // One case where this is true is when genning "central" files. We end up using the inputFileName as a sort of half-assed hint to where to write the output...
    public static void generateTdfFile(String inputFilename, String contents, ExtHashtable templateOutputPairs) throws Exception
    {
        //Set some input file dependent variables
        File inputFile = new File(inputFilename);
        String fileBase = inputFile.getName().replaceFirst("[.][^.]+$", "");
        mInputParamMap.put("FileBase", fileBase);
        
        String relativeOutputDir = inputFile.isAbsolute() ? "" : getRelativeOutputDir(inputFile);
        mInputParamMap.put("RelativeDirNoGen", relativeOutputDir);
        
    
        //Parse the input file 
        TdfParseOutput parseResult = parseTdfFile(inputFilename, contents, true);    
        BufferedTreeNodeStream nodes = new BufferedTreeNodeStream(parseResult.tree);
        nodes.setTokenStream(parseResult.tokens);
     
        //Resolve all our nodes 
        TdfTypeFixup typeFixup = new TdfTypeFixup(nodes);
        typeFixup.setTreeAdaptor(new HashTreeAdaptor());         
        BufferedTreeNodeStream fixedupNodes = new BufferedTreeNodeStream((CommonTree)typeFixup.file(allSymbols).getTree());
        fixedupNodes.setTokenStream(parseResult.tokens);            
        
        //Resolve all our nodes 
        TDFTypeResolver typeResolver = new TDFTypeResolver(fixedupNodes);
        typeResolver.setTreeAdaptor(new HashTreeAdaptor());         
        BufferedTreeNodeStream typeResolvedNodes = new BufferedTreeNodeStream((CommonTree)typeResolver.file(allSymbols, includeFileNodes, mIsClient).getTree());
        typeResolvedNodes.setTokenStream(parseResult.tokens);      
        
        //2nd pass resolution resolve all our nodes
        TDFRelationshipResolver relationshipResolver = new TDFRelationshipResolver(typeResolvedNodes);
        relationshipResolver.setTreeAdaptor(new HashTreeAdaptor()); 
        BufferedTreeNodeStream relationshipResolvedNodes = new BufferedTreeNodeStream((CommonTree)relationshipResolver.file(allSymbols).getTree());
        relationshipResolvedNodes.setTokenStream(parseResult.tokens);
   
        StringBuffer templateArgs = new StringBuffer(512);
        templateArgs.append(" ");
        //Now walk the final output tree for each template, and generate an output file if necessary.
        TDFtg walker = new TDFtg(relationshipResolvedNodes);       
        for (Map.Entry<Object, Object> templatePair : templateOutputPairs.entrySet())
        {
            String templateName = (String) templatePair.getKey();
            String outputFilePattern = (String) templatePair.getValue();
            templateArgs.append("t/p: " + templateName + " => " + outputFilePattern + " ");
            try
            {
                StringTemplateGroup group = mTemplateGroupMap.get(templateName);
                if (group == null)
                {
                    group = StringTemplateGroup.loadGroup(templateName, mStringTemplateLexerClass, null);
                    mTemplateGroupMap.put(templateName, group);
                    group.registerRenderer(String.class, new SimpleFormatRenderer());
                    group.registerRenderer(Integer.class, new SimpleFormatRenderer());
                    group.registerRenderer(Long.class, new SimpleFormatRenderer());
                    group.registerRenderer(Byte.class, new SimpleFormatRenderer());
                    group.registerRenderer(Short.class, new SimpleFormatRenderer());
                    group.registerRenderer(Double.class, new SimpleFormatRenderer());
                    group.registerRenderer(Float.class, new SimpleFormatRenderer());
                    group.registerRenderer(Boolean.class, new SimpleFormatRenderer());
                    group.registerRenderer(Character.class, new SimpleFormatRenderer());
                    group.registerRenderer(Object.class, new SimpleFormatRenderer());
                }
                walker.setTemplateLib(group);
            }
            catch (Exception e)
            {
                throw e;
            }

            ExtHashtable templateTable = new ExtHashtable();

            try
            {
                boolean processMasters = !mIsClient;
                
                if (!mIsClient) // See if we are explicitly disallowing master processing on server
                {
                    if (mInputParamMap.get("processMasters") != null && mInputParamMap.get("processMasters").equals("false"))
                    {
                        processMasters = false;
                    }
                }
                walker.file(templateTable, processMasters, outputFilePattern, mInputParamMap);
            }
            catch(Exception e)
            {
                System.err.println("Exception thrown while generating TDF using template " + templateName);
                throw e;
            }

            Set<String> fileOutputSet = new TreeSet<String>();
            //We now have a mapping of file structures to templates.  Dump those templates to a file
            for (Map.Entry<Object, Object> e : templateTable.entrySet())
            {
                String filename = (String) e.getKey();
                StringTemplate st = (StringTemplate) e.getValue();
                
                if(!mGenCaseSensitiveFilenames)
                {
                    filename = filename.toLowerCase();
                }

                String absFilename = new File(mOutputDir, filename).toString();

                if (fileOutputSet.contains(absFilename))
                {
                    // some template sets require the same prereq files to be genned, if they already have been, skip genned files.
                    continue;
                }
                try
                {
                    if (generateFile(absFilename, filename, relativeOutputDir, st, parseResult))
                    {
                        // we only add to the set when a real file has been output (empty files don't count)
                        fileOutputSet.add(absFilename);
                    }
                }
                catch(Exception genFileE)
                {
                    System.err.println("Exception thrown while generating TDF using template " + templateName);
                    throw genFileE;
                }
            }
            
            walker.reset();
        }
        String targs = templateArgs.toString();
        if (!mTemplateBatchMap.containsKey(targs))
            mTemplateBatchMap.put(targs, inputFile.getName());
    }
    
    static ArrayList<String> mFileOutputList = new ArrayList<String>(); // added for debugging
    @SuppressWarnings("unchecked")
    static boolean generateFile(String outputFilename, String relativeFilename, String relativeOutputPath, StringTemplate fileTemplate, TdfParseOutput parseResult) throws Exception
    {
        //Create the output path if it doesn't exist.
        File outFile = new File(outputFilename);
        File parentDir = outFile.getParentFile();
        if (!parentDir.exists())
            parentDir.mkdirs();
        
        String contents = "";
        if (fileTemplate != null)
        {
            Map<String, ExtHashtable> attrMap = fileTemplate.getAttributes();
            if (attrMap != null)
            {
                ExtHashtable fileAttr = attrMap.get("fileNode");
                if (fileAttr != null)
                {
                    fileAttr.put("OutputFile", outFile.getPath());
                    fileAttr.put("PackageName", parseResult.packageName);
                    fileAttr.put("ResolvedFilename", parseResult.resolvedFileName);
                    fileAttr.put("RelativeOutputPath", relativeOutputPath);           
                    fileAttr.put("RelativeOutputFilename", relativeFilename);
                    // for proto conversion, distinguish between regular and root component files. Hacked out name here, but there's no other potential 'root' files ever anyhow though for this.
                    if (outputFilename.toLowerCase().endsWith("blazerpccomponent_grpcservice.proto"))
                    {
                        fileAttr.put("IsRootComponentProto", true);
                    }
                }

                if (relativeOutputPath != null && !relativeOutputPath.isEmpty())
                {
                    ExtHashtable compAttr = attrMap.get("component");
                    if (compAttr != null)
                    {
                        compAttr.put("RelativeOutputPath", relativeOutputPath);
                        if (mProtoRootDir.length() != 0)
                            relativeOutputPath = mProtoRootDir + "/" +relativeOutputPath;
                        compAttr.put("RelativeOutputProtoPath", relativeOutputPath); // Used in service generation code
                    }
                }
            }
            contents = fileTemplate.toString(100);//the number arg allows the 'wrap' option to work if its specified in the .g or .stg files
        }
        
        if (!contents.isEmpty())
        {
            FileWriter fw = null;
            try
            {
                //Use FileWriter(..., false) ensures we always create a new file
                fw = new FileWriter(outFile, false);
                fw.write(contents);
                mFileOutputList.add(outFile.getPath());
            }
            finally
            {
                if (fw != null)
                    fw.close();
            }
            return true;
        }
        
        return false;
    }
    
    static long getLastCreatedOrModified(File f) throws Exception
    {
        if (!f.exists())
            return 0;

        BasicFileAttributes basicAttributes = Files.readAttributes(f.toPath(), BasicFileAttributes.class);
        long creationTime = basicAttributes.creationTime().toMillis();
        long modificationTime = basicAttributes.lastModifiedTime().toMillis();
        
        return (modificationTime > creationTime) ? modificationTime : creationTime;
    }

    static boolean shouldWriteFile(String filename) throws Exception
    {
        if (mDependencyFileLastModified == 0)
            return true;
    
        File writeFile = new File(filename);
        long lastModified = getLastCreatedOrModified(writeFile);
        return (lastModified == 0 || lastModified > mDependencyFileLastModified);
    }
    
    static boolean mDebug = false;
    static boolean mIsClient = false;
    static boolean mCombine = false; // whether to process inputs in a combined file
    static boolean mGenCaseSensitiveFilenames = false;
    static String mDependencyFile = null;
    static long mDependencyFileLastModified = 0;
    static String mOutputDir; 
    static List<String> mTemplateDirs = new ArrayList<String>();
    static ExtHashtable mInputParamMap = new ExtHashtable();
    static ExtHashtable mTemplatePairs = new ExtHashtable();
    static Class<?> mStringTemplateLexerClass = org.antlr.stringtemplate.language.AngleBracketTemplateLexer.class;    
        
    private static void debugPrintln(String str)
    {
        if (mDebug)
            System.out.println(str);
    }
    
    // TODO: store input and output files in a map keyed by their pathnames
    // then after they are read, written, processed store their number of accesses
    // and the time it took to process each. We can output it like this:
    // 1: filepath = reads(4) sec(5)
    // 8: filepath = writes(1) sec(8)
    private static int printDebugHeader(String[] args)
    {
        debugPrintln("TypeComp dir = " + mCurrentWorkingDir);
        debugPrintln("TypeComp inputs = [");
        int inputFileCount = 0; // TODO: this is not quite accurate because it counts bla in -sdk "bla" as another input file...
        for (int i = 0; i < args.length; ++i)
        {
            if (args[i].charAt(0) == '-')
            {
                  debugPrintln(String.format("+%3d = ", i) + args[i]);
                inputFileCount = 0;
            }
            else
            {
                debugPrintln(String.format(" %3d = ", i) + args[i]);
                ++inputFileCount;
            }
        }
        debugPrintln("]");
        return inputFileCount;
    }
    
    private static void printDebugFooter(long startTime, int inputFileCount)
    {
            
        if (mDebug)
        {
            System.out.println("TypeComp templates = [");
            int i = 0;
            for (Map.Entry<String, String> e : mTemplateBatchMap.entrySet())
            {
                System.out.println(String.format(" %3d -> ", i++) + e.getKey() + " from: " + e.getValue());
            }
            System.out.println("]");

            System.out.println("TypeComp outputs = [");
            i = 0;
            for (String output : mFileOutputList)
            {
                System.out.println(String.format(" %3d = ", i++) + output);
            }
            System.out.println("]");
        }
        
        long endTime = System.currentTimeMillis();
        debugPrintln("TypeComp Generated: " + inputFileCount + " inputs"
            + " > " + mFileOutputList.size() + " outputs in " 
            + (endTime - startTime) / 1000.0 + " seconds.");
    }

    private static File[] getTemplateFiles(String dirName) throws Exception
    {
        File dir = new File(dirName);
        return dir.listFiles(new FilenameFilter() { 
            public boolean accept(File dir, String filename) { return filename.endsWith(".stg"); }
        });
    }
    
    static String mCurrentWorkingDir;
    public static void main(String[] args) throws Exception 
    {
        // remember the current working directory, needed for computing relative pathnames later
        mCurrentWorkingDir = (new File(".")).getAbsolutePath().replace('\\','/');
        mCurrentWorkingDir = mCurrentWorkingDir.substring(0, mCurrentWorkingDir.length()-1);
        
        long startTime = System.currentTimeMillis();
        
        int inputFileCount = printDebugHeader(args);
        
        int startFileIndex = parseArgs(args);
        
        if (startFileIndex >= 0)
        {
            if (mDebug)
            {
                for (String basePath : mBaseIncludePaths)
                {
                      System.out.println("base path: " + basePath);
                }
            }
            
            if (mOutputDir == null)
                mOutputDir = ".";
    
            //cache off the last modified date
            if (mDependencyFile != null)  
            {                           
                File timeStampFile = new File(mDependencyFile);
                if (!timeStampFile.isAbsolute())
                    timeStampFile = new File(mOutputDir, mDependencyFile);
                mDependencyFileLastModified = getLastCreatedOrModified(timeStampFile);  //returns 0 if the file does not exist.
            }
    
            //Force if this jar file is newer
            File jarFile = new File(TdfComp.class.getProtectionDomain().getCodeSource().getLocation().getPath());
            boolean forceGeneration = getLastCreatedOrModified(jarFile) > mDependencyFileLastModified;
            if (forceGeneration)
            {
                debugPrintln("TypeComp jar = " + jarFile + " is newer than --dependency, forcing generation.");
            }
            else
            {
                //Force generation b/c of a template
                for (String templateDir : mTemplateDirs)
                {
                    File[] templateFiles = getTemplateFiles(templateDir);
                    if (templateFiles == null)
                        continue;
                    for (File tFile : templateFiles)
                    {
                        long timestamp = getLastCreatedOrModified(tFile);
                        if (timestamp > mDependencyFileLastModified)
                        {
                            debugPrintln("TypeComp template = " + tFile + " is newer than --dependency, forcing generation.");
                            forceGeneration = true;
                            break;
                        }
                    }
                    if (forceGeneration)
                        break;
                }
            }
                        
            debugPrintln("TypeComp template dirs: " + mTemplateDirs);
            
            StringTemplateGroup.registerGroupLoader(new PathGroupLoaderMulti(mTemplateDirs));
            
            //If combined file generation is enabled, we need to join all the input files into a single massive blob
            if (mCombine)
            {
                /*
                TODO: currently we use buffered line reading and writing to stringbuffer to read the contents of all the files into mem.
                This is slow for the following reasons:
                1. We parse all the files twice (once above, when processing them individually, and once to do this join)
                Solution: Since we are already going to be reading the whole file into memory above anyway, we can change 
                the for loop above to ensure that we always process each file as an in-memory string, and upon processing it,
                we can append it to a growing 'blob' of data if mCombine == true.
                2. We use buffered line reading and copying of line by line. 
                Solution: Instead we could just join the files in memory or on disk:
                http://stackoverflow.com/questions/6065556/what-method-is-more-efficient-for-concatenating-large-files-in-java-using-filech
                */
                mIncludePathMap.clear();
                for (String i : mBaseIncludePaths)
                {
                    addIncludePath(i);
                }
                
                boolean isModified = false;
                StringBuilder allFileContents = new StringBuilder();
                for (int index = startFileIndex; index < args.length; ++index)
                {
                    isModified = isModified || shouldWriteFile(args[index]);
                
                    // must add include paths because TDFs search for other tdfs to include based on their paths
                    addIncludePath(args[index]);
                    FileReader fr = new FileReader(args[index]);
                    BufferedReader br = new BufferedReader(fr);
                    String line = br.readLine();
                        
                    allFileContents.append("group \"");
                    allFileContents.append(args[index]); // groups are named with full relative input path
                    allFileContents.append("\"{");
                    while (line != null)
                    {
                        allFileContents.append(line);
                        allFileContents.append("\n");
                        line = br.readLine();
                    }
                    allFileContents.append("}\n");
                    br.close();
                }
                
                if (forceGeneration || isModified)
                    generateTdfFile(args[startFileIndex], allFileContents.toString(), mTemplatePairs);
            }
            else
            {
                if (mDebug)
                {
                    // print include paths
                    for (String i : mBaseIncludePaths)
                    {
                        addIncludePath(i);
                    }
                    for (Map.Entry<String, TdfFileInfo> e : mIncludePathMap.entrySet())
                    {
                        System.out.println("include path: " + e.getValue().path);
                    }
                    mIncludePathMap.clear();
                }
        
                for (int index = startFileIndex; index < args.length; ++index)
                {
                    try
                    {
                        mIncludePathMap.clear();
                        for (String i : mBaseIncludePaths)
                        {
                            addIncludePath(i);
                        }
                        addIncludePath(args[index]);
        
                        if (mTemplatePairs.size() > 0)
                        {
                            if (forceGeneration || shouldWriteFile(args[index]))
                                generateTdfFile(args[index], null, mTemplatePairs);
                        }
                    }
                    catch (Exception e)
                    {
                        System.err.println("Error processing input file " + args[index]);
                        throw e;
                    }
                }
            }
        }
        
        if (mDependencyFile != null)
        {
            if (mFileOutputList.isEmpty())
            {
                debugPrintln("No output files generated, skip creating: " + mDependencyFile);
            }
            else
            {
                File timeStampFile = new File(mDependencyFile);
                if (!timeStampFile.isAbsolute())
                    timeStampFile = new File(mOutputDir, mDependencyFile);
                File parentDir = timeStampFile.getParentFile();
                if (!parentDir.exists())
                    parentDir.mkdirs();
                FileWriter fileWriter = null;
                try 
                {
                    fileWriter = new FileWriter(timeStampFile, false);
                }
                catch (Exception e) {}
                finally
                {
                    if (fileWriter != null)
                        fileWriter.close();
                }
            }
        }
            
        printDebugFooter(startTime, inputFileCount);
    }
    
    public static int parseArgs(String[] args)
    {
        int index = 0;
        for (; index < args.length; ++index)
        {
            if (args[index].equals("--outputDir") || args[index].equals("-s") || args[index].equals("-oc"))
            {
                index++;
                if (index == args.length)
                    return -1;
                mOutputDir = args[index];
            }
            else if (args[index].equals("--dependency") || args[index].equals("-d"))
            {
                index++;
                if (index == args.length)
                    return -1; 
                mDependencyFile = args[index];
            }
            else if (args[index].equals("--client") || args[index].equals("-client"))
            {
                mIsClient = true;
            }
            else if (args[index].equals("--templateDir") || args[index].equals("-t"))
            {
                index++;
                mTemplateDirs.add(args[index]);
            }
            else if (args[index].equals("--includeDir") || args[index].equals("-I") || args[index].equals("-sdk"))
            {
                index++;
                mBaseIncludePaths.add(args[index]);
            }
            else if (args[index].equals("--combine"))
            {
                mCombine = true;
            }
            else if (args[index].startsWith("-D:"))
            {
                String argData = args[index].substring("-D:".length());
                String[] pair = argData.split("=", 2);
                if (pair.length == 1)
                {
                    mInputParamMap.put(pair[0], "");
                }
                else if (pair.length == 2)
                {
                    mInputParamMap.put(pair[0], pair[1]);
                }   
                
                if (mInputParamMap.get("protoRootDir") != null)
                  mProtoRootDir = mInputParamMap.get("protoRootDir").toString();        
            }            
            else if (args[index].startsWith("--template"))
            {
                index++;
                
                String[] pair = args[index].split("=", 2);
                if (pair.length == 2)
                {                    
                    mTemplatePairs.put(pair[0], pair[1]);
                }
            }
            else if (args[index].equals("--useAltStringTemplateLexer"))
            {
                mStringTemplateLexerClass = org.antlr.stringtemplate.language.DefaultTemplateLexer.class;
            }
            else if (args[index].equals("--genCaseSensitiveFilenames"))
            {
                mGenCaseSensitiveFilenames = true;
            }
            else if (args[index].charAt(0) == '-')
            {
                return -1;
            }
            else
            {
                return index;
            }
        }
    
        return -1;
    }

}
