grammar Rpc2TdfGrammar;

/* ================================================================================
                                          Options 
   ================================================================================*/
options
{
        language='Java';
        ASTLabelType=HashTree;
        output=template;
        rewrite=true;
}


@header
{
import java.util.*;
import org.antlr.runtime.BitSet;
}

@members
{
    	String getWhitespace(Token startToken)
    	{
    	   StringBuilder result = new StringBuilder();
    	   int index = startToken.getTokenIndex() - 1;
    	   Token t = getTokenStream().get(index);
    	   while (t.getChannel() == Token.HIDDEN_CHANNEL && 
    			   t.getType() == TABORSPACE)
    	   {
    		   result.insert(0, t.getText());
    		   ((TokenRewriteStream) getTokenStream()).delete(index);
    		   t = getTokenStream().get(--index);
    	   }
    	       	   
    	   
    	   return result.toString();
    	}
    	
    	class IncludeHelper
    	{
    	   public String Filename;
    	   public ArrayList Attribs;
    	}
        
        HashMap mIncludeFiles;
        	
    	void addIncludeFile(String fileName, ArrayList attribs)
    	{
     	   //Normalize the filename
     	   fileName = fileName.replace('\\', '/');

           if (!mIncludeFiles.containsKey(fileName))
           {
	           IncludeHelper helper = new IncludeHelper();
	    	   
	    	   helper.Filename = fileName;
	    	   helper.Attribs = attribs != null ? attribs : new ArrayList();
	    	   if (fileName.contains("/tdf/"))
	    	   {
	    		   //This is a TDF file - just hack off the .h and replace it 
	    		   helper.Filename = helper.Filename.replace("/tdf/", "/gen/");
	    		   helper.Filename = helper.Filename.replace(".h", ".tdf");
	    		   helper.Attribs.add("headername=" + fileName);
	    	   }
	    	   
               mIncludeFiles.put(fileName,helper);
           }	   
    	}
}
 
/* ================================================================================
                                          Highest Level 
   ================================================================================*/   
/* ================================================================================
                                          Main Elements
   ================================================================================*/

file
	scope { ArrayList attribs; }
	@init 
	{
		$file::attribs = new ArrayList(); 
		ArrayList comps = new ArrayList();
		mIncludeFiles = new HashMap();
	}
        :  (file_header_members)* (component_block {comps.add($component_block.st); })*
        -> template(includes={mIncludeFiles.values()},cb={comps}) <<
