grammar TdfGrammar;

/* ================================================================================
                                          Options 
   ================================================================================*/
options
{
        language='Java';
        ASTLabelType=HashTree;
        output=AST;
        backtrack=true;
}

tokens 
{
        FILE;
        TYPE_REF;
        MEMBER_DEF;
        STRING_CONST;
        CLASS_FWD_DECL;
        COMMAND;
        NOTIFICATION;
        EVENT;
        ERROR;
        TYPE;
        ATTRIBS;
        ATTRIB_MAP;
        ATTRIB_LIST;
        ATTRIB_PAIR;
        BOOLEAN;
        TYPE_PAIR;
        FAKE_INCLUDE;
        CUSTOM_ATTRIBUTE;
        CUSTOM_ATTRIBUTE_INSTANCE;
        PERMISSION;
        ALLOC_GROUP;
}

@header
{
import java.util.*;
import java.io.File;
import org.antlr.runtime.BitSet;
import java.lang.String;
}

@lexer::members
{
    public void displayRecognitionError(String[] tokenNames, RecognitionException e)
    {
         String hdr = getErrorHeader(e);
         String msg = getErrorMessage(e, tokenNames);
         emitErrorMessage(getSourceName() + ": " + hdr+" "+msg);

         throw new IllegalArgumentException("Lexer err - stopping compilation.");
    }
}

