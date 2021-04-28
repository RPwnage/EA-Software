tree grammar TDFTypeResolver;

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

scope NameGenHint
{
    String name;
}
  
scope ResolveFailBehavior
{
   boolean ThrowException;
}
  
@header
{
import java.util.*;
import org.antlr.runtime.BitSet;
} 

@members
{
    boolean mTopLevel;
    TdfSymbolTable mSymbols;
    TreeMap<String, HashToken> mIncludeFileNodes;

    //Search through all scopes on the stack for the symbol.
    // Since symbol names might collide between different types of symbols
    // (ex. Rpc name and request type), we need to segment the symbol table and subsequently the lookup
    // by type.  Depending on the lookup context, we specify what type of symbol we are looking
    // for with the type parameter.
    TdfSymbol findSymbol(List names, TdfSymbol.SymbolType type)
    {
        TdfSymbol result = null;
        for (int i = $SymbolScope.size() - 1; result == null && i >= 0; --i)
        {
            Iterator itr = names.iterator();
            if (itr.hasNext())
            {
                result = $SymbolScope[i]::Scope.findSymbol(itr, type);
            }
        }
            
        return result;
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

    int FNV1_String8 (String pData)
    {
        int kFNV1InitialValue = -2128831035; // == (unsigned int) 2166136261;
        int nInitialValue = kFNV1InitialValue;
        
        int length = pData.length();
        int curPos = 0;
        while(curPos < length)
        {
            int c = pData.charAt(curPos);
            nInitialValue = ((int)nInitialValue * (int)16777619) ^ (int)c;
            ++curPos;
        }
    
        return nInitialValue;
    }

    
    void fixupTdfId(HashTree node, boolean isClient)
    {
        TdfSymbol symbol = (TdfSymbol) node.getToken();

        //Differentiate variable tdf registrations (where tdfid is either explicitly set or set to "hash")      
        String val = (String)symbol.get("tdfid");
        if (val != null)
        {
            symbol.put("tdfIdRegistration", "true");
        }

        String fullClassName = symbol.getFullName();
        symbol.put("tdfid", FNV1_String8(fullClassName));
    }

    void fixupRegistrationType(HashTree node)
    {
        TdfSymbol symbol = (TdfSymbol) node.getToken();
            
        String val = (String)symbol.get("tdfregistration");
        if ((val != null) && 
            (val.equalsIgnoreCase("explicit")))
        {
            symbol.put("ExplicitRegistration", 1);  
       
            // mark explicit registration required in current namespace
            TdfSymbol parent = symbol.getParent();
            while(parent != null)
            {              
                if (parent.isCategoryNamespace())
                {
                    parent.put("ExplicitRegistrationRequired","true");
                    return;
                }
             
                parent = parent.getParent();
            }
        }
    }

    void verifyPassthroughNotification(TdfSymbol slaveNotification)
    {
        Object value = slaveNotification.get("client_export");
        if ((value != null) && !((Boolean)value))
        {
            throw new NullPointerException("Slave passthrough notification " + slaveNotification.get("Name") + 
            " specifies client_export=false which is not supported for passthrough notifications");
        }
    }
    
    String getHintName()
    {
        String result = "";
        for (int counter = 0; counter < $NameGenHint.size(); counter++)
        {
            result += $NameGenHint[counter]::name;
            if (counter < $NameGenHint.size() - 1) result += "_";
        }
            
        return result;
    }
    
    boolean mIsClient;
}

/* ================================================================================
                                          Highest Level 
   ================================================================================*/
//TODO file params
file [TdfSymbolTable symbols, TreeMap<String, HashToken> includeFileNodes, boolean isClient]
        scope SymbolScope, ResolveFailBehavior;
        @init 
        { 
                mIsClient = isClient;
                mSymbols = symbols;
                mIncludeFileNodes = includeFileNodes;                
                mTopLevel = true;
                $SymbolScope::Scope = mSymbols.getRootScope();
                pushTokenStream(getTreeNodeStream().getTokenStream());
                $ResolveFailBehavior::ThrowException = true;
        }
        : file_def [ mTopLevel ]
        ;
        
/* ================================================================================
                                          Main Elements
   ================================================================================*/
file_def [boolean topLevel] returns [HashToken r] 
        @init
        {
            boolean oldTopLevel = mTopLevel;
            mTopLevel = topLevel;
        }
        :   ^(FILE file_member*) 
        { 
            $r = $FILE.getHashToken();  
            mTopLevel = oldTopLevel;
        }
        ;


file_member
        : namespace_member
        | include_block
        | group_block
        ;
        
include_block 
        : ^(INCLUDE  
         {           
                TokenStream ts = (TokenStream) $INCLUDE.get("TokenStream");
                if (ts != null)
                {
                    pushTokenStream(ts); 
                }
         }
              attributes[$INCLUDE]?  file_def[false]?
         { 
                if ($file_def.r != null)
                {
                    $INCLUDE.put("FileNode", $file_def.r);
                    mIncludeFileNodes.put((String) $INCLUDE.get("ResolvedFileName"), $file_def.r);
                    popTokenStream(); 
                }
                else
                {
                    $INCLUDE.put("FileNode", mIncludeFileNodes.get((String) $INCLUDE.get("ResolvedFileName")));                
                }
         }
         ) -> ^(INCLUDE file_def?)
         | ^(FAKE_INCLUDE attributes[$FAKE_INCLUDE]?) -> FAKE_INCLUDE
        ;

namespace_block 
        scope SymbolScope;       
        : ^(NAMESPACE 
          {
               pushScope($NAMESPACE.getHashToken()); 
          } attributes[$NAMESPACE]? namespace_member*) -> ^(NAMESPACE namespace_member*)
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

custom_attribute_block
    scope NameGenHint;
    : ^(CUSTOM_ATTRIBUTE custom_attribute_member*)
     ;

custom_attribute_member
        scope NameGenHint;
        : ^(md=MEMBER_DEF {$NameGenHint::name = $MEMBER_DEF.getString("Name"); } custom_attribute_type) 
           { 
                  TdfSymbolHolder member = (TdfSymbolHolder) $md.getToken();
                 member.put("TypeRef", new TdfSymbolRef(member.getParent(), $custom_attribute_type.symbol));
                 member.setRef($custom_attribute_type.symbol);
           } 
           -> ^(MEMBER_DEF custom_attribute_type)
        ;

custom_attribute_type returns [TdfSymbol symbol]
    : INT_PRIMITIVE { $symbol = $SymbolScope[0]::Scope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.INT_PRIMITIVE, null, null); }
    | STRING { $symbol = $SymbolScope[0]::Scope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.STRING, null, null); }
    ;