<includes:{<if(it.Attribs)>[<if(rest(it.Attribs))><\n> <endif> <it.Attribs; separator=",\n  "> <if(rest(it.Attribs))><\n><endif>]<\n><endif>#include <it.Filename>}; separator="\n\n">

<cb>
>>
        ;
        
        
file_header_members
        : attribute_pair
        | errors_block
        | sdk_errors_block
        ;

component_block
	scope { ArrayList attribs; List<String> namespaceList; }
	@init {$component_block::attribs = new ArrayList(); }
        : COMPONENT SYMBOL
          '{'
                component_members*
          '}'
          -> template(ns={$component_block::namespaceList}, attr={$component_block::attribs}, text={$text}) "<ns:{namespace <it><\n>\{<\n>}><if(attr)>[<if(rest(attr))><\n> <endif> <attr; separator={,<\n>  }> <if(rest(attr))><\n><endif>]<\n><endif><text><\n><ns:{\}<\n>}>"
        ;
        
component_members
        : attribute_pair  { $component_block::attribs.add($attribute_pair.st); } ->
        | namespace_pair { $component_block::namespaceList = $namespace_pair.nameList; }  -> 
        | subcomponent_block
        | errors_block
        | sdk_errors_block
        | types_block
        ;
        
subcomponent_block
	scope { ArrayList attribs; }
	@init {$subcomponent_block::attribs = new ArrayList(); String ws; }
        : SLAVE  {ws = getWhitespace($SLAVE); }
          '{'
                subcomponent_members*
          '}'
           -> template(ws={ws},attr={$subcomponent_block::attribs}, text={$text}) "<if(attr)><ws>[<if(rest(attr))><\n><ws> <endif> <attr; separator={,<\n><ws>  }> <if(rest(attr))><\n><ws><endif>]<\n><endif><ws><text>"
        | MASTER {ws = getWhitespace($MASTER); } 
          '{'
              subcomponent_members*
          '}'
            -> template(ws={ws},attr={$subcomponent_block::attribs}, text={$text}) "<if(attr)><ws>[<if(rest(attr))><\n><ws> <endif> <attr; separator={,<\n><ws>  }> <if(rest(attr))><\n><ws><endif>]<\n><endif><ws><text>"
        ;
        
subcomponent_members
        : attribute_pair { $subcomponent_block::attribs.add($attribute_pair.st); } ->
        | include_def
        | commands_block
        | notifications_block
        | events_block
        | replication_block
        ;
        
include_def

        : INCLUDE string_literal attributes[""]? ';' { addIncludeFile($string_literal.text, $attributes.attribs); } ->
        ;
        
commands_block
        : METHODS
         '{'
                command_def_block*
         '}'
        ;
        
command_def_block
	@init {String ws; }
        : command_def_block_inner { ws = getWhitespace($command_def_block_inner.start); } attributes[ws]
        -> template(ws={ws},def={$command_def_block_inner.st},attr={$attributes.st}) "   <if(attr)><attr><\n><endif><ws><def>;"
        ;
        
command_def_block_inner 
	: (VOID | resptype=scoped_name) name=SYMBOL '(' (reqtype=scoped_name SYMBOL?)? ')' 
	  -> template(ret={resptype != null ? $resptype.text : "void"},name={$name.text},req={$reqtype.text}) "<ret> <name>(<req>)"
	;
        
notifications_block
        : NOTIFICATIONS
         '{'
                notification_def_block*
         '}' 
        ;
        

notification_def_block
	@init { String ws; }
        : notification_def_block_inner {ws = getWhitespace($notification_def_block_inner.start); } attributes[ws]   
        -> template(ws={ws},def={$notification_def_block_inner.text},attr={$attributes.st}) "<attr><\n><ws><def>;"        
        ;
 
notification_def_block_inner
        : name=SYMBOL '(' notiftype=scoped_name? ')'
          
        ;               

events_block
        : EVENTS
         '{'
             event_def_block* 
         '}'
        ;
        
event_def_block
        @init {String ws; }
        : event_def_block_inner { ws = getWhitespace($event_def_block_inner.start); } attributes[ws]    
        -> template(ws={ws},def={$event_def_block_inner.text},attr={$attributes.st}) "<attr><\n><ws><def>;"      
        ;
        
event_def_block_inner 
	:  name=SYMBOL '(' eventtype=scoped_name? ')'
	;       
        
replication_block
        : REPLICATION
         '{'
                replication_def_block*
         '}'
         ;
        
replication_def_block
        @init {String ws; }
        :  replication_def_block_inner { ws = getWhitespace($replication_def_block_inner.start); } attributes[ws]   
        -> template(ws={ws},def={$replication_def_block_inner.text},attr={$attributes.st}) "<attr><\n><ws><def>;"          
        ;
        
replication_def_block_inner  
	: DYNAMIC_MAP name=SYMBOL
	| STATIC_MAP name=SYMBOL
	;     
          
errors_block
        : ERRORS '{' error_def_block* '}' 
        ;
        

sdk_errors_block
        : SDK_ERRORS '{' error_def_block* '}'
        ;       
        
error_def_block
	@init { String ws; }
        : error_def_block_inner { ws = getWhitespace($error_def_block_inner.start); } attributes[ws]? ';'  
        -> template(ws={getWhitespace($error_def_block_inner.start)},def={$error_def_block_inner.text},attr={$attributes.st}) "<attr><\n><ws><def>;"          
        ;

error_def_block_inner
	: SYMBOL '=' integer
	;

        
types_block
        : TYPES
          '{'
              type_def_block*
          '}'       
        ;

        
type_def_block
        @init { String ws; }
        : type_def_block_inner {ws = getWhitespace($type_def_block_inner.start); } attributes[ws]? ';'  
        -> template(ws={ws},def={$type_def_block_inner.text},attr={$attributes.st}) "<attr><\n><ws><def>;"          
        ;

type_def_block_inner
	: SYMBOL '=' integer
	;

/* ================================================================================]
                                          Types
   ================================================================================*/
attributes [String ws] returns [ArrayList attribs]
	@init { $attribs = new ArrayList(); }
        : '{' 
            (   attribute_pair { $attribs.add($attribute_pair.st); }
            |   attribute_list { $attribs.add($attribute_list.text); }
            |   wal_def_block[ws]  { $attribs.add($wal_def_block.text); }
            )* 
          '}' -> template(ws={$ws},attr={$attribs}) "<ws>[<if(rest(attr))><\n><ws> <endif> <attr; separator={,<\n><ws>  }> <if(rest(attr))><\n><ws><endif>]"
          ;

namespace_pair returns [ List<String> nameList ]
	: NAMESPACE '=' scoped_name ';' { $nameList = $scoped_name.nameList; }
	;

attribute_pair 
        :       attrib_key '=' attrib_value ';' -> template(k={$attrib_key.text},v={$attrib_value.text}) "<k> = <v>"
        ;

attribute_list      
        :       k=attrib_key  attribute_list_inner -> template(k={$attrib_key.text},v={$attribute_list_inner.text}) "<k> = <v>"
        ;

attribute_list_inner
	: '{' (s1=SYMBOL (',' s2=SYMBOL )* )? '}'
        ;

attrib_key
        : SYMBOL | ERRORS;

attrib_value
        : literal | scoped_name
        ;

wal_def_block [String ws]
        @init { ArrayList items = new ArrayList(); } 
        : WAL '{'(    ap=wal_attrib_pair { items.add($ap.st); } )+ '}' -> template(ws={ws},items={items}) <<wal = {<\n><ws>          <items; separator={,<\n><ws>          }><\n><ws>        }>>
      ;


wal_attrib_pair 
        : SYMBOL '=' wal_attrib_value ';'  -> template(k={$SYMBOL.text},v={$wal_attrib_value.text}) "<k> = <v>"
        ;

wal_attrib_value
        : literal | SYMBOL 
        ;


/* ================================================================================
                                          Literals
   ================================================================================*/
literal returns [Object r] :    integer      
                           |    bool         
                           |    string_literal
                           |    type_pair     
                           ;

type_pair 
        :   s=SYMBOL '/' i=DECIMAL 
        ;


string_literal  : STRING_LITERAL 
        ;

scoped_name returns [List<String> nameList]
        @init { $nameList = new ArrayList<String>(); }
        :       (s1=SYMBOL { $nameList.add($s1.text); } '::')* s2=SYMBOL { $nameList.add($s2.text); } 
        ;

integer
        :       DECIMAL 
        |       HEX     
        ;       

bool 
        : 'true' 
        | 'false' 
        ;

/* ================================================================================
                                          Lexer Rules
   ================================================================================*/


COMMENT :        '/*' .* '*/' { $channel = HIDDEN; } 
        |        '//' .* '\n' { $channel = HIDDEN; }
        ;

INCLUDE         :       'include';
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
WAL             :       'wal';
NAMESPACE 	: 	'namespace';

SYMBOL  :       ('a'..'z'|'A'..'Z'|'_')('a'..'z'|'A'..'Z'|'0'..'9'|'_')*;



DECIMAL
        :       ('1'..'9')('0'..'9')*
        |       '0'
        ;

HEX     :       '0x'('0'..'9'|'A'..'F'|'a'..'f')* ;

STRING_LITERAL
        :       '"' ( options {greedy=false;} : .)* '"';


/* $channel = HIDDEN just makes white space handling implicit */
TABORSPACE 
	: ( '\t' | ' ' )+ { $channel = HIDDEN; } ;	 
WHITESPACE : ( '\r' | '\n'| '\u000C' )+    { $channel = HIDDEN; } ;
