tree grammar TdfTypeFixup;

/* ================================================================================
                                          Options 
   ================================================================================*/
options
{
        language='Java';
        tokenVocab=TdfGrammar;
        ASTLabelType=HashTree;
        output=AST;
        rewrite=true;
}


/* ================================================================================
                                          Members
   ================================================================================*/
scope SymbolScope
{
    TdfSymbol Scope;
}

  
@header
{
import java.util.*;
import org.antlr.runtime.BitSet;
} 

@members
{
    TdfSymbolTable mSymbols;
    
    void addToScope(HashTree tree, TdfSymbol.SymbolType type)
    {
       TdfSymbol symbol = (TdfSymbol) tree.getToken();
       int parentIndex = $SymbolScope.size() - 2;
       TdfSymbol scope = ($SymbolScope::Scope != null) ? $SymbolScope::Scope : $SymbolScope[parentIndex]::Scope;
       scope.addSymbol(symbol, type);
     }
        
    void pushScope(HashTree t) { pushScope(t.getHashToken()); }    
    void pushScope(HashToken t)
    {
            String scopeName = (String) t.get("Name");
            int parentIndex = $SymbolScope.size() - 2;
            $SymbolScope::Scope = $SymbolScope[parentIndex]::Scope.getSymbol(scopeName, TdfSymbol.SymbolType.TYPE);
    }

    Stack<TokenStream> mTokenStreamStack = new Stack<TokenStream>();
    void pushTokenStream(TokenStream stream)
    {
            mTokenStreamStack.push(stream);
            ((BufferedTreeNodeStream) getTreeNodeStream()).setTokenStream(stream);
    }
            
    void popTokenStream()
    {
            mTokenStreamStack.pop();
            ((BufferedTreeNodeStream) getTreeNodeStream()).setTokenStream(mTokenStreamStack.peek());
    }

}

/* ================================================================================
                                          Highest Level 
   ================================================================================*/
//TODO file params
file [TdfSymbolTable symbols]
        scope SymbolScope;
        @init 
        { 
                mSymbols = symbols; 
                $SymbolScope::Scope = mSymbols.getRootScope();
                pushTokenStream(getTreeNodeStream().getTokenStream());
        }
        :       file_def
        ;
        
/* ================================================================================
                                          Main Elements
   ================================================================================*/
file_def returns [HashToken r] 
        :       ^(FILE file_member*) { $r = $FILE.getHashToken(); }
        ;


file_member
        : namespace_member
        | include_block
        | group_block
        ;
        
include_block 
        : ^(INCLUDE attributes[$INCLUDE]? 
         {          
                TokenStream ts = (TokenStream) $INCLUDE.get("TokenStream");
                if (ts != null)
                {
                    pushTokenStream(ts); 
                }
         }
                file_def?
 
         { 
                if ($file_def.r != null)
                    popTokenStream(); 
         }
         )
         | ^(FAKE_INCLUDE attributes[$FAKE_INCLUDE]?) 
        ;

namespace_block 
        scope SymbolScope;       
        : ^(NAMESPACE attributes[$NAMESPACE]?
          {
               addToScope($NAMESPACE, TdfSymbol.SymbolType.TYPE);
               pushScope($NAMESPACE.getHashToken()); 
          } namespace_member*) 
        ;
        
namespace_member
        : namespace_block
        | type_definition_block
        ;
        
group_block
        : ^(GROUP group_member*)
        ;
        
group_member
        : namespace_member
        | include_block
        ;
        
type_definition_block
        : class_fwd_decl_block
        | class_block
        | typedef_block
        | bitfield_block
        | const_block
        | enum_block
        | union_block
        | root_component_block
    | component_block
    | custom_attribute_block
        ;


/* ================================================================================
                                         TDF Types
   ================================================================================*/

custom_attribute_block
    scope SymbolScope;
    : ^(CUSTOM_ATTRIBUTE     
                {
                        //Push the current scope for type resolution, add a members field attribute
                        addToScope($CUSTOM_ATTRIBUTE, TdfSymbol.SymbolType.TYPE);
                        pushScope($CUSTOM_ATTRIBUTE.getHashToken()); 
                       
                }    
    custom_attribute_member*)
     ;