/* ================================================================================
                                         TDF Types
   ================================================================================*/

class_fwd_decl_block
        : ^(CLASS_FWD_DECL attributes[$CLASS_FWD_DECL]?) -> CLASS_FWD_DECL
        ;
        
class_block
        scope SymbolScope;
        :        ^(CLASS 
                 {
                        //Push the current scope for type resolution, add a members field attribute
                        fixupTdfId($CLASS, mIsClient);
                        fixupRegistrationType($CLASS);
                        pushScope($CLASS.getHashToken()); 
                       
                }
                attributes[$CLASS]? class_members*) 
                -> ^(CLASS class_members*)
        ;
        
class_members 
    :        class_member
    |        type_definition_block
    ;
    

class_member
        scope { HashToken t; }
        scope NameGenHint;
        :   ^(md=MEMBER_DEF {$class_member::t = $md.getHashToken(); $NameGenHint::name = $md.getString("Name"); } attributes[$MEMBER_DEF]?  type) 
                {                    
                        TdfSymbolHolder member = (TdfSymbolHolder) $md.getToken();
                        member.put("TypeRef", new TdfSymbolRef(member.getParent(), $type.symbol));
                        member.setRef($type.symbol);
                } -> ^(MEMBER_DEF type)
        ;
        
union_block
        scope SymbolScope;       
        :       ^(UNION 
              {
                        //Push the current scope for type resolution, add a members field attribute
                        fixupTdfId($UNION, mIsClient);
                        fixupRegistrationType($UNION);
                        pushScope($UNION.getHashToken()); 
              }
              attributes[$UNION]? union_member*) -> ^(UNION union_member*)
        ;

union_member
        scope NameGenHint;
        :       ^(MEMBER_DEF  {$NameGenHint::name = $MEMBER_DEF.getString("Name"); } attributes[$MEMBER_DEF]? type) 
             { 
                  TdfSymbol member = (TdfSymbol) $MEMBER_DEF.getToken();
                 member.put("TypeRef", new TdfSymbolRef(member.getParent(), $type.symbol));
             } -> ^(MEMBER_DEF type)
        ;
        