@parser::members
{

    public void displayRecognitionError(String[] tokenNames, RecognitionException e)
    {
         String hdr = getErrorHeader(e);
         String msg = getErrorMessage(e, tokenNames);
         emitErrorMessage(getSourceName() + ": " + hdr+" "+msg);
    
         throw new IllegalArgumentException("Parser err - stopping compilation.");
    }
    
    // stripMemberName function does following conversion:    
    // mMemberName -> MemberName
    // memberName  -> MemberName
    // MemberName  -> MemberName
    // MEMBERNAME  -> MEMBERNAME (enums - leave untouched)
    String stripMemberName(String name)
    {
        String result = name;
                
        //Get rid of the 'm' if followed by an uppercase letter
        if (name.length() > 1 && name.charAt(0) == 'm' && Character.isUpperCase(name.charAt(1))) 
        {
            result = name.substring(1);
        }

        //Uppercase the first letter                
        result = Character.toUpperCase(result.charAt(0)) + result.substring(1);
        return result;
    }
    
    // This is the string that will be used when parsing a variable. By default, it is the variable's name.
    // If a "name" attribute is give, that will be used instead. 
    String genMemberParseName(String name, Object attrName)
    {
        String result = "m" + stripMemberName(name);

        if (attrName != null)
        {
            String attrNameString = attrName.toString();

            if (attrNameString.length() < 1)
            {
                throw new NullPointerException("Member " + name + " has an empty attribute Name (" + attrNameString + "). Add a name, or remove the attribute.");
            }

            result = attrNameString;
        }
        return result;
    }
        
    static long TAG_CHAR_MIN = 32;
            
    static long generateMemberTag(String memberName, Object tag)
    {
        if (tag == null)
            throw new NullPointerException("Member " + memberName + " has no tag.");      

        String stringTag = tag.toString().toUpperCase();

        if (stringTag.length() > 4)
            throw new NullPointerException("Member " + memberName + " has a tag (" + stringTag + ") exceeding the maximum length of 4 characters");
        
        long longTag = ((stringTag.charAt(0) - TAG_CHAR_MIN) & 0x3f) << 26;
        if (stringTag.length() > 1)
        {
            longTag |= (((stringTag.charAt(1) - TAG_CHAR_MIN) & 0x3f) << 20);
            if (stringTag.length() > 2)
            {
                longTag |= (((stringTag.charAt(2) - TAG_CHAR_MIN) & 0x3f) << 14);
                if (stringTag.length() > 3)
                {
                    longTag |= ((stringTag.charAt(3) - TAG_CHAR_MIN) & 0x3f) << 8;
                }
            }
        }
        return longTag;
    }


    static long generateProtobufTagFromMemberTag(Object tag)
    {
        String stringTag = tag.toString().toUpperCase();

        long longTag = ((stringTag.charAt(0) - TAG_CHAR_MIN) & 0x3f);
        if (stringTag.length() > 1)
        {
            longTag |= (((stringTag.charAt(1) - TAG_CHAR_MIN) & 0x3f) << 6);
            if (stringTag.length() > 2)
            {
                longTag |= (((stringTag.charAt(2) - TAG_CHAR_MIN) & 0x3f) << 12);
                if (stringTag.length() > 3)
                {
                    longTag |= ((stringTag.charAt(3) - TAG_CHAR_MIN) & 0x3f) << 18;
                }
            }
        }
        return longTag;
    }

    static long generateProtobufTagForUnion(int defaultVal, Object tag)
    {
        String stringTag = tag.toString().toUpperCase();

        if (stringTag.equals("VALU"))
            return defaultVal;

        return generateProtobufTagFromMemberTag(tag);
    }

    // Generate the protobuf tag from notification id. This is required otherwise the tag could keep on changing from release to release (presenting backward compatibility challenges)
    // depending on addition/removal of a notification. 
    static int generateProtobufTagFromNotificationId(int id)
    {
        // 0 is not a valid protobuf tag number. Use a hard-coded magic number. 
        if (id == 0)
            return 20002; // If this changes, change the code in framework/component/component.h    

        return id;
    }

    static String generateReconfigurableValue(Object reconfigurable)
    {
        if (reconfigurable != null)
        {
            String reconfigString = reconfigurable.toString().toLowerCase();
            if (reconfigString.equals("yes") || reconfigString.equals("true"))
            {
                return "EA::TDF::TdfMemberInfo::RECONFIGURE_YES";
            }
            else if (reconfigString.equals("no") || reconfigString.equals("false"))
            {
                return "EA::TDF::TdfMemberInfo::RECONFIGURE_NO";
            }
            else if (reconfigString.equals("default"))
            {
                return "EA::TDF::TdfMemberInfo::RECONFIGURE_DEFAULT";
            }
            else
            {
                throw new IllegalArgumentException("Invalid value specified for 'reconfigurable' attribute: " + reconfigString);
            }
        }

        return "EA::TDF::TdfMemberInfo::RECONFIGURE_DEFAULT";
    }

    static String generatePrintFormat(Object printFormat)
    {
        if (printFormat != null)
        {
            String formatString = printFormat.toString().toLowerCase();
            if (formatString.equals("normal"))
            {
                return "EA::TDF::TdfMemberInfo::NORMAL";
            }
            else if (formatString.equals("censor"))
            {
                return "EA::TDF::TdfMemberInfo::CENSOR";
            }
            else if (formatString.equals("hash"))
            {
                return "EA::TDF::TdfMemberInfo::HASH";
            }
            else if (formatString.equals("lower"))
            {
                return "EA::TDF::TdfMemberInfo::LOWER";
            }
            else
            {
                throw new IllegalArgumentException("Invalid value specified for 'printFormat' attribute: " + formatString);
            }
        }

        return "EA::TDF::TdfMemberInfo::NORMAL";
    }

    boolean includeProto(String relativeFileName, Object includeProtoAttrib)
    {
        // These protos are not even included in code generation (so no file generated with this name/path) as opposed to a partials protos when generateProto
        // attribute is used.
        if (isCustomProtoBlacklisted(relativeFileName))
            return false;

        return boolFromAttrib(includeProtoAttrib, true); // By default, all non-blacklisted protos are included unless they have been explicitly tagged for exclusion. 
    }

    static boolean boolFromAttrib(Object attrib, boolean defaultReturn)
    {
        if (attrib != null)
        {
            String attribString = attrib.toString().toLowerCase();
            if (attribString.equals("no") || attribString.equals("false"))
            {
                return false;
            }
            else if(attribString.equals("yes") || attribString.equals("true"))
            {
                return true;
            }
        }

        return defaultReturn;
    }

    boolean isCustomProtoBlacklisted(String fileName)
    {
        if (fileName.contains("customcomponent/") || fileName.contains("customcode/component/"))
        {
            if (!mGenerateCustomProtos)
                return true;
            
            if (fileName.contains("customcomponent/") && mProtoBlackListedCustomComps != null)
            {
                for (int i = 0; i < mProtoBlackListedCustomComps.length; ++i)
                {
                    if (fileName.contains("customcomponent/"+mProtoBlackListedCustomComps[i]+"/"))
                    {
                        return true;
                    }
                }
            }

            if (fileName.contains("customcode/component/") && mProtoBlackListedCustomCode != null)
            {
                for (int i = 0; i < mProtoBlackListedCustomCode.length; ++i)
                {
                    if (fileName.contains("customcode/component/"+mProtoBlackListedCustomCode[i]+"/"))
                    {
                        return true;
                    }
                }
            }
         }

         return false;
    }

    // The .generateProto attribute at tdf level
    boolean generateProtoMsg(Object generateProtoAttrib)
    {
        if (!mGenerateProtos)
            return false;

        if (isCustomProtoBlacklisted(mInputFileName))
            return false; 

        // PROTOPLUGIN_TODO: enable once we're ready to transition the below disabled items to only use .protos. See Blaze docs

        // For proxy components, only create proto message if tagged explicitly. This keeps layering violation to a minimum and
        // helps reduce the code size. 
        if (mInputFileName.contains("proxycomponent/"))
            return boolFromAttrib(generateProtoAttrib, false);

        // For framework, we always generate proto message unless tagged to be false explicitly. Tagging to false is necessary in 
        // case of incompatible code generation scenarios for tdfs that are not serialized over the wire but there is no automated
        // way of knowing that(for example, config tdfs)
        if (mInputFileName.contains("framework/"))
            return boolFromAttrib(generateProtoAttrib, true);

        // Any events defined in the file that ends in events_server.tdf suffix get automatically exported to proto files. If an event
        // is in a different _server.tdf file, it has to be manually tagged (or moved to a *events_server.tdf file)
        if (mInputFileName.endsWith("events_server.tdf"))
            return boolFromAttrib(generateProtoAttrib, true);

        // For regular components, in server tdfs, we only generate proto messages if tagged to be true explicitly. Again, this saves
        // code size and we don't have to bother with rpcs that are only meant for in-cluster communication and we have no automated
        // way of knowing that.
        if (mInputFileName.endsWith("_server.tdf"))
            return boolFromAttrib(generateProtoAttrib, false);

        // All the regular tdfs generate the proto message unless explicitly disabled.
        return boolFromAttrib(generateProtoAttrib, true);
    }

    // The .generateProto attribute at rpc level (including notification)
    boolean generateProtoRpc(Object generateProtoAttrib, boolean isMaster)
    {
        if (!mGenerateProtos)
            return false;

        // PROTOPLUGIN_TODO: enable once we're ready to transition the below disabled items to only use .protos. See Blaze docs

        if (mInputFileName.contains("proxycomponent/"))
            return false;

        if (isCustomProtoBlacklisted(mInputFileName))
            return false;

        if (mInputFileName.contains("framework/"))
            return boolFromAttrib(generateProtoAttrib, true);

        if (mInputFileName.endsWith("_server.rpc"))
            return boolFromAttrib(generateProtoAttrib, false);

        // For non-framework master components, rpc's have to be explicitly tagged 
        if (isMaster)
            return boolFromAttrib(generateProtoAttrib, false);
            
        // All the regular rpc generate the proto rpc unless explicitly disabled.
        return boolFromAttrib(generateProtoAttrib, true);
    }
    
    // The .generateProto attribute at component level
    boolean generateProtoService(Object generateProtoAttrib, boolean isMaster)
    {
        return generateProtoRpc(generateProtoAttrib, isMaster);
    }

    boolean generateProtoEvent(Object generateProtoAttrib)
    {
        // Events are exported regardless of whether they are in slave or master component.
        return generateProtoRpc(generateProtoAttrib, false);
    }

    static boolean useProtobufWrapper(Object protoWrapperAttrib)
    {
        if (protoWrapperAttrib != null)
        {
            String protoWrapperString = protoWrapperAttrib.toString().toLowerCase();
            if (protoWrapperString.equals("no") || protoWrapperString.equals("false"))
            {
                return false;
            }
            else if(protoWrapperString.equals("yes") || protoWrapperString.equals("true"))
            {
                return true;
            } 
        }

        return false;
    }

    boolean mIsClient;
    // Turns out it is exceptionally tricky to get the right proto package name from the tdf file processing. The nodes are process on-the-go and we don't know ahead of the time
    // how many namespaces node we have in the file. So as we encounter the namespace blocks, they keep getting added to the string below. Fortunately, the ProtoC compiler allows
    // us to put the package attibute anywhere in the file. So after the file is traversed, we put the string below at the end of the file. 
    String mProtoPackageName; 

    boolean mMultipleNamespaces;
    boolean mClosedANamespace;

    String mInputFileName;

    boolean mGenerateProtos;
    boolean mGenerateCustomProtos;
    boolean mGenerateProtoAttribDefault;

    String mProtoRootDir;
    String[] mProtoBlackListedCustomComps;
    String[] mProtoBlackListedCustomCode;
}

 
/* ================================================================================
                                          Highest Level 
   ================================================================================*/