custom_attribute_member
    : ^(MEMBER_DEF custom_attribute_type)
                {
                        addToScope($MEMBER_DEF, TdfSymbol.SymbolType.TYPE);
                }    
    ;
    
custom_attribute_type
    : INT_PRIMITIVE 
    | CHAR_PRIMITIVE
    | STRING
    ;

class_fwd_decl_block
        : ^(CLASS_FWD_DECL attributes[$CLASS_FWD_DECL]?) { addToScope($CLASS_FWD_DECL, TdfSymbol.SymbolType.TYPE); } 
        ;
        
class_block
        scope SymbolScope;
        : ^(CLASS 
                {
                        //Push the current scope for type resolution, add a members field attribute
                        addToScope($CLASS, TdfSymbol.SymbolType.TYPE);
                        pushScope($CLASS.getHashToken()); 
                       
                }
                attributes[$CLASS]? class_members*)                 
        ;
        
class_members 
    :        class_member
    |        type_definition_block
    ;
    

class_member
        :   ^(md=MEMBER_DEF attributes[$MEMBER_DEF]? type) 
                {
                        addToScope($MEMBER_DEF, TdfSymbol.SymbolType.TYPE);
                } 
        ;
        
union_block
        scope SymbolScope;       
        :       ^(UNION 
              {
                        //Push the current scope for type resolution, add a members field attribute
                        addToScope($UNION, TdfSymbol.SymbolType.TYPE);
                        pushScope($UNION.getHashToken()); 
              }
              attributes[$UNION]? union_member*) 
        ;

union_member
        :       ^(MEMBER_DEF attributes[$MEMBER_DEF]? type) 
             { 
                 addToScope($MEMBER_DEF, TdfSymbol.SymbolType.TYPE);             
             } 
        ;
        

typedef_block
        :       ^(TYPEDEF attributes[$TYPEDEF]? type) 
            { 
                 addToScope($TYPEDEF, TdfSymbol.SymbolType.TYPE);         
            }             
        ;
        
        
const_block
        :       ^(CONST attributes[$CONST]? type (literal | sn=scoped_name | mm=MINMAX))
                {          
                        addToScope($CONST, TdfSymbol.SymbolType.TYPE);                       
                }
        |       ^(STRING_CONST attributes[$STRING_CONST]? (STRING_LITERAL | sn=scoped_name))
                {
                       addToScope($STRING_CONST, TdfSymbol.SymbolType.TYPE);
                }
        ;
        

bitfield_block
        :       ^(BITFIELD  { addToScope($BITFIELD, TdfSymbol.SymbolType.TYPE); } attributes[$BITFIELD]? bitfield_member*)
        ;
        
bitfield_member
    : ^(MEMBER_DEF attributes[$MEMBER_DEF]?)
    ;

enum_block
        : ^(ENUM {addToScope($ENUM, TdfSymbol.SymbolType.TYPE); } attributes[$ENUM]?  enum_member*)
        ;
        
enum_member
    :  ^(MEMBER_DEF attributes[$MEMBER_DEF]?) {addToScope($MEMBER_DEF, TdfSymbol.SymbolType.TYPE); }
    ;


/* ================================================================================
                                         RPC Types
   ================================================================================*/
root_component_block 
    : ^(ROOTCOMPONENT attributes[$ROOTCOMPONENT]? root_component_members*)
    ;
    
root_component_members
    : errors_block
    | sdk_errors_block
    | permissions_block
    | alloc_groups_block
    ;

component_block
        scope SymbolScope;
        : ^(COMPONENT 
                {
                  addToScope($COMPONENT, TdfSymbol.SymbolType.TYPE);
                  pushScope($COMPONENT);
                } attributes[$COMPONENT]? component_members*)
        ;
        
        
component_members
        : subcomponent_block
        | errors_block
        | ^(TYPE attributes[$TYPE]?)
        | permissions_block
        | alloc_groups_block
        ;
        