typedef_block
        scope NameGenHint;
        :       ^(TYPEDEF {$NameGenHint::name = $TYPEDEF.getString("Name"); } attributes[$TYPEDEF]? type) 
            { 
                 TdfSymbol typedef = (TdfSymbol) $TYPEDEF.getToken();
                 typedef.put("TypeRef", new TdfSymbolRef(typedef.getParent(), $type.symbol));
                 typedef.setInnerSymbol($type.symbol);
            }
             -> ^(TYPEDEF type)
        ;
        
        
const_block
        scope NameGenHint;
        :       ^(CONST {$NameGenHint::name = $CONST.getString("Name"); } attributes[$CONST]?  type (literal | sn=scoped_name[TdfSymbol.SymbolType.TYPE] | mm=MINMAX))
                {          
                        TdfSymbol symbol = (TdfSymbol) $CONST.getToken();  
                        symbol.put("TypeRef", new TdfSymbolRef($SymbolScope::Scope, $type.symbol));
                        symbol.put("ValueRef", ($sn.symbol != null) ? new TdfSymbolRef($SymbolScope::Scope, $sn.symbol) : null);
                        symbol.put("ValueString", ($mm.text != null) ? $mm.text : null);
                        symbol.put("IsExterned", ($type.symbol != null) ? $type.symbol.isCategoryExterned() : null);
                }
         -> CONST
        |       ^(STRING_CONST attributes[$STRING_CONST]? (STRING_LITERAL | sn=scoped_name[TdfSymbol.SymbolType.TYPE] ))
                {
                        TdfSymbol symbol = (TdfSymbol) $STRING_CONST.getToken();  
                        symbol.put("IsString", true);
                        symbol.put("ValueRef", ($sn.symbol != null) ? new TdfSymbolRef(symbol.getParent(), $sn.symbol) : null);
                }
           -> STRING_CONST
        ;
        

bitfield_block
        :       ^(BITFIELD 
                {
                    fixupTdfId($BITFIELD, mIsClient);
                    fixupRegistrationType($BITFIELD);
                }
                attributes[$BITFIELD]?  bitfield_member*) -> ^(BITFIELD bitfield_member*) 
        ;
        
bitfield_member
    : ^(MEMBER_DEF attributes[$MEMBER_DEF]?) -> MEMBER_DEF
    ;

enum_block
        : ^(ENUM 
            {
                fixupTdfId($ENUM, mIsClient);
                fixupRegistrationType($ENUM);
            }
            attributes[$ENUM]? enum_member*) -> ^(ENUM enum_member*)
        ;
        
enum_member
    :  ^(MEMBER_DEF attributes[$MEMBER_DEF]?)  -> MEMBER_DEF
    ;


/* ================================================================================
                                         RPC Types
   ================================================================================*/
root_component_block 
    : ^(ROOTCOMPONENT attributes[$ROOTCOMPONENT]? root_component_members*) -> ^(ROOTCOMPONENT root_component_members*)
    ;
    
root_component_members
    : errors_block
    | sdk_errors_block
    | permissions_block
    | alloc_groups_block
    ;

component_block
        scope SymbolScope;
        : ^(COMPONENT { pushScope($COMPONENT); } attributes[$COMPONENT]? component_members*) -> ^(COMPONENT component_members*)
        ;
        
        
component_members
        : subcomponent_block
        | errors_block
        | ^(TYPE attributes[$TYPE]?) -> TYPE
        | permissions_block
        | alloc_groups_block
        ;
        
subcomponent_block
        scope ResolveFailBehavior;        
        : ^(SLAVE { $ResolveFailBehavior::ThrowException = true; } attributes[$SLAVE]? subcomponent_members*) -> ^(SLAVE subcomponent_members*)                   
        | ^(MASTER { $ResolveFailBehavior::ThrowException = !mIsClient; } attributes[$MASTER]? subcomponent_members*)  -> ^(MASTER subcomponent_members*)                   
        ;
        