file[String inputFile, boolean topLevel, boolean isClient, ExtHashtable inputParams]
        @init 
        { 
            mIsClient = isClient; 
            mProtoPackageName = new String();

            mMultipleNamespaces = false;
            mClosedANamespace = false;

            mInputFileName = inputFile;

            // mGenerateProtos ensures that the Proto code generation does not kick in on the (BlazeSDK) client.
            mGenerateProtos = inputParams.get("generateProtos") != null ? 
                                                inputParams.get("generateProtos").equals("true") : false;

            mGenerateCustomProtos = inputParams.get("generateCustomProtos") != null ? 
                                                inputParams.get("generateCustomProtos").equals("true") : false;

            mProtoRootDir = inputParams.get("protoRootDir") != null ? 
                                                inputParams.get("protoRootDir").toString() : "";

            String protoBlackListedCustomComps = (inputParams.get("customCompBlacklistProtos") != null ? 
                                                inputParams.get("customCompBlacklistProtos").toString() : "").trim();
            
            if (protoBlackListedCustomComps.length() > 0)
                mProtoBlackListedCustomComps = protoBlackListedCustomComps.split("\\s");

            String protoBlackListedCustomCode = (inputParams.get("customCodeBlacklistProtos") != null ? 
                                                inputParams.get("customCodeBlacklistProtos").toString() : "").trim();
            if (protoBlackListedCustomCode.length() > 0)
               mProtoBlackListedCustomCode = protoBlackListedCustomCode.split("\\s");

        }
        : file_member* -> ^(FILE<HashToken>["FILE", new ExtHashtable().
                                put("InputFile", inputFile).
                                put("TopLevel", topLevel).
                                put("DefTable", new DefineHelper()).
                                put("InputParams", inputParams).
                                put("ProtoPackageName", mProtoPackageName).
                                put("MultipleNamespaces", mMultipleNamespaces).
                                put("HasNamespace", mClosedANamespace).
                                put("IsClient", mIsClient).
                                put("generateProto", mGenerateProtos).
                                put("protoRootDir", mProtoRootDir)] file_member*) 
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
@init { 
                TdfParseOutput parseResult = null; 
                boolean fakeIncludeFile = false;
                boolean clientInclude = true;
                ExtHashtable attribs = null;
      }
 :
  attributes? INCLUDE filename=string_literal  {
      //TODO HACK - MAKE THIS AN ATTRIB
      fakeIncludeFile = !$filename.r.endsWith(".tdf");
 
      attribs = new ExtHashtable($attributes.attribs);
      attribs.putIfNotExists("generated", !fakeIncludeFile);
      attribs.putIfNotExists("client_include", true);

      clientInclude = (Boolean) attribs.get("client_include") || !mIsClient;                 

      if (!fakeIncludeFile && clientInclude)
      {
          try 
          { 
               parseResult = TdfComp.parseTdfFile($filename.r, null, false); 
               attribs.put("TokenStream", parseResult.tokens);
               attribs.put("packageName", parseResult.packageName);
               attribs.put("ResolvedFileName", parseResult.resolvedFileName);
               attribs.put("relativeProtoPath", parseResult.relativeProtoPath);
               attribs.put("relativeTdfPath", parseResult.relativeTdfPath);
               attribs.putIfNotExists("includeproto", includeProto(parseResult.relativeProtoPath, $attributes.attribs != null ? $attributes.attribs.get("includeProto") : null));
          } catch (Exception e) { e.printStackTrace();}
      }

      //Do this here instead of in the rewrite because our rewirte has a choice

      attribs.put("Filename", $filename.r);      
  }
  -> {fakeIncludeFile && clientInclude}? ^(FAKE_INCLUDE<HashToken>[$INCLUDE, attribs.put("headername", $filename.r)] attributes?)
  -> {clientInclude && parseResult.tree != null}? ^(INCLUDE<HashToken>[$INCLUDE, attribs] attributes? {parseResult.tree})
  -> {clientInclude}? ^(INCLUDE<HashToken>[$INCLUDE, attribs] attributes? )
  -> //Just drop it if its not a client file.
