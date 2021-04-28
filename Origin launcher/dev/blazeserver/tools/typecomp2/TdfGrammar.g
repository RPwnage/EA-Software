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
}

@header
{
import java.util.*;
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

@members
{

    public void displayRecognitionError(String[] tokenNames, RecognitionException e)
    {
         String hdr = getErrorHeader(e);
         String msg = getErrorMessage(e, tokenNames);
         emitErrorMessage(getSourceName() + ": " + hdr+" "+msg);
    
         throw new IllegalArgumentException("Parser err - stopping compilation.");
    }
        

    String stripMemberName(String name)
    {
        String result = name;
                
        //Get rid of the M
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
        
    static int TAG_CHAR_MIN = 32;
            
    static int generateMemberTag(String memberName, Object tag)
    {
        if (tag == null)
            throw new NullPointerException("Member " + memberName + " has no tag.");      

        String stringTag = tag.toString().toUpperCase();

        if (stringTag.length() > 4)
            throw new NullPointerException("Member " + memberName + " has a tag (" + stringTag + ") exceeding the maximum length of 4 characters");
        
        int intTag = ((stringTag.charAt(0) - TAG_CHAR_MIN) & 0x3f) << 26;
        if (stringTag.length() > 1)
        {
            intTag |= (((stringTag.charAt(1) - TAG_CHAR_MIN) & 0x3f) << 20);
            if (stringTag.length() > 2)
            {
                intTag |= (((stringTag.charAt(2) - TAG_CHAR_MIN) & 0x3f) << 14);
                if (stringTag.length() > 3)
                {
                    intTag |= ((stringTag.charAt(3) - TAG_CHAR_MIN) & 0x3f) << 8;
                }
            }
        }
        return intTag;
    }
    
    static String generateReconfigurableValue(Object reconfigurable)
    {
        if (reconfigurable != null)
        {
            String reconfigString = reconfigurable.toString().toLowerCase();
            if (reconfigString.equals("yes") || reconfigString.equals("true"))
            {
                return "TdfTagInfo::RECONFIGURE_YES";
            }
            else if (reconfigString.equals("no") || reconfigString.equals("false"))
            {
                return "TdfTagInfo::RECONFIGURE_NO";
            }
            else if (reconfigString.equals("default"))
            {
                return "TdfTagInfo::RECONFIGURE_DEFAULT";
            }
            else
            {            
                throw new IllegalArgumentException("Invalid value specified for 'reconfigurable' attribute: " + reconfigString);
            }
        }

        return "TdfTagInfo::RECONFIGURE_DEFAULT";
    }

    static String generatePrintFormat(Object printFormat)
    {
        if (printFormat != null)
        {
            String formatString = printFormat.toString().toLowerCase();
            if (formatString.equals("normal"))
            {
                return "TdfTagInfo::NORMAL";
            }
            else if (formatString.equals("censor"))
            {
                return "TdfTagInfo::CENSOR";
            }
            else if (formatString.equals("hash"))
            {
                return "TdfTagInfo::HASH";
            }
            else
            {            
                throw new IllegalArgumentException("Invalid value specified for 'printFormat' attribute: " + formatString);
            }
        }

        return "TdfTagInfo::NORMAL";
    }

    boolean mIsClient;
}

 
/* ================================================================================
                                          Highest Level 
   ================================================================================*/
file[String inputFile, String outputFileBase,boolean topLevel, boolean isClient, String headerPrefix]
        @init { mIsClient = isClient; }
        : file_member* -> ^(FILE<HashToken>["FILE", new ExtHashtable().
                                put("InputFile", inputFile).
                                put("OutputFileBase", outputFileBase).
                                put("TopLevel", topLevel).
                                put("HeaderPrefix", headerPrefix)] file_member*)
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
        : attributes? NAMESPACE SYMBOL
         '{' 
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
            {$group_block::relativeOutputPath = path.r;} group_member*       
        '}' -> ^(GROUP group_member*)
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
                                $attributes.attribs] attributes? class_member*)
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
                                                put("PrintFormat", generatePrintFormat($attributes.attribs.get("printFormat")))] attributes? type)
                ;

        
union_block
        :       attributes? UNION SYMBOL
                '{'
                        union_member* 
                '}' ';'  -> 
                ^(UNION<TdfSymbol>[$UNION, $SYMBOL.text, TdfSymbol.Category.UNION, 
                                				new ExtHashtable($attributes.attribs)] attributes? union_member*)                                                                      
        ;

union_member
        :       attributes? type SYMBOL  ';' -> ^(MEMBER_DEF<TdfSymbolHolder>[$SYMBOL, $SYMBOL.text, TdfSymbol.Category.MEMBER,
                                               new ExtHashtable($attributes.attribs).
                                                put("Name", stripMemberName($SYMBOL.text)).
                                                put("TagValue", generateMemberTag("", "VALU")).
                                                put("PrintFormat", generatePrintFormat("normal"))] attributes? type)
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

const_value returns [Integer r]
        : integer { $r = $integer.value; }
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
        }
        @init { $bitfield_block::bitStart = 0; }
        :       attributes? BITFIELD SYMBOL
                '{' 
                        bitfield_member* 
                '}' ';' 
                 -> ^(BITFIELD<TdfSymbol>[$BITFIELD, $SYMBOL.text,TdfSymbol.Category.BITFIELD,
                        new ExtHashtable($attributes.attribs) ] attributes? bitfield_member*)                    
        ;
        