subcomponent_members
        : ^(COMMAND attributes[$COMMAND]? (VOID | resp=typename) req=typename?) 
        { 
           if ($resp.symbol != null)
           {
               $COMMAND.put("ResponseType", new TdfSymbolRef(((TdfSymbol) $COMMAND.getToken()), $resp.symbol));              
           }
           
           if ($req.symbol != null)
           {
               $COMMAND.put("RequestType", new TdfSymbolRef(((TdfSymbol) $COMMAND.getToken()).getParent(), $req.symbol));
           }
        } -> COMMAND
        | ^(NOTIFICATION attributes[$NOTIFICATION]? typename?) 
        { 
            if ($typename.symbol != null)
            {
                $NOTIFICATION.put("NotificationType", new TdfSymbolRef(((TdfSymbol) $NOTIFICATION.getToken()).getParent(), $typename.symbol));
            }
        } -> NOTIFICATION
        | ^(EVENT attributes[$EVENT]? typename?) 
        { 
            if ($typename.symbol != null)
            {
                $EVENT.put("EventType", new TdfSymbolRef(((TdfSymbol) $EVENT.getToken()).getParent(), $typename.symbol));
            }
        } -> EVENT
        | ^(DYNAMIC_MAP attributes[$DYNAMIC_MAP]?) -> DYNAMIC_MAP
        | ^(STATIC_MAP attributes[$STATIC_MAP]?) -> STATIC_MAP
        ;
        
errors_block
        : ^(ERRORS (^(ERROR attributes[$ERROR]?))*) -> ^(ERRORS ERROR*)
        ;
        

sdk_errors_block
        : ^(SDK_ERRORS (^(ERROR attributes[$ERROR]?))*) -> ^(SDK_ERRORS ERROR*)
        ;
        
permissions_block
        : ^(PERMISSIONS (^(PERMISSION attributes[$PERMISSION]?))*) -> ^(PERMISSIONS PERMISSION*)
        ;

alloc_groups_block
        : ^(ALLOC_GROUPS (^(ALLOC_GROUP attributes[$ALLOC_GROUP]?))*) -> ^(ALLOC_GROUPS ALLOC_GROUP*)
        ;
        
/* ================================================================================
                                          Types
   ================================================================================*/
type returns [TdfSymbol symbol]
        :       non_collection_type { $symbol = $non_collection_type.symbol; }
        |       collection_type { $symbol = $collection_type.symbol; }
        ;
        
non_collection_type  returns [TdfSymbol symbol]
        :   INT_PRIMITIVE { $symbol = $SymbolScope[0]::Scope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.INT_PRIMITIVE, null, null); }
        |   CHAR_PRIMITIVE { $symbol = $SymbolScope[0]::Scope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.INT_PRIMITIVE, null, null); }
        |   FLOAT_PRIMITIVE { $symbol = $SymbolScope[0]::Scope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.FLOAT_PRIMITIVE, null, null); }
        |   OBJECT_TYPE
            { 
                TdfSymbol eascope = $SymbolScope[0]::Scope.getOrAddSymbol("EA", TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.NAMESPACE, null, null);
                TdfSymbol tdfscope = eascope.getOrAddSymbol("TDF", TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.NAMESPACE, null, null);
                $symbol = tdfscope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.OBJECT_TYPE, null, null); 
            }
        |   OBJECT_ID
            { 
                TdfSymbol eascope = $SymbolScope[0]::Scope.getOrAddSymbol("EA", TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.NAMESPACE, null, null);
                TdfSymbol tdfscope = eascope.getOrAddSymbol("TDF", TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.NAMESPACE, null, null);
                $symbol = tdfscope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.OBJECT_ID, null, null); 
            }
        |   OBJECT_PRIMITIVE 
            { 
            
                TdfSymbol eascope = $SymbolScope[0]::Scope.getOrAddSymbol("EA", TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.NAMESPACE, null, null);
                TdfSymbol tdfscope = eascope.getOrAddSymbol("TDF", TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.NAMESPACE, null, null);
                $symbol = tdfscope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.INT_PRIMITIVE, null, null); 
            }
        |   TIMEVALUE
            {
                TdfSymbol eascope = $SymbolScope[0]::Scope.getOrAddSymbol("EA", TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.NAMESPACE, null, null);
                TdfSymbol tdfscope = eascope.getOrAddSymbol("TDF", TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.NAMESPACE, null, null);
                $symbol = tdfscope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.TIMEVALUE, null, null);
            }
        |   BLOB { $symbol = $SymbolScope[0]::Scope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.BLOB, null, null); }
        |   VARIABLE { $symbol =  $SymbolScope[0]::Scope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.VARIABLE, null, null);}
        |   GENERIC { $symbol =  $SymbolScope[0]::Scope.getOrAddSymbol($text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.GENERIC, null, null);}
        |   string_type  { $symbol = $string_type.symbol; }
        |   typename { $symbol = $typename.symbol; }
        ;