;       

  
namespace_block 
        @init
        {
            if (mClosedANamespace)
                mMultipleNamespaces = true;
        }
        @after 
        {
            mClosedANamespace = true;
        }
        : attributes? NAMESPACE SYMBOL
         '{' 
              {
                if (mProtoPackageName.isEmpty()) // Dealing with top level namespace. Use "eadp.blaze" if it is "Blaze"
                {
                    if (($SYMBOL.text).equals("Blaze"))
                        mProtoPackageName += "eadp.blaze";
                    else
                        mProtoPackageName += ($SYMBOL.text);
                }
                else
                {
                    mProtoPackageName += ".";
                    mProtoPackageName += ($SYMBOL.text);
                }
                
              }

              namespace_member*
         '}' -> 
          ^(NAMESPACE<TdfSymbol>[$NAMESPACE, $SYMBOL.text, TdfSymbol.Category.NAMESPACE,
                           new ExtHashtable($attributes.attribs)] attributes? namespace_member*)
        ;
        
        
namespace_member
        : namespace_block
        | type_definition_block
        | root_component_block
        | component_block
        | custom_attribute_block        
        ;
        
group_block
        scope{ String relativeOutputPath; }
        @init { $group_block::relativeOutputPath = ""; }
        : GROUP path=string_literal
        '{'
            {$group_block::relativeOutputPath = $path.r;} group_member*       
        '}' -> ^(GROUP<HashToken>["GROUP", new ExtHashtable().
                    put("RelativeOutputPath", TdfComp.getRelativeOutputDir(new File($path.r))).
                                put("InputFileName", new File($path.r).getName())] group_member*)
        ;

group_member
        : namespace_member
        | include_block
        ;
         
/* ================================================================================]
                                          TDF Types
   ================================================================================*/

        
type_definition_block
        : class_fwd_decl_block
        | class_block
        | typedef_block
        | bitfield_block
        | const_block
        | enum_block
        | union_block        
        ;             
        
class_fwd_decl_block
        : attributes? CLASS SYMBOL ';' 
                -> ^(CLASS_FWD_DECL<TdfSymbol>[$CLASS, $SYMBOL.text, TdfSymbol.Category.CLASS,
                    new ExtHashtable($attributes.attribs).put("IsFwdDecl", true)] attributes?)
        ;

custom_attribute_block
    : CUSTOM_ATTRIBUTE SYMBOL 
    '{' 
    custom_attribute_member*
    '}' ';' -> 
         ^(CUSTOM_ATTRIBUTE<TdfSymbol>[$CUSTOM_ATTRIBUTE, $SYMBOL.text, TdfSymbol.Category.CUSTOM_ATTRIBUTE, 
                         new ExtHashtable().
                                                put("Name", $SYMBOL.text)] 
         custom_attribute_member*)
     ;

custom_attribute_type
    : INT_PRIMITIVE 
    | CHAR_PRIMITIVE
    | STRING
    ;

custom_attribute_member
    : 
    custom_attribute_type SYMBOL ';' 
        -> ^(MEMBER_DEF<TdfSymbolHolder>[$SYMBOL, $SYMBOL.text, 
        TdfSymbol.Category.MEMBER, new ExtHashtable().put("Name", stripMemberName($SYMBOL.text))] custom_attribute_type)
        ;    

class_block       
        scope 
        { 
                int memberNumber;
        }
        @init { $class_block::memberNumber = 0; }        
        :
        attributes? CLASS SYMBOL
        '{'
          class_member* 
         '}' ';' ->  ^(CLASS<TdfSymbol>[$CLASS, $SYMBOL.text, TdfSymbol.Category.CLASS, 
                                new ExtHashtable($attributes.attribs).
                                put("IsUnion", false).
                                put("generateProto", generateProtoMsg($attributes.attribs != null ? $attributes.attribs.get("generateProto") : null))] attributes? class_member*)
        ;

class_member
        :       (attributes CONST? type SYMBOL  ';') => class_member_def
        |       type_definition_block
        ;
 
class_member_def
                @after { $class_block::memberNumber++; }
                :               attributes CONST? type SYMBOL  ';' 
                -> ^(MEMBER_DEF<TdfSymbolHolder>[$SYMBOL, $SYMBOL.text, TdfSymbol.Category.MEMBER, 
                                                new ExtHashtable($attributes.attribs).
                                                put("Name", stripMemberName($SYMBOL.text)).
                                                put("ParseName", genMemberParseName($SYMBOL.text, $attributes.attribs.get("name"))).
                                                put("TagValue", generateMemberTag($SYMBOL.text, $attributes.attribs.get("tag"))).
                                                putIfNotExists("ownsmem", true).
                                                put("IsConst", $CONST != null).
                                                putIfNotExists("reconfigValue",generateReconfigurableValue($attributes.attribs.get("reconfigurable"))).
                                                put("MemberNumber", $class_block::memberNumber).
                                                put("ProtobufTagValue", generateProtobufTagFromMemberTag($attributes.attribs.get("tag"))).
                                                put("UseProtobufWrapper", useProtobufWrapper($attributes.attribs.get("protoWrapper"))).
                                                put("PrintFormat", generatePrintFormat($attributes.attribs.get("printFormat")))] attributes? type)
                ;

        
union_block
        scope 
        { 
                int memberNumberOneBased;
        }
        @init { $union_block::memberNumberOneBased = 1; }
        :       attributes? UNION SYMBOL
                '{'
                        union_member* 
                '}' ';'  -> 
                ^(UNION<TdfSymbol>[$UNION, $SYMBOL.text, TdfSymbol.Category.UNION, 
                                                new ExtHashtable($attributes.attribs).
                                                put("IsUnion", true).
                                                put("generateProto", generateProtoMsg($attributes.attribs != null ? $attributes.attribs.get("generateProto") : null))] attributes? union_member*)                                                                      
        ;

