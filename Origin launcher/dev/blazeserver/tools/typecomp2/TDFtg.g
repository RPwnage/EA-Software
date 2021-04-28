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
}

@members
{
ExtHashtable mTemplateTable;
boolean mProcessMasters;
boolean mIsClient;
Stack<String> mNameStack;
HashToken mFileToken;
Stack<String> mClassStack;
Stack<String> mNameSpaces;
boolean mIsCustomComponent;
}

/* ================================================================================
                                          Highest Level 
   ================================================================================*/
//TODO file params
file [ExtHashtable templateTable, boolean isClient, boolean noCentral]        
        @init
        {
            String inputFile;
			String outputFileBase;
            mNameStack = new Stack<String>();
            mNameSpaces = new Stack<String>();
            mClassStack = new Stack<String>();
            mTemplateTable = templateTable;
            mProcessMasters = !isClient;
            mIsClient = isClient;
            mIsCustomComponent = isClient && noCentral;
        }
        @after
        {
                //Theres only one member of our table after we're done - the whole file
                ArrayList<String> list = new ArrayList<String>();
                
                if (inputFile.endsWith(".tdf"))
                {
                     //Using a file like this strips off all but the actual name of the file.
                     File f = new File(inputFile);               
                     list.add(f.getName().replace(".tdf", ""));
					 list.add(outputFileBase.replace(".tdf", "").replace("/", "_"));
                }
                templateTable.put(list, $st);
        }
        :       ^(FILE {mFileToken = $FILE.getHashToken(); inputFile = $FILE.getString("InputFile"); outputFileBase = $FILE.getString("OutputFileBase");} file_member*)
               -> {inputFile.endsWith(".tdf") }? file(fileNode={$FILE.getHashToken()}, defTable={new DefineHelper()})
               -> rootComponent(file={mFileToken}, node={$FILE.getToken()})                      
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
        @after 
        {
            String className = mClassStack.pop();
            if(mClassStack.empty())
            {
                if ($st != null) {
                    String nameSpaceDir = "";
                    for(String nameSpace : mNameSpaces)
                    {
                       nameSpaceDir += nameSpace.toLowerCase() + "/";
                    }
                    mNameStack.push(nameSpaceDir);
                    mTemplateTable.put(new ArrayList<String>(mNameStack), $st);
                    mNameStack.pop();
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
            mNameStack.pop();
        }
        :   ^(CLASS {mClassStack.push($CLASS.getString("Name")); mNameStack.push($CLASS.getString("Name"));} (type_definition_block | class_member_def)*)
            -> singleClass(classNode={$CLASS.getToken()}, defTable={new DefineHelper()})
        ;

class_member_def
        :       ^(MEMBER_DEF type)
        ;

union_block
        @after 
        {
            if ($st != null) {
                String nameSpaceDir = "";
                for(String nameSpace : mNameSpaces)
                {
                   nameSpaceDir += nameSpace.toLowerCase() + "/";
                }
                mNameStack.push(nameSpaceDir);
                mTemplateTable.put(new ArrayList<String>(mNameStack), $st);
                mNameStack.pop();
            }
            mNameStack.pop();
        }
        :   ^(UNION {mNameStack.push($UNION.getString("Name"));} union_member*)
            -> { mClassStack.empty() }? singleClass(classNode={$UNION.getToken()}, defTable={new DefineHelper()})
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
        @after 
        {
            if ($st != null) {
                String nameSpaceDir = "";
                for(String nameSpace : mNameSpaces)
                {
                   nameSpaceDir += nameSpace.toLowerCase() + "/";
                }
                mNameStack.push(nameSpaceDir);
                mTemplateTable.put(new ArrayList<String>(mNameStack), $st);
                mNameStack.pop();
            }
            mNameStack.pop();
        }
        :   ^(BITFIELD {mNameStack.push($BITFIELD.getString("Name"));} (MEMBER_DEF)*)
            -> { mClassStack.empty() }? singleClass(classNode={$BITFIELD.getToken()}, defTable={new DefineHelper()})
            ->
        ;
                        
enum_block
        @after 
        {
            if ($st != null) {
                String nameSpaceDir = "";
                for(String nameSpace : mNameSpaces)
                {
                   nameSpaceDir += nameSpace.toLowerCase() + "/";
                }
                mNameStack.push(nameSpaceDir);
                mTemplateTable.put(new ArrayList<String>(mNameStack), $st);
                mNameStack.pop();
            }
            mNameStack.pop();
        }
        :   ^(ENUM {mNameStack.push($ENUM.getString("Name"));} (MEMBER_DEF)*)
            -> { mClassStack.empty() }? singleClass(classNode={$ENUM.getToken()}, defTable={new DefineHelper()})
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
	;

component_block
        @after 
        {
            if ($st != null) {
                mTemplateTable.put(new ArrayList<String>(mNameStack), $st);
            }
            mNameStack.pop();
        }
        : ^(COMPONENT 
          {
             mNameStack.push($COMPONENT.getString("Name"));
          }
        component_members*
        ) -> baseComponent(file={mFileToken}, component={$COMPONENT.getToken()})
        ;
        
        
component_members
        : subcomponent_block 
        | errors_block
        | TYPE
        | permissions_block
        ;
        
subcomponent_block returns [String name]
        scope { HashToken component; }
        @after 
        {
            if ($st != null) {
                mTemplateTable.put(new ArrayList<String>(mNameStack), $st);
            }
            mNameStack.pop();
        }
        : ^(SLAVE { $subcomponent_block::component = $SLAVE.getHashToken(); 
                     mNameStack.push("Slave"); } subcomponent_members*)
            -> {!mIsCustomComponent}? slaveComponent(file={mFileToken}, component={$SLAVE.getToken()})
            -> slaveCustomComponent(file={mFileToken}, component={$SLAVE.getToken()})
        | ^(MASTER { $subcomponent_block::component = $MASTER.getHashToken(); 
                     mNameStack.push("Master"); } subcomponent_members*) 
            -> { mProcessMasters }? masterComponent(file={mFileToken}, component={$MASTER.getToken()})
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
            mTemplateTable.put(new ArrayList<String>(mNameStack), $st);
            mNameStack.pop();
        }
        : COMMAND { mNameStack.push($COMMAND.getString("Name")); } 
                -> { !mIsClient || $subcomponent_block::component.containsKey("IsSlave") }? 
                        command(file={mFileToken}, command={$COMMAND.getToken()}, component={$subcomponent_block::component}) 
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
        

/* ================================================================================
                                          Types
   ================================================================================*/
type    :       INT_PRIMITIVE
        |       CHAR_PRIMITIVE
        |       FLOAT_PRIMITIVE
        |       BLOB
        |       BLAZE_OBJECT_TYPE
        |       BLAZE_OBJECT_ID
        |       BLAZE_OBJECT_PRIMITIVE
        |       TIMEVALUE
        |       VARIABLE
        |       TYPE_REF
        |       MAP
        |       LIST
        |       STRING
        ;