subcomponent_block       
        : ^(SLAVE { addToScope($SLAVE, TdfSymbol.SymbolType.TYPE); } attributes[$SLAVE]?  subcomponent_members*)
        | ^(MASTER { addToScope($MASTER, TdfSymbol.SymbolType.TYPE); } attributes[$MASTER]?  subcomponent_members*) 
        ;
        
subcomponent_members
        : ^(COMMAND attributes[$COMMAND]? (VOID | resp=typename) req=typename?) 
        { 
           addToScope($COMMAND, TdfSymbol.SymbolType.RPC); 
        } 
        | ^(NOTIFICATION attributes[$NOTIFICATION]? typename?) 
        { 
            addToScope($NOTIFICATION, TdfSymbol.SymbolType.NOTIFICATION);
        } 
        | ^(EVENT attributes[$EVENT]? typename?) 
        { 
            addToScope($EVENT, TdfSymbol.SymbolType.EVENT); 
        } 
        | ^(DYNAMIC_MAP attributes[$DYNAMIC_MAP]?) { addToScope($DYNAMIC_MAP, TdfSymbol.SymbolType.TYPE); } 
        | ^(STATIC_MAP attributes[$STATIC_MAP]?) { addToScope($STATIC_MAP, TdfSymbol.SymbolType.TYPE); } 
        ;
        
errors_block
        : ^(ERRORS (^(ERROR { addToScope($ERROR, TdfSymbol.SymbolType.TYPE); } attributes[$ERROR]?))*) 
        ;
        

sdk_errors_block
        : ^(SDK_ERRORS (^(ERROR attributes[$ERROR]?))*) 
        ;
        
permissions_block
        : ^(PERMISSIONS (^(PERMISSION attributes[$PERMISSION]?))*) 
        ;

alloc_groups_block
        : ^(ALLOC_GROUPS (^(ALLOC_GROUP attributes[$ALLOC_GROUP]?))*) 
        ;        
        
/* ================================================================================
                                          Types
   ================================================================================*/
type
        :       non_collection_type
        |       collection_type 
        ;
        
non_collection_type  
        :   INT_PRIMITIVE
        |   CHAR_PRIMITIVE
        |   FLOAT_PRIMITIVE
        |   OBJECT_TYPE 
        |   OBJECT_ID
        |   OBJECT_PRIMITIVE 
        |   TIMEVALUE 
        |   BLOB 
        |   VARIABLE
        |   GENERIC
        |   string_type 
        |   typename 
        ;

typename 
        :   ^(TYPE_REF scoped_name)
        ;
        

string_type 
        : ^(STRING integer)            
        | ^(STRING scoped_name)          
        ;
        
collection_type 
        : ^(LIST type scoped_name?)                
        | ^(MAP t1=non_collection_type t2=type comp=SYMBOL?) 
    ;
        
        
/* ================================================================================]
                                         Attributes
   ================================================================================*/
        
attributes [HashTree owner]
        :  ^(ATTRIBS attribute[$owner]+)                                                                        
        ;

custom_attrib_instance_name
        : SYMBOL
        ;

attribute [HashTree owner]
        : attribute_pair[$owner] 
        | ^(ATTRIB_LIST attrib_key SYMBOL*)
    ;

attribute_pair [HashTree owner]
        : ^(ATTRIB_PAIR attrib_key literal)
        | ^(ATTRIB_PAIR attrib_key minmax_literal)
        | ^(ATTRIB_PAIR attrib_key typename)
        | ^(ATTRIB_PAIR attrib_key attribute_map[$owner])
        ;
        
attribute_map [HashTree owner]
        : ^(ATTRIB_MAP attribute_pair[$owner]+)
        ;

attrib_key 
        : SYMBOL | ERRORS | STRING_LITERAL | DECIMAL;
     
                                                               
/* ================================================================================
                                          Literals
   ================================================================================*/
minmax_literal : MINMAX
        ;

literal : DECIMAL|HEX|FLOAT|BOOLEAN|STRING_LITERAL|TYPE_PAIR
        ;

integer returns [Integer value]
        :       DECIMAL { $value = Integer.parseInt($DECIMAL.text); }
        |       HEX     { $value = Integer.parseInt($HEX.text.substring(2), 16); }
        ;
        
scoped_name 
        : SYMBOL+        
        ;