typename returns [TdfSymbol symbol]
        :   ^(TYPE_REF scoped_name[TdfSymbol.SymbolType.TYPE]) { $symbol = $scoped_name.symbol; } -> TYPE_REF
        ;
        

string_type returns [TdfSymbol symbol]
    @init { TdfSymbol constToken; }
        :        ^(STRING integer)
             {
                //We need to craete a constant instead of resolving one
                String constName = "MAX_" + getHintName().toUpperCase() + "_LEN";
                constToken = new TdfSymbol(CONST, "const", constName, TdfSymbol.Category.CONST,  
                                new ExtHashtable().
                                put("TypeRef", new TdfSymbolRef($SymbolScope::Scope, mSymbols.getRootScope().getOrAddSymbol("uint32_t", TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.INT_PRIMITIVE, null, null))).
                                put("Value",$integer.value + 1).
                                put("description", "Max string length for member " + $NameGenHint[0]::name + ".").
                                put("IsStringLen", true));
                $SymbolScope::Scope.addSymbol(constToken, TdfSymbol.SymbolType.TYPE);
                                

                ExtHashtable attr = new ExtHashtable();
                attr.put("GenConstant", true);
                attr.put("SizeConstant", constToken);
                ArrayList<String> attrOrderList = new ArrayList<String>();
                attrOrderList.add("SizeConstant");
                $symbol = $SymbolScope[0]::Scope.getOrAddSymbol($STRING.text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.STRING, attr, attrOrderList);
             }
             -> ^(STRING<HashToken>[$STRING.getToken(), new ExtHashtable().put("GenConst", constToken)])
        |        ^(STRING scoped_name[TdfSymbol.SymbolType.TYPE])
                 {
                        ExtHashtable attr = new ExtHashtable();
                        attr.put("SizeConstant", $scoped_name.symbol);
                        ArrayList<String> attrOrderList = new ArrayList<String>();
                        attrOrderList.add("SizeConstant");
                        
                        $symbol = $SymbolScope[0]::Scope.getOrAddSymbol($STRING.text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.STRING, attr, attrOrderList);
                 }
                 -> ^(STRING<HashToken>[$STRING.getToken(), null])
        ;
        
collection_type  returns [TdfSymbol symbol]
        scope NameGenHint;
        :       ^(LIST {$NameGenHint::name = "Value"; } type scoped_name[TdfSymbol.SymbolType.TYPE]?) 
                {
                        ExtHashtable attr = new ExtHashtable($LIST.getHashToken());
                        attr.put("ValueType", $type.symbol); 
                        attr.put("ValueTypeRef", new TdfSymbolRef($SymbolScope::Scope, $type.symbol));  
                        attr.put("ReserveSizeConst", $scoped_name.symbol); 
                        attr.put("ReserveSize", (attr.containsKey("ReserveSizeConst") || attr.containsKey("ReserveSizeValue"))); 

                        ArrayList<String> attrOrderList = new ArrayList<String>();
                        attrOrderList.add("ValueType");
                        attrOrderList.add("ReserveSizeConst");
                        attrOrderList.add("ReserveSizeValue");
                        attrOrderList.add("FillList");
                
                        //TODO ugly hack to not print the typedef twice in the header - todo - add typedefs here for automatic usage elsewhere
                        if ($class_member.size() > 0)
                        {
                                //Put a new symbol in for the typedef.
                                boolean isPresent = $SymbolScope::Scope.isDefined($type.symbol.getAttributedName() + "List", TdfSymbol.SymbolType.TYPE, null, null);
                                if (mTopLevel && !isPresent)
                                {
                                    $class_member::t.put("FirstDef", true);
                                    $SymbolScope::Scope.getOrAddSymbol($type.symbol.getAttributedName() + "List", TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.TYPEDEF, null, null);
                                }
                                
                                $class_member::t.put("DeprecateDef", !$class_member::t.getString("Name").equals($type.symbol.getName()));
                                
                        }
                        
                        $symbol = $SymbolScope::Scope.getOrAddSymbol($LIST.text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.LIST, attr, attrOrderList);
                        
                }
                -> ^(LIST type)
        |       ^(MAP {$NameGenHint::name = "Key"; } t1=non_collection_type {$NameGenHint::name = "Value"; } t2=type comp=SYMBOL?) 
                { 
                        ExtHashtable attr = new ExtHashtable();                 
                        attr.put("KeyType", $t1.symbol); 
                        attr.put("ValueType", $t2.symbol); 
                        attr.put("KeyTypeRef", new TdfSymbolRef($SymbolScope::Scope, $t1.symbol)); 
                        attr.put("ValueTypeRef", new TdfSymbolRef($SymbolScope::Scope, $t2.symbol)); 
                        attr.put("Comparator", $comp.text);
                        
                        ArrayList<String> attrOrderList = new ArrayList<String>();
                        attrOrderList.add("KeyType");
                        attrOrderList.add("ValueType");
                        attrOrderList.add("Comparator");
                        
                        $symbol = $SymbolScope::Scope.getOrAddSymbol($MAP.text, TdfSymbol.SymbolType.TYPE, TdfSymbol.Category.MAP, attr, attrOrderList);
                        
                } -> ^(MAP non_collection_type type)
        ;
        
        