union_member
        @after { $union_block::memberNumberOneBased++; }
        :       attributes? type SYMBOL  ';' -> ^(MEMBER_DEF<TdfSymbolHolder>[$SYMBOL, $SYMBOL.text, TdfSymbol.Category.MEMBER,
                                                new ExtHashtable($attributes.attribs).
                                                put("Name", stripMemberName($SYMBOL.text)).
                                                put("ParseName", genMemberParseName($SYMBOL.text, $attributes.attribs != null ? $attributes.attribs.get("name") : null)).
                                                put("TagValue", generateMemberTag($SYMBOL.text, $attributes.attribs != null && $attributes.attribs.get("tag") != null ? $attributes.attribs.get("tag") : "VALU")).
                                                put("PrintFormat", generatePrintFormat("normal")).
                                                put("ProtobufTagValue", generateProtobufTagForUnion($union_block::memberNumberOneBased, $attributes.attribs != null && $attributes.attribs.get("tag") != null ? $attributes.attribs.get("tag") : "VALU"))] attributes? type)
        ;
        

typedef_block
        :       attributes? TYPEDEF type SYMBOL ';'
                 -> ^(TYPEDEF<TdfSymbol>[$TYPEDEF, $SYMBOL.text,TdfSymbol.Category.TYPEDEF, 
                        new ExtHashtable($attributes.attribs) ] attributes? type)
        ;

        //TODO string type
const_block
        :       attributes? CONST (c='char8_t' '*' ss=SYMBOL '=' const_string_value | type si=SYMBOL '=' const_value) ';' 
                -> {$c != null}? ^(STRING_CONST<TdfSymbol>[$CONST,$ss.text,TdfSymbol.Category.CONST,
                                new ExtHashtable($attributes.attribs).
                                put("Value", $const_string_value.r)] attributes? const_string_value)                     
                -> ^(CONST<TdfSymbol>[$CONST,$si.text,TdfSymbol.Category.CONST,
                                new ExtHashtable($attributes.attribs).
                                put("Value", $const_value.r)] type const_value)

        ;

const_value returns [Object r]
        : base_literal { $r = $base_literal.r; }
        | MINMAX
        | scoped_name
        ;

const_string_value returns [String r]
        : string_literal { $r = $string_literal.r; }
        | scoped_name
        ;

bitfield_block
        scope 
        { 
                TdfSymbol blockType; 
                int bitStart;
                int memberNumberOneBased;
        }
        @init { $bitfield_block::bitStart = 0;
                $bitfield_block::memberNumberOneBased = 1;
            }
        :       attributes? BITFIELD SYMBOL
                '{' 
                        bitfield_member* 
                '}' ';' 
                 -> ^(BITFIELD<TdfSymbol>[$BITFIELD, $SYMBOL.text,TdfSymbol.Category.BITFIELD,
                        new ExtHashtable($attributes.attribs).put("generateProto", generateProtoMsg($attributes.attribs != null ? $attributes.attribs.get("generateProto") : null))] attributes? bitfield_member*)                    
        ;
        
bitfield_member
        @after { $bitfield_block::memberNumberOneBased++; }
        @init {int bitMask = 0; int bitSize = 1; int bitStart = $bitfield_block::bitStart;}
        :       attributes? SYMBOL (':' i=integer { bitSize = $i.value; })? ';'  
                {
                        bitMask = ((1 << bitSize) - 1) << bitStart;
                        $bitfield_block::bitStart += bitSize;
                }
                -> ^(MEMBER_DEF<HashToken>[$SYMBOL, 
                        new ExtHashtable($attributes.attribs).
                        put("Name", stripMemberName($SYMBOL.text)).
                        put("BitSize", bitSize).
                        put("Bool", bitSize == 1).
                        put("BitStart", bitStart).
                        put("BitMask", bitMask).
                        put("MemberNumberOneBased", $bitfield_block::memberNumberOneBased)] attributes?)
        ;
        
enum_block
        scope
        {
            int prevMemberNumber;
            int currMemberNumber;
        }
        @init { $enum_block::prevMemberNumber = -1; $enum_block::currMemberNumber = 0;}
        :       attributes? ENUM SYMBOL
                '{' 
                      enum_member (',' enum_member)* (',')?
                '}' ';' 
                -> ^(ENUM<TdfSymbol>[$ENUM, $SYMBOL.text, TdfSymbol.Category.ENUM, 
                        new ExtHashtable($attributes.attribs).
                        put("generateProto", generateProtoMsg($attributes.attribs != null ? $attributes.attribs.get("generateProto") : null)).
                        put("addProtoAlias", boolFromAttrib(($attributes.attribs != null ? $attributes.attribs.get("addProtoAlias") : null), false))] attributes? enum_member*)
        ;

enum_member
        :       attributes? SYMBOL ('=' integer)? {
                if ($integer.value != null)
                {
                    $enum_block::currMemberNumber = $integer.value;
                }
                else
                {
                    $enum_block::currMemberNumber = $enum_block::prevMemberNumber + 1;
                }
                $enum_block::prevMemberNumber =  $enum_block::currMemberNumber;
        }
                -> ^(MEMBER_DEF<TdfSymbol>[$SYMBOL, $SYMBOL.text, TdfSymbol.Category.ENUM_VALUE,
                        new ExtHashtable($attributes.attribs).
                        put("Value", $integer.value).
                        put("ValueIsZero", $integer.value != null ? $integer.value == 0 : true).
                        put("MemberNumber", $enum_block::currMemberNumber)] attributes?)
        ;


/* ================================================================================]
                                         RPC Component Definitions
   ================================================================================*/

root_component_block 
    : attributes? ROOTCOMPONENT 
      '{'
          root_component_members*
      '}' -> ^(ROOTCOMPONENT<HashToken>[$ROOTCOMPONENT, new ExtHashtable($attributes.attribs)] attributes? root_component_members*)
    ;
    
root_component_members
    : errors_block
    | sdk_errors_block
    | permissions_block
    | alloc_groups_block
    ;

