import org.antlr.runtime.*;
import org.antlr.runtime.tree.*;
import org.antlr.stringtemplate.*;

import java.io.*;
import java.util.*;
import java.util.Map.Entry;
import java.lang.*;

public class TdfComp 
{
    class FileGroup 
    {
        public FileGroup(String name) { Name = name; }
        public String Name;
        public ArrayList<String> FileList = new ArrayList<String>();
    }
    
    static TreeMap<String, FileGroup> mFileGroups = new TreeMap<String, FileGroup>();
    
    static void addFileToGroup(String groupName, String fileName)
    {
        if (!mFileGroups.containsKey(groupName))
            mFileGroups.put(groupName, new TdfComp().new FileGroup(groupName));
        mFileGroups.get(groupName).FileList.add(fileName.replace('\\', '/'));
    }
    
    static ArrayList<String> mBaseIncludePaths = new ArrayList<String>();
    static ArrayList<String> mLuaOutputFileNames = new ArrayList<String>();
    static ArrayList<TdfFileInfo> mIncludePaths = new ArrayList<TdfFileInfo>();
    
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
            if (f.isDirectory())
            {
                mIncludePaths.add(new TdfFileInfo(f.getPath(), packageName));
            }
    
            while (f.getParent() != null)
            {
                f = f.getParentFile();
                mIncludePaths.add(new TdfFileInfo(f.getPath(), packageName));
            }
        }            
    }
        
    static String getRelativePath(File file)
    {
        if(!file.isAbsolute())
        {
            return file.getPath();
        }
        else
        {
            String absolutePath = (new File(".")).getAbsolutePath();
            absolutePath = absolutePath.substring(0, absolutePath.length()-1);
            return file.getPath().replace(absolutePath, "");
        }
    }
    
    static TdfFileInfo findFile(String inputFilename) throws FileNotFoundException
    {
        //early out for the fine
        if (new File(inputFilename).exists())
            return new TdfFileInfo(inputFilename, null);
        
        //Go through each path and try the filename out.
        for (TdfFileInfo info : mIncludePaths)
        {
            File f = new File(info.path, inputFilename);
            if (f.exists())
            {
                // See if we can get a relative dir
                return new TdfFileInfo(getRelativePath(f).replace('\\', '/'), info.packageName);
            }
        }
        throw new FileNotFoundException("Cannot find file " + inputFilename);  
    }
    
    static TreeSet<String> mVisitedFileSet = new TreeSet<String>();
    public static TdfParseOutput parseTdfFile(String inputFilename, String contents, boolean topLevel) throws Exception
    {
        CharStream stream;
        String resolvedFilename = inputFilename;
        TdfParseOutput result = new TdfParseOutput();
        
        if (contents == null)
        {
            TdfFileInfo tdfInfo = findFile(inputFilename);
            resolvedFilename = tdfInfo.path;
            result.packageName = tdfInfo.packageName;
            result.resolvedFileName = resolvedFilename;
            
            //If we've already visited this file, just bail on it.  No reason to see it again.
            if (!topLevel && mVisitedFileSet.contains(resolvedFilename))
            {          
                return result;
            }

            mVisitedFileSet.add(resolvedFilename);
            
            addIncludePath(resolvedFilename);
            stream = new ANTLRFileStream(resolvedFilename);
        }
        else
        {
            stream = new ANTLRStringStream(contents);
        }
        
        TdfGrammarLexer lexer = new TdfGrammarLexer(stream);
        result.tokens = new CommonTokenStream(lexer);
        TdfGrammarParser parser = new TdfGrammarParser(result.tokens);
        parser.setTreeAdaptor(new HashTreeAdaptor());
        result.tree = (CommonTree) parser.file(resolvedFilename, mTemplateConfig.buildOutputFileBase(resolvedFilename), topLevel, mIsClient, mHeaderPrefix).getTree();

        
        
        return result;
    }
    
    private static boolean containsSourceElements(CommonTree tree)
    {
        if (tree.token instanceof TdfSymbol)
        {
            TdfSymbol symbol = (TdfSymbol)tree.token;
            if (symbol.isActualCategoryStruct()
                || symbol.isActualCategoryEnum()
                || symbol.isActualCategoryMajorEntity())
                return true;
        }
        
        if (tree.getChildren() != null)
        {
            for (Object t : tree.getChildren())
            {
                if (containsSourceElements((CommonTree)t))
                    return true;
            }
        }
        
        return false;
    }

    static TdfSymbolTable allSymbols = new TdfSymbolTable();
    static TreeMap<String, HashToken> includeFileNodes = new TreeMap<String, HashToken>();
    public static void generateTdfFile(String inputFilename, String contents, String[][] templateList, boolean modOutDir, boolean centralFile) throws Exception
    {
    
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
        BufferedTreeNodeStream relationshipResolvedNodes = new BufferedTreeNodeStream((CommonTree)relationshipResolver.file(allSymbols, centralFile).getTree());
        relationshipResolvedNodes.setTokenStream(parseResult.tokens);
        
        //Fixup the output filenames.
        String sourceOutputDir = mSourceOutputDir;
        String headerOutputDir = mHeaderOutputDir;
        File inputFile = new File(inputFilename);
        String relativeOutputDir = "";
        if (!inputFile.isAbsolute() && inputFile.getParent() != null)
        {
            relativeOutputDir = inputFile.getParent().toString();
            if (relativeOutputDir.endsWith( File.separator + "gen" ))
                relativeOutputDir = relativeOutputDir.substring(0, relativeOutputDir.lastIndexOf(File.separator + "gen"));
            relativeOutputDir = relativeOutputDir.replace(File.separator + "gen" + File.separator, "").replace("gen" + File.separator, "");
        }
        if (modOutDir)
        {
            sourceOutputDir = new File(sourceOutputDir, relativeOutputDir).toString();
            headerOutputDir = new File(headerOutputDir, relativeOutputDir).toString();
        }
        
        //Now walk the final output tree for each template, and generate an output file if necessary.
        TDFtg walker = new TDFtg(relationshipResolvedNodes);       
        for (int counter = 0; counter < templateList.length; ++counter)
        {
            String templateName = templateList[counter][TEMPLATE_INDEX];
            String outputFilePattern = templateList[counter][FILENAME_INDEX];
            String groupPattern = templateList[counter][GROUPNAME_INDEX];
            boolean doHeader = templateList[counter].length > HEADER_INDEX ? false : true;          
            String guardPattern = templateList[counter].length > GUARD_INDEX ? templateList[counter][GUARD_INDEX] : null;
   
            doHeader = !mTemplateConfig.useSimpleOutput(templateName);
   
            StringTemplateGroup group = StringTemplateGroup.loadGroup(templateName, mTemplateConfig.getStringTemplateLexerClass(), null);
            group.registerRenderer(String.class, new SimpleFormatRenderer());
            walker.setTemplateLib(group);

            ExtHashtable templateTable = new ExtHashtable();

            try
            {
                walker.file(templateTable, mIsClient, mNoCentral);
            }
            catch(Exception e)
            {
                System.err.println("Exception thrown while generating TDF using template " + templateName);
                throw e;
            }

            int maxChunkCountWritten = 0;
            TreeSet<String> fileOutputSet = new TreeSet<String>();
            //We now have a mapping of file structures to templates.  Dump those templates to a file
            for (Map.Entry<Object, Object> e : templateTable.entrySet())
            {
                StringTemplate st = (StringTemplate) e.getValue();

                if (st != null && st.getChunks() != null && st.getChunks().size() > 0)
                {
                    List<?> list = (List<?>) e.getKey();
                    String filename = String.format(outputFilePattern, list.toArray());

                    if(!mTemplateConfig.hasCaseSensitiveFilenames())
                    {
                        filename = filename.toLowerCase();
                    }

                    if ( mIsLuaGen && mGenLanguage.equalsIgnoreCase("html") )
                    {
                        if (!relativeOutputDir.isEmpty())
                            mLuaOutputFileNames.add(relativeOutputDir.replace('\\', '/') + "/" + filename.replace('\\', '/'));
                        else
                            mLuaOutputFileNames.add(filename.replace('\\', '/'));
                    }
                    
                    filename = new File(filename.endsWith(".h") ? headerOutputDir : sourceOutputDir, filename).toString();
                    String guardName = (guardPattern != null) ? String.format(guardPattern, list.toArray()) : null;
                    String groupName = (groupPattern != null) ? String.format(groupPattern, list.toArray()) : null;

                    if (groupName != null)
                    {
                        addFileToGroup(groupName, filename);
                    }
                    
                    boolean skipEmpty = false;

                    if (mTemplateConfig.skipEmptyGeneration(filename) && !containsSourceElements(parseResult.tree))
                    {
                        skipEmpty = true;
                    }
                    
                    if (!skipEmpty)
                    {
                        try
                        {             
                            //Because of the "template templates" where we wrap a headerfile with some base header file stuff, it might 
                            //be possible that an empty file has already been set as the filename for this header (this occurs when 
                            //multiple sections of a template collapse to a single file - such as the "base component file".  So what we do 
                            //here is see if we've already written something, and pick the bigger one.  There should be no cae where 
                            //we're doing two legitimate templates for the same entry, so this should be sufficient.
                            if (!fileOutputSet.contains(filename) || st.getChunks().size() > maxChunkCountWritten)
                            {
                                generateFile(inputFilename, filename, relativeOutputDir, st, doHeader, guardName);
                                maxChunkCountWritten = st.getChunks().size();
                                fileOutputSet.add(filename);
                            }
                        }
                        catch(Exception genFileE)
                        {
                            System.err.println("Exception thrown while generating TDF using template " + templateName);
                            throw genFileE;
                        }
                    }
                }
            }
            
            walker.reset();
        }
    }
    
    static void generateFile(String inputFilename, String outputFilename, String relativeOutputPath,
            StringTemplate contents, boolean doHeader, String guardName) throws Exception
    {
        //Create the output path if it doesn't exist.
        File outFile = new File(outputFilename);
        outFile.getParentFile().mkdirs();
        
        //Get a copy of the wrapping file output template and set the variables
        StringTemplate fileTemplate = null;
        
        if (doHeader)
        {
            fileTemplate = mCommonGroup.getInstanceOf("fileOutput");
            fileTemplate.setAttribute("contents", contents);
            fileTemplate.setAttribute("defTable", new DefineHelper());
            fileTemplate.setAttribute("inputFilename", inputFilename.replace('\\', '/'));
            fileTemplate.setAttribute("outputFilename", outputFilename.replace('\\', '/').replace("/gen2/", "/gen/"));
            fileTemplate.setAttribute("relativeOutputPath", relativeOutputPath.replace('\\', '/'));
            // HACK: replace("gen/component/", "") is just to quickly workaround incorrect include guards for TDF component files.
            inputFilename = inputFilename.replace("gen/component/", "");
            String guardName2;
            if (guardName != null)
            {
                guardName2 = guardName;
            }
            else
            {
                guardName2 = inputFilename.substring(inputFilename.lastIndexOf(':') + 1, inputFilename.length());
                guardName2 = guardName2.substring(0, guardName2.lastIndexOf('.')) + ".h";
            }
            fileTemplate.setAttribute("guardFilename", guardName2);
            fileTemplate.setAttribute("isHeader", outputFilename.endsWith(".h"));
        }
        else
        {
            fileTemplate = contents;
        }
        
        Writer fw = new FileWriter(outFile);
        if (fileTemplate != null)
            fw.write(fileTemplate.toString());
        fw.close();
    }

    static void generateTimeStamp(String timeStampName) throws Exception
    {
        if (mTemplateConfig.generateDepFile())
        {
            StringTemplateGroup depGroup = StringTemplateGroup.loadGroup("timestamp", mTemplateConfig.getStringTemplateLexerClass(), null);
            if (depGroup != null)
            {
                StringTemplate depTemp = depGroup.getInstanceOf("fileGroupList");
                depTemp.setAttribute("groupList", mFileGroups.values());
                generateFile(null, new File(mSourceOutputDir, timeStampName).toString(), "", depTemp, false, null);
            }
        }
    }

    static StringTemplateGroupLoader mLoader;
    static StringTemplateGroup mCommonGroup;
    
    static TemplateConfig mTemplateConfig;

    static int TEMPLATE_INDEX = 0;
    static int FILENAME_INDEX = 1;
    static int GROUPNAME_INDEX = 2;
    static int GUARD_INDEX = 3;
    static int HEADER_INDEX = 4;

   static String[][] arrayConcat(String[][] A, String[][] B) {
        String[][] C= new String[A.length+B.length][];
        System.arraycopy(A, 0, C, 0, A.length);
        System.arraycopy(B, 0, C, A.length, B.length);

        return C;
   }
    
    static boolean mIsClient = false;
    static boolean mIsArson = false;
    static boolean mCentralOnly = false;
    static boolean mIsWAL = false;
    static boolean mIsConfigDoc = false;
    static boolean mIsLuaGen = false;
    static boolean mIsProxyGen = false;
    static boolean mIsRpc = false;
    static boolean mNoCentral = false;

    public static void main(String[] args) throws Exception 
    {
        if (args[0].equals("rpc"))
        {
            mIsRpc = true;
        }
        else if (!args[0].equals("tdf"))
        {
            throw new IllegalArgumentException("Uknown program option: " + args[0]);
        }
        
        int startFileIndex = mIsRpc ? parseRpcArgs(args) : parseTdfArgs(args);

        mTemplateConfig = TemplateConfig.GetLanguageConfig(mGenLanguage);
        
        if(mTemplateConfig == null)
        {
            throw new IllegalArgumentException("Unrecognized language specified with -lang flag: " + mGenLanguage);
        }
        
        if (startFileIndex > -1)
        {
            //Make sure neither one are null
            if (mSourceOutputDir == null)
                mSourceOutputDir = mHeaderOutputDir;
            if (mHeaderOutputDir == null)
                mHeaderOutputDir = mSourceOutputDir;
            
            StringTemplateGroup.registerGroupLoader(new PathGroupLoader(mTemplateDir, null));
            mCommonGroup = StringTemplateGroup.loadGroup("servercommon", mTemplateConfig.getStringTemplateLexerClass(), null);
            mCommonGroup.registerRenderer(String.class, new SimpleFormatRenderer());    
            mCommonGroup.registerRenderer(Integer.class, new SimpleFormatRenderer());
            
            if ((mIsWAL || mIsConfigDoc) && mIsRpc && mGenLanguage.equalsIgnoreCase("html")) {
                startFileIndex++;
            }

            if (!mCentralOnly)
            {
                for (int index = startFileIndex; index < args.length; ++index)
                {
                    try
                    {
                    mIncludePaths.clear();
                    for (String i : mBaseIncludePaths)
                    {
                        addIncludePath(i);
                    }
                    addIncludePath(args[index]);
                    
                    if (mIsRpc) 
                    {
                        if (mIsArson)
                        {
                            if (mTemplateConfig.hasArsonRpcTemplates()) 
                            {
                                generateTdfFile(args[index], null, mTemplateConfig.getArsonRpcTemplates(), false, false);
                                generateTimeStamp("arsonrpc.d");
                            }
                        }
                        else if (mIsWAL)
                        {
                            if (mTemplateConfig.hasRpcDocTemplates())
                            {
                                generateTdfFile(args[index], null, mTemplateConfig.getRpcDocTemplates(), false, false);
                            }
                        }
                        else if (mIsConfigDoc)
                        {
                            if (mTemplateConfig.hasTdfDocComponentTemplates())
                            {
                                generateTdfFile(args[index], null, mTemplateConfig.getTdfDocComponentTemplates(), false, false);
                            }
                        }
                        else if (mIsClient)
                        {
                            if (mTemplateConfig.hasClientRpcTemplates()) 
                            {
                                generateTdfFile(args[index], null, mTemplateConfig.getClientRpcTemplates(), false, false);
                                generateTimeStamp("blazesdkrpc.d");
                            }
                        }
                        else if (mTemplateConfig.hasRpcTemplates() || mTemplateConfig.hasRpcLuaTemplates())
                        {
                            String[][] templateArray = new String[0][];
                            if (!mIsProxyGen && mTemplateConfig.hasRpcTemplates())
                            {
                                templateArray = arrayConcat(templateArray, mTemplateConfig.getRpcTemplates());
                            }
                            if (mIsProxyGen && mTemplateConfig.hasRpcProxyTemplates())
                            {
                                templateArray = arrayConcat(templateArray, mTemplateConfig.getRpcProxyTemplates());
                            }
                            if (mIsLuaGen && mTemplateConfig.hasRpcLuaTemplates())
                            {
                                templateArray = arrayConcat(templateArray, mTemplateConfig.getRpcLuaTemplates());
                            }

                            if (templateArray.length > 0)
                            {
                                generateTdfFile(args[index], null, templateArray, true, false);
                                if (mIsProxyGen && mTemplateConfig.hasRpcProxyTemplates())
                                    generateFile(null, new File(mSourceOutputDir, "blazeproxyrpc.d").toString(), "", null, false, null);
                            }
                        }
                    }
                    else
                    {
                        if (mIsArson && mTemplateConfig.hasArsonClientTdfTemplates())
                        {
                            generateTdfFile(args[index], null, mTemplateConfig.getArsonClientTdfTemplates(), false, false);
                            generateTimeStamp("arsontdf.d");
                        }
                        else if (mIsClient && mTemplateConfig.hasClientTdfTemplates())
                        {
                            generateTdfFile(args[index], null, mTemplateConfig.getClientTdfTemplates(), true, false);
                            generateTimeStamp("blazesdktdf.d");
                        }
                        else if (mIsConfigDoc)
                        {
                            if (mTemplateConfig.hasTdfDocFrameworkTemplates())
                            {
                                generateTdfFile(args[index], null, mTemplateConfig.getTdfDocFrameworkTemplates(), false, false);
								generateTimeStamp("configdoctdf.d");
                            }
                            ConfigDocGenerator.generateIndex(mSourceOutputDir + "/tdf/index.html");
                        }
                        else if (mTemplateConfig.hasTdfTemplates() || mTemplateConfig.hasTdfLuaTemplates())
                        {
                            String[][] templateArray = new String[0][];
                            if (mTemplateConfig.hasTdfTemplates())
                            {
                                templateArray = arrayConcat(templateArray, mTemplateConfig.getTdfTemplates());
                            }
                            if (mIsLuaGen && mTemplateConfig.hasTdfLuaTemplates())
                            {
                                templateArray = arrayConcat(templateArray, mTemplateConfig.getTdfLuaTemplates());
                            }

                            if (templateArray.length > 0)
                            {
                                generateTdfFile(args[index], null, templateArray, true, false);
                            }
                        }
                        
                    }
                    }
                    catch (Exception e)
                    {
                        System.out.println("Error processing input file " + args[index]);
                        throw e;
                    }
                }
            }
            
            //If this is RPC we need to go one step further and do the central file list
            if (!mNoCentral)
            {
                if (mIsRpc)
                {
                    StringBuilder allFileContents = new StringBuilder();
                    for (int index = startFileIndex; index < args.length; ++index)
                    {
                        FileReader fr = new FileReader(args[index]);
                        BufferedReader br = new BufferedReader(fr);
                        String line = br.readLine();
                        
                        String relativeOutputDir = "";
                        File inputFile = new File(args[index]);
                        if (!inputFile.isAbsolute() && inputFile.getParent() != null)
                        {
                            relativeOutputDir = inputFile.getParent().toString();
                            if (relativeOutputDir.endsWith( File.separator + "gen" ) )
                                relativeOutputDir = relativeOutputDir.substring(0, relativeOutputDir.lastIndexOf(File.separator + "gen"));

                            relativeOutputDir = relativeOutputDir.replace(File.separator + "gen" + File.separator, "").replace("gen" + File.separator, "");
                            relativeOutputDir = relativeOutputDir.replace('\\', '/');
                        }
                        allFileContents.append("group \"");
                        allFileContents.append(relativeOutputDir);
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

                    if (mIsArson && mTemplateConfig.hasArsonRpcCentralTemplates())
                    {
                        generateTdfFile(args[startFileIndex], allFileContents.toString(), mTemplateConfig.getArsonRpcCentralTemplates(), false, true);    
                    }
                    else if (mIsWAL)
                    {
						if (mTemplateConfig.hasRpcDocCentralTemplates())
                        {
                            generateTdfFile(args[startFileIndex], allFileContents.toString(), mTemplateConfig.getRpcDocCentralTemplates(), false, true);
                        }
                    }
                    else if (mIsConfigDoc && mTemplateConfig.hasTdfDocComponentCentralTemplates())
                    {
                        generateTdfFile(args[startFileIndex], allFileContents.toString(), mTemplateConfig.getTdfDocComponentCentralTemplates(), false, true);
                    }
                    else if (mIsClient && mTemplateConfig.hasClientRpcCentralTemplates())
                    {
                        generateTdfFile(args[startFileIndex], allFileContents.toString(), mTemplateConfig.getClientRpcCentralTemplates(), false, true);
                    }
                    else if (mIsProxyGen && mTemplateConfig.hasRpcProxyCentralTemplates())
                    {
                        generateTdfFile(args[startFileIndex], allFileContents.toString(), mTemplateConfig.getRpcProxyCentralTemplates(), false, true);
                    }
                    else if (mTemplateConfig.hasRpcCentralTemplates() || mTemplateConfig.hasLuaRpcCentralTemplates())
                    {
                        String[][] templateArray = new String[0][];
                        if (mTemplateConfig.hasRpcCentralTemplates())
                        {
                            templateArray = arrayConcat(templateArray, mTemplateConfig.getRpcCentralTemplates());
                        }
                        if (mIsLuaGen && mTemplateConfig.hasLuaRpcCentralTemplates())
                        {
                            templateArray = arrayConcat(templateArray, mTemplateConfig.getLuaRpcCentralTemplates());
                        }
                        
                        if (templateArray.length > 0)
                        {
                            generateTdfFile(args[startFileIndex], allFileContents.toString(), templateArray, false, true);
                        }
                    } 

                    if (mIsLuaGen && mGenLanguage.equalsIgnoreCase("html"))
                    {
                        LuaDocGenerator.generate(mLuaOutputFileNames, mSourceOutputDir + "/luaindex/rpcindex.html");
                        mLuaOutputFileNames.clear();
                        mLuaOutputFileNames.add("tdfcomponentindex.html");
                        mLuaOutputFileNames.add("tdfframeworkindex.html");
                        mLuaOutputFileNames.add("tdfcustomcodeindex.html");
                        mLuaOutputFileNames.add("tdfcustomcomponentindex.html");
                        mLuaOutputFileNames.add("rpcindex.html");
                        LuaDocGenerator.generate(mLuaOutputFileNames, mSourceOutputDir + "/luaindex/index.html");
                    }

                    if (mTemplateConfig.generateDepFile())
                    {
                        //lastly we need to make the dependency and build file 
                        StringTemplateGroup depGroup = StringTemplateGroup.loadGroup("rpcdependencies", mTemplateConfig.getStringTemplateLexerClass(), null);
                        if (depGroup != null)
                        {
                            StringTemplate depTemp = depGroup.getInstanceOf("fileGroupList");
                            depTemp.setAttribute("groupList", mFileGroups.values());
                            generateFile(null, new File(mSourceOutputDir, "blazerpc.d").toString(), "", depTemp, false, null);
                        }
                    }
                    
                    //Create a new, empty build file with the current time.
                    File buildFile = new File(mSourceOutputDir, "blazerpc.build");
                    buildFile.delete();
                    buildFile.createNewFile();
                }
                else
                {
                    if (mIsLuaGen && mGenLanguage.equalsIgnoreCase("html") && !mLuaOutputFileNames.isEmpty())
                    {
                        generateLuaHtmlIndexForTdf();
                    }
                }
            }
        }
    }
    
    static void generateLuaHtmlIndexForTdf() throws Exception
    {
        Map<String, ArrayList<String> > fileNamesMap = new HashMap<String, ArrayList<String> >();
        for (String filename : mLuaOutputFileNames)
        {
            if (!filename.endsWith(".html"))
                continue;
            int endPos = filename.indexOf("/");
            String prefix = "";
            prefix = filename.substring(0, endPos);
            ArrayList<String> outputFileNames = fileNamesMap.get(prefix);
            if (outputFileNames == null)
            {
                outputFileNames = new ArrayList<String>();
                fileNamesMap.put(prefix, outputFileNames);
            }
            outputFileNames.add(filename);
        } 
        Iterator<Entry<String, ArrayList<String>>> iter = fileNamesMap.entrySet().iterator(); 
        while (iter.hasNext()) { 
            Entry<String, ArrayList<String>> entry = iter.next();
            if (!entry.getValue().isEmpty())
                LuaDocGenerator.generate(entry.getValue(), mSourceOutputDir + "/luaindex/tdf" + entry.getKey() + "index.html");
            }
        }

    static String mSourceOutputDir;
    static String mHeaderOutputDir;    
    static String mHeaderPrefix;
    static String mTemplateDir = "src\\templates";
    static String mGenLanguage = "cpp";
    
    public static int parseRpcArgs(String[] args)
    {
        int index = 1;
        for (; index < args.length; ++index)
        {
            if (args[index].equals("-h"))
            {
                index++;
                if (index == args.length)
                    return -1;//usage("Specify header directory.", argv[0]);
                mHeaderOutputDir = args[index];
            }
            else if (args[index].equals("-hp"))
            {
                index++;
                if (index == args.length)
                    return -1;
                mHeaderPrefix = args[index];
            }
            else if (args[index].equals("-s"))
            {
                index++;
                if (index == args.length)
                    return -1;//usage("Specify source directory.", argv[0]);
                mSourceOutputDir = args[index];
            }
            else if (args[index].equals("-d"))
            {
                index++;
                if (index == args.length)
                    return -1;//usage("Specify dependency file.", argv[0]);
                //depFile = args[index];
            }
            else if (args[index].equals("-c"))
            {
                index++;
                if (index == args.length)
                    return -1;//usage("Specify component list to build.", argv[0]);
            }
            else if (args[index].equals("-lang"))
            {
                index++;
                if (index == args.length)
                    return -1; //usage("Specify component list to build.", argv[0]);

                mGenLanguage = args[index];
            }
            else if (args[index].equals("-client"))
            {
                mIsClient = true;
            }
            else if (args[index].equals("-arson"))
            {
                mIsClient = true;
                mIsArson = true;
            }
            else if (args[index].equals("-wadl"))
            {
                mIsWAL = true;
            }
            else if (args[index].equals("-cfgdoc"))
            {
                mIsConfigDoc = true;
            }
            else if (args[index].equals("-centralOnly"))
            {
                mCentralOnly = true;
            }
            else if (args[index].equals("-v"))
            {
                //gVerbose = true;
            }
            else if (args[index].equals("-t"))
            {
                index++;
                mTemplateDir = args[index];
            }
            else if (args[index].equals("-I"))
            {
                index++;
                mBaseIncludePaths.add(args[index]);
            }
            else if (args[index].equals("-luagen"))
            {
                mIsLuaGen = true;
            }
            else if (args[index].equals("-proxy"))
            {
                mIsProxyGen = true;
            }
            else if (args[index].equals("-nocentral"))
            {
                mNoCentral = true;
            }
            else if (args[index].charAt(0) == '-')
            {
                //char msg[256];
                //sprintf(msg, "Unknown command \'%s\'", args[index]);
                return -1; //usage(msg, argv[0]);
            }
            else
            {
                return index;
            }
        }
    
        return -1;
    }

    public static int parseTdfArgs(String[] args)
    {
        int index = 1;
        for (; index < args.length; ++index)
        {
            if (args[index].equals("-oh"))
            {
                index++;
                if (index == args.length)
                    return -1; //usage(argv[0]);;
                mHeaderOutputDir = args[index];
            }
            else if (args[index].equals("-hp"))
            {
                index++;
                if (index == args.length)
                    return -1; //usage(argv[0]);;
                mHeaderPrefix = args[index];
            }
            else if (args[index].equals("-oc"))
            {
                index++;
                if (index == args.length)
                    return -1; //usage(argv[0]);;
                mSourceOutputDir = args[index];
            }
            else if (args[index].equals("-d"))
            {
                index++;
                if (index == args.length)
                    return -1; //usage(argv[0]);;
                //dependencyFile = args[index];
            }
            else if (args[index].equals("-i"))
            {
                index++;
                if (index == args.length)
                    return -1; //usage(argv[0]);;
                //includePrefix = args[index];
            }
            else if (args[index].equals("-lang"))
            {
                index++;
                if (index == args.length)
                    return -1; //usage("Specify component list to build.", argv[0]);

                mGenLanguage = args[index];
            }
            else if (args[index].equals("-client"))
            {
                mIsClient = true;
            }
            else if (args[index].equals("-arson"))
            {
                mIsClient = true;
                mIsArson = true;
            }
            else if (args[index].equals("-luagen"))
            {
                mIsLuaGen = true;
            }
            else if (args[index].equals("-proxy"))
            {
                mIsProxyGen = true;
            }
            else if (args[index].equals("-cfgdoc"))
            {
                mIsConfigDoc = true;
            }
            else if (args[index].equals("-v"))
            {
                //gTdfVerbose = true;
            }
            else if (args[index].equals("-t"))
            {
                index++;
                mTemplateDir = args[index];
            }
            else if (args[index].equals("-sdk"))
            {
                index++;
                mBaseIncludePaths.add(args[index]);
            }
            else if (args[index].charAt(0) == '-')
            {
                return -1; //usage(argv[0]);;
            }
            else
            {
                return index;
            }
        }
    
        return -1;
    }

}
