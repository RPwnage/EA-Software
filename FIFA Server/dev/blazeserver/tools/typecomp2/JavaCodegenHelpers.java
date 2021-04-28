import java.io.*;
import java.util.*;
import java.util.Map.Entry;

public class JavaCodegenHelpers
{    
    private static final String NEWLINE = System.getProperty("line.separator");

    // Reads in a generated constants txt file and adds all the constants and their respective namespaces to the map.
    // The map consists of the constant's namespace as the key and the vector of constants that belong to that namespace as the value.
    public static void consolidateConstants(String fileName, HashMap<String, Vector<String>> constants) {        
        Scanner fin = null;
        
        try {
            fin = new Scanner(new File(fileName));
        }
        catch(FileNotFoundException e) {
            System.out.println("Input file not found.");
        }
        
        fin.useDelimiter("#&"); // Each constant entry in the file starts with this token. 
        
        fin.next();
        
        while(fin.hasNext()) {
            addConstant(fin.next(), constants);    
        }
    }

    private static void addConstant(String entry, HashMap<String, Vector<String>> constants) {
        int endLineIndex; 
        
        String namespace; 
        String value;
        
        endLineIndex = entry.indexOf(NEWLINE); // Get the first newline in the entry which is between the namespace and the actual constant.
      
        namespace = entry.substring(0, endLineIndex); // Fetch the namespace
        
        value = entry.substring(endLineIndex + NEWLINE.length(), entry.length() - NEWLINE.length()); // Store the constant and the corresponding access modifiers. 

        if(!constants.containsKey(namespace)) {
            constants.put(namespace, new Vector<String>());
        }
                    
        constants.get(namespace).add(value);
    }

    // Writes out the constants from the map in java files organized by namespace.
    public static void generateConstantsFiles(String outputDirectory, HashMap<String, Vector<String>> constants)
    {
        String outputFileName;
        
        String constant;
        
        Map.Entry<String, Vector<String>> constantEntry;
        
        Iterator<String> namespaceConstants;
        
        Iterator<Entry<String, Vector<String>>> it = constants.entrySet().iterator();
        
        FileWriter fw = null;
        
        outputDirectory += "/constants/";
        
        while(it.hasNext()) {    
            constantEntry = it.next();
            
            outputFileName = constantEntry.getKey();
        
            namespaceConstants = constantEntry.getValue().iterator();

            try {
                outputFileName = outputFileName.replace('.', '/');
                new File(outputDirectory + outputFileName).mkdirs();
                outputFileName += "/Constants.java";

                File constantsFile = new File(outputDirectory + outputFileName);
                fw = new FileWriter(new File(outputDirectory + outputFileName));
            }
            catch(IOException e) {
                System.out.println("Cannot create file." + outputDirectory);
            }
            
            try {
                fw.write("package " + constantEntry.getKey() + ";" + NEWLINE + NEWLINE);

                fw.write("/**" + NEWLINE + " * Constants for the package " + constantEntry.getKey() + NEWLINE + " */" + NEWLINE);

                fw.write("public class Constants" + NEWLINE + "{" + NEWLINE);
                
                while(namespaceConstants.hasNext()) {
                    constant = namespaceConstants.next();
                    namespaceConstants.remove();

                    StringTokenizer tok = new StringTokenizer(constant, NEWLINE);
                    while(tok.hasMoreTokens())
                    {
                        fw.write("    " + tok.nextToken());
                        fw.write(NEWLINE);    
                    }

                    fw.write(NEWLINE);
                }
                
                fw.write(NEWLINE + "}" + NEWLINE);
            }
            catch(IOException e) {
                System.out.println("Cannot write to file.");
            }
            
            try {
                fw.close();
            }
            catch(IOException e) {
                System.out.println("Cannot close file.");
            }
        }
    }

    /**
     * This main is called by the constants task defined in blazejava.xml
     * It accepts two arguments, the first is the output dir of the consolidated
     * constants file, and the second is a text file containing the list of 
     * *_constants.txt that we are generating from.
     */
    public static void main(String[] args) throws Exception 
    {
        if(args.length < 2)
        {
            System.err.println("Missing arguments to JavaCodegenHelper main.");
            System.exit(-1);
        }
        else
        {
            String outputDirectory = args[0];
            String inputFileList = args[1];
            
            HashMap<String, Vector<String>> constantsMap = new HashMap<String, Vector<String>>();

            BufferedReader in = new BufferedReader(new FileReader(inputFileList));
            String filename; 

            while((filename = in.readLine()) != null) {
                JavaCodegenHelpers.consolidateConstants(filename, constantsMap);
            }

            JavaCodegenHelpers.generateConstantsFiles(outputDirectory, constantsMap);
        }
    }
}