component_block
        : attributes? COMPONENT SYMBOL
          '{'
                component_members[$SYMBOL.text]*
          '}'  
          -> ^(COMPONENT<TdfSymbol>[$COMPONENT, $SYMBOL.text, TdfSymbol.Category.COMPONENT,
                        new ExtHashtable($attributes.attribs).
                        put("Name", $SYMBOL.text).
                        put("RelativeOutputPath", $group_block.empty() ? "" : TdfComp.getRelativeOutputDir(new File($group_block::relativeOutputPath))).
                        put("InputFileName", $group_block.empty() ? "" : new File($group_block::relativeOutputPath).getName()).
                        putIfNotExists("factory_create", true).
                        putIfNotExists("setCurrentUserSession", true).
                        putIfNotExists("grpcOnly", false).
                        putIfNotExists("factory_create", true)] attributes? component_members*)
        ;
        
component_members [String componentName]
        : subcomponent_block[componentName]
        | errors_block
        | types_block
        | permissions_block
        | alloc_groups_block
        ;

subcomponent_block [String componentName]
        scope 
        {
           Boolean isMaster; 
        }
        : attributes? (s = SLAVE {$subcomponent_block::isMaster = false;} | m = MASTER {$subcomponent_block::isMaster = true;} ) 
          '{'
                 subcomponent_members*
          '}' 
          -> {$s != null}? ^(SLAVE<TdfSymbol>[$SLAVE, $componentName + "Slave", TdfSymbol.Category.SUBCOMPONENT,
                   new ExtHashtable($attributes.attribs).
                   put("IsSlave", true).
                   putIfNotExists("requiresMaster", true).
                   put("RelativeOutputPath", $group_block.empty() ? "" : TdfComp.getRelativeOutputDir(new File($group_block::relativeOutputPath))).
                   put("InputFileName", $group_block.empty() ? "" : new File($group_block::relativeOutputPath).getName()).
                   put("Type", "Slave").
                   putIfNotExists("shouldAutoRegisterForMasterReplication", true).
                   putIfNotExists("shouldAutoRegisterForMasterNotifications", true).
                   put("generateProto", generateProtoService($attributes.attribs != null ? $attributes.attribs.get("generateProto") : null, false))] attributes? subcomponent_members*)
          -> ^(MASTER<TdfSymbol>[$MASTER, $componentName + "Master", TdfSymbol.Category.SUBCOMPONENT,
                   new ExtHashtable($attributes.attribs).
                   put("IsMaster", true).
                   put("RelativeOutputPath", $group_block.empty() ? "" : TdfComp.getRelativeOutputDir(new File($group_block::relativeOutputPath))).
                   put("InputFileName", $group_block.empty() ? "" : new File($group_block::relativeOutputPath).getName()).
                   put("Type", "Master").
                   put("generateProto", generateProtoService($attributes.attribs != null ? $attributes.attribs.get("generateProto") : null, true))] attributes? subcomponent_members*)              
        ;
        
subcomponent_members
        : commands_block
        | notifications_block
        | events_block
        | replication_block
        ;
        
        
commands_block
        : METHODS
         '{'
                command_def_block*
         '}' -> command_def_block*
        ;
        
command_def_block
        @init { boolean clientExport = true; boolean internal = false; }
        : attributes (respvoid=VOID | (respstream=STREAM)? resptype=typename) name=SYMBOL '(' ((reqstream=STREAM)? reqtype=typename)? ')' ';'  
          {
             $attributes.attribs.putIfNotExists("client_export", true);
             $attributes.attribs.putIfNotExists("internal", false);
             $attributes.attribs.putIfNotExists("obfuscate_platform_info", false);
             if ((Boolean) $attributes.attribs.get("obfuscate_platform_info") && $attributes.attribs.get("passthrough") != null)
                 throw new IllegalArgumentException("Invalid RPC definition for '" + $name.text + "': obfuscate_platform_info is not supported for passthrough RPCs");
             $attributes.attribs.put("generateProto", generateProtoRpc($attributes.attribs != null ? $attributes.attribs.get("generateProto") : null, $subcomponent_block::isMaster));
             clientExport = (Boolean) $attributes.attribs.get("client_export");
             internal = (Boolean) $attributes.attribs.get("internal");
          }      
          -> { clientExport || !mIsClient }? ^(COMMAND<TdfSymbol>[$name, $name.text, TdfSymbol.Category.COMMAND,
                $attributes.attribs.
                putIfNotExists("requires_authentication", true).
                putIfNotExists("requiresUserSession", true).
                putIfNotExists("setCurrentUserSession", true).
                putIfNotExists("reset_idle_timer", true).
                putIfNotExists("generate_command_class", !$subcomponent_block::isMaster && ($attributes.attribs.get("passthrough") == null)).
                put("IsServerStreaming", $respstream != null).
                put("IsClientStreaming", $reqstream != null).
                put("IsBidirectionalStreaming", $respstream != null && $reqstream != null)] attributes? $respvoid? $resptype? $reqtype?)
          -> //If this is the client and we don't export, then do nothing here.
        ;
        
notifications_block
        : NOTIFICATIONS
         '{'
                notification_def_block*
         '}' -> notification_def_block*
        ;
        

notification_def_block
        @init { boolean clientExport = true; }
        : attributes? name=SYMBOL '(' notiftype=typename? ')' ';'
          {
             $attributes.attribs.putIfNotExists("client_export", !$subcomponent_block::isMaster);
             $attributes.attribs.putIfNotExists("obfuscate_platform_info", false);
             $attributes.attribs.put("generateProto", generateProtoRpc($attributes.attribs != null ? $attributes.attribs.get("generateProto") : null, $subcomponent_block::isMaster));
             $attributes.attribs.putIfNotExists("protobuf_tag", generateProtobufTagFromNotificationId((int)  $attributes.attribs.get("id")));
             clientExport = (Boolean) $attributes.attribs.get("client_export");
          }  
           -> { clientExport || !mIsClient }? ^(NOTIFICATION<TdfSymbol>[$name, $name.text, TdfSymbol.Category.NOTIFICATION,
                 $attributes.attribs] attributes? $notiftype?)
           -> //If this is the client and we don't export, then do nothing here.
        ;
                