bitfield_member
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
                        put("BitMask", bitMask)] attributes?)
        ;
        
enum_block

        :       attributes? ENUM SYMBOL
                '{' 
                      enum_member (',' enum_member)* (',')?
                '}' ';' 
                -> ^(ENUM<TdfSymbol>[$ENUM, $SYMBOL.text, TdfSymbol.Category.ENUM, 
                        new ExtHashtable($attributes.attribs)] attributes? enum_member*)
        ;

enum_member
        :       attributes? SYMBOL ('=' integer)? 
                -> ^(MEMBER_DEF<TdfSymbol>[$SYMBOL, $SYMBOL.text, TdfSymbol.Category.ENUM_VALUE,
                        new ExtHashtable($attributes.attribs).
                        put("Value", $integer.value)] attributes?)
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
	;

component_block
        : attributes? COMPONENT SYMBOL
          '{'
                component_members[$SYMBOL.text]*
          '}'  
          -> ^(COMPONENT<TdfSymbol>[$COMPONENT, $SYMBOL.text, TdfSymbol.Category.COMPONENT,
                        new ExtHashtable($attributes.attribs).
                        put("Name", $SYMBOL.text). 
                        putIfNotExists("factory_create", true).
                        putIfNotExists("proxyOnly", false).
                        putIfNotExists("exposeRawConfig", ($attributes.attribs.get("configurationType") == null))] attributes? component_members*)
        ;
        
component_members [String componentName]
        : subcomponent_block[componentName]
        | errors_block
        | types_block
        | permissions_block
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
                   put("RelativeOutputPath", $group_block.empty() ? "" : $group_block::relativeOutputPath).
                   put("Type", "Slave").
                   putIfNotExists("shouldAutoRegisterForMasterReplication", true).
                   putIfNotExists("shouldAutoRegisterForMasterNotifications", true)] attributes? subcomponent_members*)
          -> ^(MASTER<TdfSymbol>[$MASTER, $componentName + "Master", TdfSymbol.Category.SUBCOMPONENT,
                        new ExtHashtable($attributes.attribs).
                   put("IsMaster", true).
                   put("RelativeOutputPath", $group_block.empty() ? "" : $group_block::relativeOutputPath).
                   put("Type", "Master")] attributes? subcomponent_members*)              
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
        : attributes (respvoid=VOID | resptype=typename) name=SYMBOL '(' (reqtype=typename)? ')' ';'  
          {
             $attributes.attribs.putIfNotExists("client_export", true);
             $attributes.attribs.putIfNotExists("internal", false);
             clientExport = (Boolean) $attributes.attribs.get("client_export");
             internal = (Boolean) $attributes.attribs.get("internal");
          }      
          -> { clientExport || !mIsClient }? ^(COMMAND<TdfSymbol>[$name, $name.text, TdfSymbol.Category.COMMAND,
                $attributes.attribs.
                putIfNotExists("requires_authentication", true).
                putIfNotExists("requiresUserSession", true).
                putIfNotExists("setCurrentUserSession", true).
                putIfNotExists("reset_idle_timer", true).
                putIfNotExists("generate_command_class", !$subcomponent_block::isMaster && ($attributes.attribs.get("passthrough") == null))] attributes? $respvoid? $resptype?  $reqtype?)
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
                 $attributes.attribs] attributes? typename?)
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
        |       BLOB
        |       BLAZE_OBJECT_TYPE
        |       BLAZE_OBJECT_ID
        |       BLAZE_OBJECT_PRIMITIVE
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
        -> {!mIsClient || !$key.equals("configurationType")}? ^(ATTRIB_PAIR attrib_key attrib_value)
        -> //configurationType is only used on the server
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
literal returns [Object r] :    integer        { $r = $integer.value;}
                           |    bool           { $r = $bool.value; }
                           |    floatingpoint  { $r = $floatingpoint.value; }
                           |    minmax_literal { $r = $minmax_literal.r; } 
                           |    string_literal { $r = $string_literal.r; }
                           |    type_pair      { $r = $type_pair.r; }
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
        :       'bool' | 'bool8_t' |
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

BLAZE_OBJECT_TYPE
        :       'BlazeObjectType';
BLAZE_OBJECT_ID         
        :       'BlazeObjectId';
BLAZE_OBJECT_PRIMITIVE  
        :       'BlazeId' | 'ComponentId' | 'EntityType' | 'EntityId';
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
TYPES           :       'types';
PERMISSIONS	:       'permissions';

CUSTOM_ATTRIBUTE
	:	'attribute';

SYMBOL  :       ('a'..'z'|'A'..'Z'|'_')('a'..'z'|'A'..'Z'|'0'..'9'|'_')*;



DECIMAL
        :       ('-')?('1'..'9')('0'..'9')*
        |       '0'
        ;

HEX     :       '0x'('0'..'9'|'A'..'F'|'a'..'f')* ;

FLOAT   :        ('-')?('0'..'9')+(('.')('0'..'9')+)?('f')?;

/* $channel = HIDDEN just makes white space handling implicit */
WHITESPACE : ( '\t' | ' ' | '\r' | '\n'| '\u000C' )+    { $channel = HIDDEN; } ;
