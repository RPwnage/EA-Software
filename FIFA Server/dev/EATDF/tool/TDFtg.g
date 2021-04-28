tree grammar TDFtg;

/* ================================================================================
                                          Options 
   ================================================================================*/
options
{
        language='Java';
        tokenVocab=TdfGrammar;
        ASTLabelType=HashTree;
        output=template;
}


@header
{
import java.util.*;
import org.antlr.runtime.BitSet;
import java.io.File;
import java.util.regex.*;
}

@members
{
ExtHashtable mTemplateTable;
boolean mProcessMasters;
HashToken mFileToken;
Stack<String> mClassStack;
Stack<String> mNameSpaces;
String mOutputFilePattern;
ExtHashtable mInputParams;

    void addTemplate(StringTemplate st) throws NullPointerException
    {
        if (st == null || st.getChunks() == null || st.getChunks().size() == 0)
            return;
            
        Pattern p = Pattern.compile("\\{[a-zA-Z_][a-zA-Z_]*\\}");
        Matcher m = p.matcher(mOutputFilePattern);
        StringBuffer sb = new StringBuffer();
        while (m.find()) {
            String varName = m.group().substring(1, m.group().length() - 1);
            String replacement = (String) mInputParams.get(varName);
            if (replacement == null) 
                throw new NullPointerException("Cannot find variable " + varName + " for template output file pattern " + mOutputFilePattern);            
            m.appendReplacement(sb, replacement);
        }
        m.appendTail(sb);        
        mTemplateTable.put(sb.toString(), st);
    }
    
    void addTdfDefinitionTemplate(String defName, StringTemplate st)
    {
        String nameSpaceDir = "";
        for(String nameSpace : mNameSpaces)
        {
           nameSpaceDir += nameSpace.toLowerCase() + "/";
        }
        mInputParams.put("NamespacePath", nameSpaceDir);       
        mInputParams.put("TdfDefinitionName", defName);
        
        addTemplate(st);        
    }
    
}

/* ================================================================================
                                          Highest Level 
   ================================================================================*/
//TODO file params
file [ExtHashtable templateTable, boolean processMasters, String outputFilePattern, ExtHashtable inputParams]        
        @init
        {
            String inputFile;
            mNameSpaces = new Stack<String>();
            mClassStack = new Stack<String>();
            mTemplateTable = templateTable;
            mProcessMasters = processMasters;
            mOutputFilePattern = outputFilePattern;
            mInputParams = inputParams;            
        }
        @after
        {               
                addTemplate($st);
        }
        :       ^(FILE {mFileToken = $FILE.getHashToken(); inputFile = $FILE.getString("InputFile"); } file_member*)
               -> {inputFile.endsWith(".tdf") }? file(fileNode={$FILE.getHashToken()}, defTable={new DefineHelper()})
               -> rootComponent(fileNode={mFileToken}, node={$FILE.getToken()})                      
       ;
        
/* ================================================================================
                                          Main Elements
   ================================================================================*/
file_member
        : namespace_member 
        | include_block 
        | group_block
        ;

include_block 
        : INCLUDE
        | FAKE_INCLUDE
        ;

namespace_block 
        @after
        {
            mNameSpaces.pop();
        }
        : ^(NAMESPACE {mNameSpaces.push($NAMESPACE.getString("Name"));} namespace_member*)
        ;
        
namespace_member
        : namespace_block
        | type_definition_block 
        | root_component_block
        | component_block
        ;

group_block
        : ^(GROUP group_member*)
        ;
        
group_member
        : namespace_member
        | include_block
        ;
        
/* ================================================================================
                                      TDF    Types
   ================================================================================*/        
        
type_definition_block
        : class_fwd_decl_block
        | class_block 
        | typedef_block 
        | bitfield_block 
        | const_block 
        | enum_block 
        | union_block 
        | custom_attribute_block        
        ;

custom_attribute_block    
        : ^(CUSTOM_ATTRIBUTE custom_attribute_member*)
        ;

custom_attribute_member
        : ^(MEMBER_DEF custom_attribute_type)
        ;

custom_attribute_type
        : INT_PRIMITIVE 
        | CHAR_PRIMITIVE
        | STRING
        ;

class_fwd_decl_block
        : CLASS_FWD_DECL
        ;

class_block
        scope { String tdfDefinitionName; }
        @after 
        {
            String className = mClassStack.pop();
            if(mClassStack.empty())
            {
                if ($st != null) {                    
                    addTdfDefinitionTemplate($class_block::tdfDefinitionName, $st);                    
                }
                else
                {
                    String nameSpaceDir = "";
                    for(String nameSpace : mNameSpaces)
                    {
                       nameSpaceDir += nameSpace.toLowerCase() + "::";
                    }

                    throw new NullPointerException("Top level class " + nameSpaceDir + className + " should have but failed to produce a generated string template.");
                }
            }
        }
        :   ^(CLASS {mClassStack.push($CLASS.getString("Name")); $class_block::tdfDefinitionName = $CLASS.getString("Name");} (type_definition_block | class_member_def)*)
            -> singleClass(fileNode={mFileToken}, classNode={$CLASS.getToken()}, defTable={new DefineHelper()})
        ;