events_block
        : EVENTS
         '{'
             event_def_block* 
         '}' -> { !mIsClient }?  event_def_block*
          -> //If this is the client we ignore the events block
        ;
        
event_def_block
        : attributes? name=SYMBOL '(' eventtype=typename? ')' ';'
           -> ^(EVENT<TdfSymbol>[$name, $name.text, TdfSymbol.Category.EVENT,
                 $attributes.attribs.put("generateProto", generateProtoEvent($attributes.attribs != null ? $attributes.attribs.get("generateProto") : null))] attributes? typename?)
        ;
        
replication_block
        : REPLICATION
         '{'
                replication_def_block*
         '}' -> { !mIsClient }? replication_def_block*
         -> //If this is the client we ignore the replication block
        ;
        
replication_def_block
        : attributes? (d=DYNAMIC_MAP | s=STATIC_MAP) name=SYMBOL ';'
             -> {$d != null}? ^(DYNAMIC_MAP<TdfSymbol>[$DYNAMIC_MAP, $name.text, TdfSymbol.Category.REPLICATEDMAP,
                $attributes.attribs.
                put("Dynamic", true)] attributes?)
             -> ^(STATIC_MAP<TdfSymbol>[$STATIC_MAP, $name.text, TdfSymbol.Category.REPLICATEDMAP,
                $attributes.attribs.
                put("Static", false)] attributes?)
        
        ;
          
errors_block
        : ERRORS '{' error_def_block* '}' -> ^(ERRORS error_def_block*)
        ;
        

sdk_errors_block
        : SDK_ERRORS '{' error_def_block* '}' -> ^(SDK_ERRORS error_def_block*)
        ;

error_def_block
        : attributes? SYMBOL '=' integer';'
           -> ^(ERROR<TdfSymbol>[$SYMBOL, $SYMBOL.text, TdfSymbol.Category.ERROR, new ExtHashtable().put($attributes.attribs).
                put("Name", $SYMBOL.text).
                put("Value", $integer.value)] attributes?)
        ;
    
permissions_block
                : PERMISSIONS '{' permissions_def_block* '}' -> ^(PERMISSIONS permissions_def_block*)
                ;
        
permissions_def_block
        : attributes? SYMBOL '=' integer';'
           -> ^(PERMISSION<HashToken>[$SYMBOL, new ExtHashtable().put($attributes.attribs).
                        put("Name", $SYMBOL.text).
                put("Value", $integer.value)] attributes?)
        ;

alloc_groups_block
                : ALLOC_GROUPS '{' alloc_groups_def_block* '}' -> ^(ALLOC_GROUPS alloc_groups_def_block*)
                ;
        
alloc_groups_def_block
        : attributes? SYMBOL '=' integer';'
           -> ^(ALLOC_GROUP<HashToken>[$SYMBOL, new ExtHashtable().put($attributes.attribs).
                        put("Name", $SYMBOL.text).
                put("Value", $integer.value)] attributes?)
        ;
        
types_block
        : TYPES
          '{'
              type_def_block*
          '}'
           -> type_def_block*
        ;
        
type_def_block
        : attributes? SYMBOL '=' integer  ';'
            -> ^(TYPE<HashToken>[$SYMBOL, new ExtHashtable().put($attributes.attribs).
                  put("Name", $SYMBOL.text).
                        put("Value", $integer.value).
                  putIfNotExists("has_identity", true)] attributes?)
        ;



/* ================================================================================]
                                          Types
   ================================================================================*/
type    
        :       collection_type
        |       non_collection_type
        ;

non_collection_type 
        :       INT_PRIMITIVE
        |       CHAR_PRIMITIVE
        |       FLOAT_PRIMITIVE     
        |       VARIABLE
        |       GENERIC
        |       BLOB
        |       OBJECT_TYPE
        |       OBJECT_ID
        |       OBJECT_PRIMITIVE
        |       TIMEVALUE
        |       string_type
        |       typename
        ;
        
    
typename returns [TdfSymbolName name]
    : scoped_name { $name = $scoped_name.name; } -> ^(TYPE_REF scoped_name)
        ;
        
string_type
        scope { TdfSymbol newType; }
        :       STRING '(' integer ')'     -> ^(STRING integer)
        |       STRING '(' scoped_name ')' -> ^(STRING scoped_name)
        ;
        
collection_type
        :       LIST '<' type  (',' (scoped_name | integer) ',' bool )? '>' 
                 -> ^(LIST<HashToken>[$LIST, new ExtHashtable().
                                put("ReserveSizeValue", $integer.value).
                                put("FillList", $bool.value)] type scoped_name?)
        |       MAP '<' non_collection_type ',' type (',' SYMBOL)? '>' -> ^(MAP non_collection_type type SYMBOL?)
        ;

/* ================================================================================]
                                         Attributes
   ================================================================================*/
        
attributes returns [ExtHashtable attribs]
        @init { 
                $attribs = new ExtHashtable(); 
        }
        :       '[' 
        a=attribute[$attribs] (',' b=attribute[$attribs] )* 
        ']'      
        -> ^(ATTRIBS attribute+)                                                                     
        ;

attribute [ExtHashtable attribs]
        : ap=attribute_pair { $attribs.put($ap.key, $ap.value); }
        | al=attribute_list { $attribs.put($al.key, $al.value); } 
    ;

attribute_list returns [String key, ArrayList<String> value]
        @init { $value = new ArrayList<String>(); }
        :       k=attrib_key {$key=$k.text; } '=' '{' (s1=SYMBOL { $value.add($s1.text); }(',' s2=SYMBOL { $value.add($s2.text); } )* )? '}'
        -> ^(ATTRIB_LIST attrib_key SYMBOL*)
        ;