/* ================================================================================]
                                         Attributes
   ================================================================================*/
        
attributes [HashTree owner]
        :  ^(ATTRIBS attribute[$owner]+)                                                                        
        ;

attribute [HashTree owner]
        : attribute_pair[$owner] 
        | ^(ATTRIB_LIST attrib_key SYMBOL*)
    ;

    

attribute_pair [HashTree owner]
        : ^(ATTRIB_PAIR attrib_key literal)
        | ^(ATTRIB_PAIR attrib_key minmax_literal)
          {
            $owner.put($attrib_key.text+"IsMinMax", true);
          }
        | ^(ATTRIB_PAIR attrib_key attribute_value_scoped_name[$attrib_key.text, (TdfSymbol)$owner.getToken()]) 
          { 
          
              //We reset the type to be a symbol instead of a typename in the owning attribute map

              // But first, for the TdfToProto conversion tools, back off original values. E.g. for things like
              // the command rpc passthrough field, we want to have the original identifier, in the converted to .proto.
              String origValStr = $owner.getToken().toString();
              String origValNewKey = $attrib_key.text+"_origval";
              $owner.put(origValNewKey, origValStr);

               if ($attribute_value_scoped_name.symbol != null) {
                    $owner.put($attrib_key.text, new TdfSymbolRef(((TdfSymbol) $owner.getToken()).getScope(), $attribute_value_scoped_name.symbol)); 
                    $owner.put($attrib_key.text+"IsSymbolRef", true);
               }
          }
        | ^(ATTRIB_PAIR attrib_key attribute_map[$owner])
        ;
        
attribute_map [HashTree owner]
        : ^(ATTRIB_MAP (attribute_pair[$owner])+)
        ;        

//TODO: attribute values are typically literals or types, with the exception of passthrough rpcs.  Since the symbol table is divided by
// symbol type, we need to do some special handling for the passthrough attribute to make sure that the symbol can
// be found.  This is a temporary workaround until we can handle this more gracefully without specialized code.
attribute_value_scoped_name[String value, TdfSymbol owner] returns [TdfSymbol symbol]
scope { String attributeKey; Boolean isNotification;}
@init 
{
    $attribute_value_scoped_name::attributeKey = $value; 
    $attribute_value_scoped_name::isNotification = (owner.getCategory() == TdfSymbol.Category.NOTIFICATION);
}
    : {$attribute_value_scoped_name::attributeKey.equals("passthrough") && !$attribute_value_scoped_name::isNotification}? => ^(TYPE_REF scoped_name[TdfSymbol.SymbolType.RPC]) { $symbol = $scoped_name.symbol; } -> TYPE_REF
    | {$attribute_value_scoped_name::attributeKey.equals("passthrough") && $attribute_value_scoped_name::isNotification}? => ^(TYPE_REF scoped_name[TdfSymbol.SymbolType.NOTIFICATION]) { $symbol = $scoped_name.symbol; verifyPassthroughNotification($symbol);} -> TYPE_REF
    | ^(TYPE_REF scoped_name[TdfSymbol.SymbolType.TYPE]) { $symbol = $scoped_name.symbol; } -> TYPE_REF
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
        
scoped_name[TdfSymbol.SymbolType type] returns [TdfSymbol symbol]
        : names+=SYMBOL+ 
        { 
                $symbol = findSymbol($names, $type); 
                if ($symbol == null && $ResolveFailBehavior::ThrowException)
                    throw new NullPointerException("Cannot find symbol " + $names);
        }
        ;
        