class_member_def
        :       ^(MEMBER_DEF type)
        ;

union_block
        scope { String tdfDefinitionName; }
        @after 
        {
            if ($st != null) {                
                addTdfDefinitionTemplate($union_block::tdfDefinitionName, $st);
            }
        }
        :   ^(UNION { $union_block::tdfDefinitionName = $UNION.getString("Name");} union_member*)
            -> { mClassStack.empty() }? singleClass(fileNode={mFileToken}, classNode={$UNION.getToken()}, defTable={new DefineHelper()})
            ->
        ;

union_member
        :       ^(MEMBER_DEF type)
        ;
        

typedef_block
        :       ^(TYPEDEF type)
        ;       
        
const_block
        :       CONST 
        |       STRING_CONST 
        ;
        

bitfield_block
        scope { String tdfDefinitionName; }
        @after 
        {
            if ($st != null) {
                addTdfDefinitionTemplate($bitfield_block::tdfDefinitionName, $st);        
            }
        }
        :   ^(BITFIELD { $bitfield_block::tdfDefinitionName = $BITFIELD.getString("Name");} (MEMBER_DEF)*)
            -> { mClassStack.empty() }? singleClass(fileNode={mFileToken}, classNode={$BITFIELD.getToken()}, defTable={new DefineHelper()})
            ->
        ;
                        
enum_block
        scope { String tdfDefinitionName; }
        @after 
        {
            if ($st != null) {
                addTdfDefinitionTemplate($enum_block::tdfDefinitionName, $st);
            }
        }
        :   ^(ENUM { $enum_block::tdfDefinitionName = $ENUM.getString("Name");} (MEMBER_DEF)*)
            -> { mClassStack.empty() }? singleClass(fileNode={mFileToken}, classNode={$ENUM.getToken()}, defTable={new DefineHelper()})
            ->
        ;


/* ================================================================================
                                     RPC Types
   ================================================================================*/

root_component_block 
    : ^(ROOTCOMPONENT root_component_members*)
    ;
    
root_component_members
    : errors_block
    | sdk_errors_block
    | permissions_block
    | alloc_groups_block
    ;

component_block
        @after 
        {
            if ($st != null) {
                addTemplate($st);
            }
        }
        : ^(COMPONENT 
          {
            mInputParams.put("ComponentName", $COMPONENT.getString("Name"));
          }
        component_members*
        ) -> baseComponent(fileNode={mFileToken}, component={$COMPONENT.getToken()})
        ;
        
        
component_members
        : subcomponent_block 
        | errors_block
        | TYPE
        | permissions_block
        | alloc_groups_block
        ;
        
subcomponent_block returns [String name]
        scope { HashToken component; }
        @after 
        {
            if ($st != null) {
                addTemplate($st);
            }          
        }
        : ^(SLAVE { $subcomponent_block::component = $SLAVE.getHashToken(); 
                     mInputParams.put("ComponentType", "Slave"); } subcomponent_members*)
            -> slaveComponent(fileNode={mFileToken}, component={$SLAVE.getToken()})
        | ^(MASTER { $subcomponent_block::component = $MASTER.getHashToken(); 
                     mInputParams.put("ComponentType", "Master"); } subcomponent_members*) 
            -> { mProcessMasters }? masterComponent(fileNode={mFileToken}, component={$MASTER.getToken()})
            ->
        ;
        
subcomponent_members
        :  subcomponent_template_members
        | INCLUDE 
        | NOTIFICATION 
        | EVENT 
        | DYNAMIC_MAP 
        | STATIC_MAP 
        ;
        
subcomponent_template_members
        @after 
        {
            addTemplate($st);
        }
        : COMMAND { mInputParams.put("CommandName", $COMMAND.getString("Name")); } 
                -> { mProcessMasters || $subcomponent_block::component.containsKey("IsSlave") }? 
                        command(fileNode={mFileToken}, command={$COMMAND.getToken()}, component={$subcomponent_block::component}) 
                ->
        ;
        
errors_block
        : ^(ERRORS ERROR*)
        ;
        

sdk_errors_block
        : ^(SDK_ERRORS ERROR*)
        ;
        
permissions_block
        : ^(PERMISSIONS PERMISSION*)
        ;
        
alloc_groups_block
        : ^(ALLOC_GROUPS ALLOC_GROUP*)
        ;        

/* ================================================================================
                                          Types
   ================================================================================*/
type    :       INT_PRIMITIVE
        |       CHAR_PRIMITIVE
        |       FLOAT_PRIMITIVE
        |       BLOB
        |       OBJECT_TYPE
        |       OBJECT_ID
        |       OBJECT_PRIMITIVE
        |       TIMEVALUE
        |       VARIABLE
        |       GENERIC
        |       TYPE_REF
        |       MAP
        |       LIST
        |       STRING
        ;