attribute_pair returns [Object key, Object value]
        :      attrib_key '=' attrib_value {$key = $attrib_key.k; $value=$attrib_value.v; }
        -> {!mIsClient || !($key.equals("configurationType") || $key.equals("preconfigurationType"))}? ^(ATTRIB_PAIR attrib_key attrib_value)
        -> //configurationType & preconfigurationType are only used on the server
        ;

attribute_map returns [ExtHashtable value]
        @init { $value = new ExtHashtable(); }
        :  '{' (s1=attribute_pair { $value.put($s1.key, $s1.value); }(',' s2=attribute_pair { $value.put($s2.key, $s2.value); } )* ) '}'
           -> ^(ATTRIB_MAP attribute_pair+)
        ;

attrib_key returns [Object k]
        : SYMBOL {$k = $SYMBOL.text;} 
        | ERRORS {$k = $ERRORS.text;} 
        | string_literal { $k = $string_literal.r;} 
        | integer { $k = $integer.value;}        
        ;

attrib_value returns [Object v]
        : literal { $v = $literal.r; }
        | typename { $v = $typename.name; }
        | attribute_map { $v = $attribute_map.value; }
        ;

/* ================================================================================
                                          Literals
   ================================================================================*/
literal returns [Object r] :    base_literal   { $r = $base_literal.r; }
                           |    minmax_literal { $r = $minmax_literal.r; } 
                           |    string_literal { $r = $string_literal.r; }
                           |    type_pair      { $r = $type_pair.r; }
                           ;
                           
base_literal returns [Object r] :    integer        { $r = $integer.value;}
                           |    bool           { $r = $bool.value; }
                           |    floatingpoint  { $r = $floatingpoint.value; }
                           ;


minmax_literal returns [String r] :MINMAX {$r = $MINMAX.text; } 
        ;

string_literal returns [String r] :s=STRING_LITERAL {$r = $s.text.substring(0, $s.text.length() - 1).substring(1); }  //strip quotes
        ;

scoped_name returns [TdfSymbolName name]
        @init { $name = new TdfSymbolName(); }
        :       (s1=SYMBOL { $name.add($s1.text); } '::')* s2=SYMBOL { $name.add($s2.text); } -> ^(SYMBOL)+
        ;

integer returns [Integer value]
        :       DECIMAL { $value = Integer.parseInt($DECIMAL.text); }
        |       HEX     { $value = Integer.parseInt($HEX.text.substring(2), 16); }
        ;     

floatingpoint returns [Float value]
        :       FLOAT { $value = Float.parseFloat($FLOAT.text); }
        ;            
        
bool returns [Boolean value]
        : 'true' { $value = true; } -> BOOLEAN
        | 'false' { $value = false; } -> BOOLEAN
        ;

type_pair returns [List<String> r] :   s=SYMBOL '/' i=DECIMAL 
        {
          $r = new ArrayList<String>();
          $r.add($s.text);
          $r.add($i.text);
        } -> TYPE_PAIR
        ;

/* ================================================================================
                                          Lexer Rules
   ================================================================================*/


COMMENT :        '/*' .* '*/' { $channel = HIDDEN; } 
        |        '//' ( ~'\n')* '\n'? { $channel = HIDDEN; }
        ;

STRING_LITERAL
        :       '"' ( options {greedy=false;} : .)* '"';

INT_PRIMITIVE
        :       'bool' |
                'int8_t' | 'uint8_t' | 'int16_t' | 'uint16_t' |
                'int32_t' | 'uint32_t' | 'int64_t' | 'uint64_t'
    ;
    
CHAR_PRIMITIVE
        :       'char8_t'
    ;

FLOAT_PRIMITIVE
    :       'float'
    ;

        
MINMAX
        : 'INT'('8'|'16'|'32'|'64')'_'('MIN'|'MAX')
        | 'UINT'('8'|'16'|'32'|'64')'_MAX'
        | 'FLT_'('MIN'|'MAX')
        ;
        
INCLUDE :       '#include';
NAMESPACE 
        :       'namespace';
CLASS   :       'class';
UNION   :       'union';
TYPEDEF :       'typedef';
BITFIELD :      'bitfield';
CONST   :       'const';
STRING  :       'string';
LIST    :       'list';
MAP     :       'map';
ENUM    :       'enum';
VARIABLE 
        :       'variable';
BLOB    :       'blob' | 'TdfBlob';
GENERIC :       'generic';

OBJECT_TYPE
        :       'ObjectType';
OBJECT_ID         
        :       'ObjectId';
OBJECT_PRIMITIVE  
        :       'ComponentId' | 'EntityType' | 'EntityId';
TIMEVALUE
        :       'TimeValue';
GROUP           :       'group';
ROOTCOMPONENT   :       'rootcomponent';
COMPONENT       :       'component';
ERRORS          :       'errors';
SDK_ERRORS      :       'sdk_errors';
SLAVE           :       'slave';
MASTER          :       'master';
METHODS         :       'methods';
NOTIFICATIONS   :       'notifications';
EVENTS          :       'events';
REPLICATION     :       'replication';
DYNAMIC_MAP     :       'dynamic_map';
STATIC_MAP      :       'static_map';
VOID            :       'void';
STREAM          :       'stream';
TYPES           :       'types';
PERMISSIONS     :       'permissions';
ALLOC_GROUPS    :       'alloc_groups';

CUSTOM_ATTRIBUTE
    :    'attribute';

SYMBOL  :       ('a'..'z'|'A'..'Z'|'_')('a'..'z'|'A'..'Z'|'0'..'9'|'_')*;



DECIMAL
        :       ('-')?('1'..'9')('0'..'9')*
        |       '0'
        ;

HEX     :       '0x'('0'..'9'|'A'..'F'|'a'..'f')* ;

FLOAT   :        ('-')?('0'..'9')+(('.')('0'..'9')+)?('f')?;

/* $channel = HIDDEN just makes white space handling implicit */
WHITESPACE : ( '\t' | ' ' | '\r' | '\n'| '\u000C' )+    { $channel = HIDDEN; } ;
