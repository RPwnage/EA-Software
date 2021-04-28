import java.util.*;
import java.lang.reflect.*;
import org.antlr.runtime.*;

@SuppressWarnings("serial")
public class TdfSymbol extends HashToken 
{
    public enum Category
    {
        INT_PRIMITIVE,
        FLOAT_PRIMITIVE,
        STRING,
        CLASS,
        UNION,
        BLOB,
        VARIABLE,
        GENERIC,
        BITFIELD,
        MAP,
        LIST,
        ENUM,
        ENUM_VALUE,
        TYPEDEF,
        CONST,
        OBJECT_TYPE,
        OBJECT_ID,
        TIMEVALUE,
        MEMBER,
        NAMESPACE,
        COMPONENT,
        SUBCOMPONENT,
        COMMAND,
        EVENT,
        NOTIFICATION,
        REPLICATEDMAP,
        ERROR,
        CUSTOM_ATTRIBUTE,
        CUSTOM_ATTRIBUTE_INSTANCE
    }
    
    public enum SymbolType
    {
        TYPE,
        NOTIFICATION,
        RPC,
        EVENT,
        SCOPE
    }
    
    private interface GetObjectHandler
    {
        public Object get(TdfSymbol s);
    }
    
    static HashMap<String, GetObjectHandler> mHandlers = new HashMap<String, GetObjectHandler>();
    static HashMap<String, String> mHiddenMethods = new HashMap<String, String>();
    static
    {
        // initialize a map of double-dispatch handlers that we used to replace reflection and claw back 25% CPU from our parser
        // these handlers are used to convert StringTemplate member value access to an implicit function call against TdfSymbol.
        mHandlers.put("ActualCategory", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getActualCategory(); } });
        mHandlers.put("ActualCategoryBitField", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryBitField(); } });
        mHandlers.put("ActualCategoryBlob", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryBlob(); } });
        mHandlers.put("ActualCategoryClass", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryClass(); } });
        mHandlers.put("ActualCategoryCollection", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryCollection(); } });
        mHandlers.put("ActualCategoryConst", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryConst(); } });
        mHandlers.put("ActualCategoryEnum", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryEnum(); } });
        mHandlers.put("ActualCategoryEnumValue", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryEnumValue(); } });
        mHandlers.put("ActualCategoryFloatPrimitive", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryFloatPrimitive(); } });
        mHandlers.put("ActualCategoryIntPrimitive", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryIntPrimitive(); } });
        mHandlers.put("ActualCategoryList", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryList(); } });
        mHandlers.put("ActualCategoryMajorEntity", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryMajorEntity(); } });
        mHandlers.put("ActualCategoryMap", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryMap(); } });
        mHandlers.put("ActualCategoryPrimitive", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryPrimitive(); } });
        mHandlers.put("ActualCategoryString", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryString(); } });
        mHandlers.put("ActualCategoryStruct", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryStruct(); } });
        mHandlers.put("ActualCategoryTimeValue", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryTimeValue(); } });
        mHandlers.put("ActualCategoryTypedef", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryTypedef(); } });
        mHandlers.put("ActualCategoryUnion", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryUnion(); } });
        mHandlers.put("ActualCategoryVariable", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isActualCategoryVariable(); } });
        mHandlers.put("ActualSymbol", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getActualSymbol(); } });
        mHandlers.put("AttributedName", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getAttributedName(); } });
        mHandlers.put("BaseSymbol", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getBaseSymbol(); } });
        mHandlers.put("Category", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getCategory(); } });
        mHandlers.put("CategoryBitField", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryBitField(); } });
        mHandlers.put("CategoryBlob", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryBlob(); } });
        mHandlers.put("CategoryClass", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryClass(); } });
        mHandlers.put("CategoryConst", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryConst(); } });
        mHandlers.put("CategoryEnum", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryEnum(); } });
        mHandlers.put("CategoryEnumValue", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryEnumValue(); } });
        mHandlers.put("CategoryFloatPrimitive", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryFloatPrimitive(); } });
        mHandlers.put("CategoryIntPrimitive", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryIntPrimitive(); } });
        mHandlers.put("CategoryPrimitive", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryPrimitive(); } });
        mHandlers.put("CategoryList", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryList(); } });
        mHandlers.put("CategoryMajorEntity", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryMajorEntity(); } });
        mHandlers.put("CategoryMap", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryMap(); } });
        mHandlers.put("CategoryNamespace", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryNamespace(); } });
        mHandlers.put("CategoryString", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryString(); } });
        mHandlers.put("CategoryStruct", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryStruct(); } });
        mHandlers.put("CategoryExterned", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryExterned(); } });
        mHandlers.put("CategoryTimeValue", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryTimeValue(); } });
        mHandlers.put("CategoryTypedef", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryTypedef(); } });
        mHandlers.put("CategoryUnion", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryUnion(); } });
        mHandlers.put("CategoryVariable", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isCategoryVariable(); } });
        mHandlers.put("FullName", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getFullName(); } });
        mHandlers.put("FullNameList", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getFullNameList(); } });
        mHandlers.put("FullProtoNameList", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getFullProtoNameList(); } });
        mHandlers.put("Name", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getName(); } });
        mHandlers.put("NamespaceScope", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getNamespaceScope(); } });
        mHandlers.put("NonNamespaceScope", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getNonNamespaceScope(); } });
        mHandlers.put("Root", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.isRoot(); } });
        mHandlers.put("Scope", new GetObjectHandler() { public Object get(TdfSymbol s) { return s.getScope(); } });     // DEPRECATED

        Method[] methods = TdfSymbol.class.getDeclaredMethods();
        for (Method method : methods)
        {
            String formattedMethodName = method.getName();
            // as with handlers above, all implicit method invocations are performed via
            // a key that omits the "get" or "is" prefix of the method name.
            if (formattedMethodName.startsWith("get"))
                formattedMethodName = formattedMethodName.replaceFirst("get", "");
            else if (formattedMethodName.startsWith("is"))
                formattedMethodName = formattedMethodName.replaceFirst("is", "");
            // initialize list of methods that 'could' be mapped by a handler
            // and therefore *could* be callable from a StringTemplate, but are not.
            // This is useful to catch a situation where someone adds a new method
            // that they want to be callable from a StringTemplate but forgets to add
            // a corresponding handler above.
            if (!mHandlers.containsKey(formattedMethodName))
            {
                //System.err.println("Registered hidden method: " + method.getName());
                mHiddenMethods.put(formattedMethodName, method.getName());
            }
        }
    }

    private static Object invokeRegisteredHandler(TdfSymbol s, String key)
    {
        GetObjectHandler h = mHandlers.get(key);
        if (h != null)
            return h.get(s);
        String methodName = mHiddenMethods.get(key);
        if (methodName != null)
            throw new NullPointerException("TdfSymbol must register a handler for property: " + key + " -> " + methodName + "()");
        return null;
    }
    
    String mName;
    String mAttributedName;
    String mFullName;
    Category mCategory;
    TdfSymbol mParentSymbol;
    TdfSymbol mInnerSymbol;
    TdfSymbolTable mSymbolTable;
    HashMap<SymbolType, HashMap<String, List<TdfSymbol>>> mChildSymbols = new HashMap<SymbolType, HashMap<String, List<TdfSymbol>>>();
    List<TdfSymbol> mScopeHeirarchy = new ArrayList<TdfSymbol>();
    
    public String getName() { return mName; }
    
    
    public String getAttributedName() { return mAttributedName; }

    
    public String getFullName() 
    {
        boolean firstOne = true;
        StringBuilder result = new StringBuilder();
        for (TdfSymbol s : getScopeHeirarchy())
        {
            if (!s.isRoot())
            {
                if (firstOne)
                {
                    firstOne = false;
                }
                else
                {
                    result.append("::");
                }
                result.append(s.getAttributedName());
            }
        }

        return result.toString(); 
    }
    
    public List<String> getNamespaceScope() 
    {
        List<String> result = new ArrayList<String>();

        for (TdfSymbol s : getScopeHeirarchy())
        {
            if (!s.isRoot()) 
            {
                if(s.getCategory() == Category.NAMESPACE)
                {
                    result.add(s.getName());
                }
            }
        }

        return result;
    }

    public List<String> getNonNamespaceScope() 
    {
        List<String> result = new ArrayList<String>();

        for (TdfSymbol s : getScopeHeirarchy())
        {
            if(!s.isRoot()) 
            {
                if(s.getCategory() != Category.NAMESPACE) 
                {
                    result.add(s.getName());
                }
            }
        }

        return result;
    }

    public List<String> getFullNameList()
    {
        List<String> result = new ArrayList<String>();
        for (TdfSymbol s : getScopeHeirarchy())
        {
            if (!s.isRoot())
                result.add(s.getName());
        }

        return result;
    }

    
    public List<String> getFullProtoNameList()
    {
        List<String> result = new ArrayList<String>();
        for (TdfSymbol s : getScopeHeirarchy())
        {
            if (!s.isRoot())
            {
                if (s.getName().equals("Blaze"))
                {
                    result.add("eadp");
                    result.add("blaze");
                }
                else
                {
                    result.add(s.getName());
                }
            }
        }

        return result;
    }

    public TdfSymbol getParent() { return mParentSymbol; }
    public void setParent(TdfSymbol parent) 
    { 
        mParentSymbol = parent;
        mSymbolTable = mParentSymbol.mSymbolTable;
        mScopeHeirarchy.addAll(mParentSymbol.mScopeHeirarchy);
        mScopeHeirarchy.add(this);
    }
        
    public TdfSymbol getScope() { return mParentSymbol; } //DEPRECATED   
    
    public TdfSymbol.Category getCategory() { return mCategory; }
           
    public TdfSymbol getInnerSymbol() { return mInnerSymbol; }
    public void setInnerSymbol(TdfSymbol value) { mInnerSymbol = value; }
   
    
    public TdfSymbol addSymbol(TdfSymbol symbol, TdfSymbol.SymbolType symbolType) 
    {
        symbol.setParent(this);
        mSymbolTable.addSymbol(symbol, symbolType);
        HashMap<String, List<TdfSymbol>> symbolsByName = mChildSymbols.get(symbolType);
        if (symbolsByName == null)
        {
            symbolsByName = new HashMap<String, List<TdfSymbol>>();
            mChildSymbols.put(symbolType, symbolsByName);
        }
        
        List<TdfSymbol> list;
        if (symbolsByName.containsKey(symbol.getAttributedName()))
        {
            list = symbolsByName.get(symbol.getAttributedName());
        }
        else
        {
            list = new ArrayList<TdfSymbol>();
            symbolsByName.put(symbol.getAttributedName(), list);    
        }
        list.add(symbol);
        return symbol;
    }
    
    public TdfSymbol getOrAddSymbol(String name, SymbolType type, TdfSymbol.Category category, ExtHashtable attributes, ArrayList<String> attrOrderList) 
    {
        return getOrAddSymbol(new TdfSymbol(name, category, attributes, attrOrderList), type);
    }
    
    public TdfSymbol getOrAddSymbol(TdfSymbol symbol, SymbolType type)
    {
        String realname = symbol.getAttributedName();
        return isDefined(realname, type) ? getSymbol(realname, type) : addSymbol(symbol, type);
    }

    public TdfSymbol getSymbol(String name, SymbolType type) 
    { 
        if (isDefined(name, type))
        {
            Map<String, List<TdfSymbol>> symbolsByName = mChildSymbols.get(type);
            return symbolsByName.get(name).get(0);      
        } 
        return null;
    }
    
    public TdfSymbol getBaseSymbol()
    {
        TdfSymbol symbol = this;

        while(symbol.getInnerSymbol() != null)
        {
            symbol = symbol.getInnerSymbol();
        }

        return symbol;
    } 
    
    public List<TdfSymbol> getSymbols(String name, SymbolType type) 
    { 
        if (isDefined(name, type))
        {
            return mChildSymbols.get(type).get(name);
        }
        return null;            
    }
     
    public boolean isDefined(String name, SymbolType type) 
    { 
        Map<String, List<TdfSymbol>> symbolsByName = mChildSymbols.get(type);
        if (symbolsByName != null)
        {
            return symbolsByName.containsKey(name); 
        }
        return false;
    }
    
    public boolean isDefined(String name, SymbolType type, ExtHashtable attribs, ArrayList<String> attribOrderList) 
    { 
        return isDefined(TdfSymbol.createAttributedName(name, attribs, attribOrderList), type); 
    }
 
    public boolean isRoot() { return mParentSymbol == null; }

    public TdfSymbol findSymbol(Iterator<?> scopeName, SymbolType type)
    {
        String name = scopeName.next().toString();
        if (scopeName.hasNext())
        {
            //More to go, pass on to our child
            if (isDefined(name, type))
            {
                for (TdfSymbol s : getSymbols(name, type))
                {
                    TdfSymbol result = s.findSymbol(scopeName, type);
                    return result;
                }
            }
        }
        else
        {
            return getSymbol(name, type);
        }
        
        return null;
    }

    public List<String> getScopedName(TdfSymbol symbol, boolean includeSymbol)
    {
        //Find the common scope between us and the symbol
        Iterator<TdfSymbol> e = mScopeHeirarchy.iterator();
        Iterator<TdfSymbol> o = symbol.getScope().mScopeHeirarchy.iterator();
        TdfSymbol commonScope = mSymbolTable.getRootScope();
        while (e.hasNext() && o.hasNext())
        {
            TdfSymbol ev = e.next();
            TdfSymbol ov = o.next();
            
            if (ev == ov || ev.getName().equals(ov.getName()))
            {
                commonScope = ev;
            }
            else
                break;
        }

        //Now that we have a common scope, build the list of names from there.
        List<String> result = new ArrayList<String>();
        if(includeSymbol)
        {
            result.add(symbol.getName());
        }
        
        TdfSymbol curSymbolScope = symbol.getScope();
        while (curSymbolScope != commonScope && !curSymbolScope.getName().equals(commonScope.getName()) && !curSymbolScope.isRoot())
        {
            result.add(0, curSymbolScope.getName());
            curSymbolScope = curSymbolScope.mParentSymbol;
        }

        return result;
    }

    
    public List<TdfSymbol> getScopeHeirarchy() { return mScopeHeirarchy; }

    
    public TdfSymbol(int tokenVal, Token token, String name,  
            Category category, ExtHashtable attribs)
    {
        super(tokenVal, token, attribs);
    
        mName = name;
        mAttributedName = createAttributedName(name, null, null);
        mCategory = category;              
    }
       
    public TdfSymbol(int tokenVal, String text, String name,  
            Category category, ExtHashtable attribs)
    {
        super(tokenVal, text, attribs);
    
        mName = name;
        mAttributedName = createAttributedName(name, null, null);
        mCategory = category;
    }
    
    public TdfSymbol(String name, Category category, ExtHashtable attributes, ArrayList<String> attrOrderList)
    {
        super(0);
        mName = name;
        mAttributedName = createAttributedName(name, attributes, attrOrderList);
        put(attributes);                
        mCategory = category;
    }
    
    //Special constructor for the root symbol
    public TdfSymbol(TdfSymbolTable table)
    {
        super(0);
        mSymbolTable = table;
        mParentSymbol = null;
        mScopeHeirarchy.add(this); //Add ourselves.
    }

    //Category Helpers    
    public static boolean isCategoryString(Category c) { return c == Category.STRING;  }
    public static boolean isCategoryBlob(Category c) { return c == Category.BLOB;  }
    public static boolean isCategoryMap(Category c) { return c == Category.MAP; }
    public static boolean isCategoryList(Category c) { return c == Category.LIST; }
    public static boolean isCategoryStruct(Category c)
    {
        switch (c)
        {
            case CLASS: 
            case UNION:
            case LIST:
            case MAP:
            case VARIABLE:
            case GENERIC:
                return true;
            default:
                return false;
        }    
    }
    public static boolean isCategoryUnion(Category c) { return c == Category.UNION; }
    public static boolean isCategoryClass(Category c) { return c == Category.CLASS; }
    public static boolean isCategoryPrimitive(Category c) { return (c == Category.INT_PRIMITIVE || c == Category.FLOAT_PRIMITIVE || c == Category.ENUM || c == Category.OBJECT_TYPE || c == Category.OBJECT_ID); }
    public static boolean isCategoryFloatPrimitive(Category c) { return (c == Category.FLOAT_PRIMITIVE); }
    public static boolean isCategoryIntPrimitive(Category c) { return (c == Category.INT_PRIMITIVE); }
    public static boolean isCategoryEnum(Category c) { return c == Category.ENUM; }
    public static boolean isCategoryEnumValue(Category c) { return c == Category.ENUM_VALUE; }
    public static boolean isCategoryVariable(Category c) { return c == Category.VARIABLE; }
    public static boolean isCategoryGeneric(Category c) { return c == Category.GENERIC; }
    public static boolean isCategoryTypedef(Category c) { return c == Category.TYPEDEF; }
    // Data types that have to be externed (string already special cased)
    public static boolean isCategoryExterned(Category c) { return (c == Category.TIMEVALUE || c == Category.OBJECT_TYPE || c == Category.OBJECT_ID); }
    public static boolean isCategoryTimeValue(Category c) { return c == Category.TIMEVALUE; }
    public static boolean isCategoryNamespace(Category c) { return c == Category.NAMESPACE; }
    public static boolean isCategoryBitField(Category c) { return c == Category.BITFIELD; }
    public static boolean isCategoryCollection(Category c) { return (c == Category.LIST || c == Category.MAP); }
    public static boolean isCategoryConst(Category c) { return c == Category.CONST;  }
    public static boolean isCategoryMajorEntity(Category c) 
    {
        switch (c)
        {
            case COMPONENT:
            case SUBCOMPONENT:
            case COMMAND:
            case EVENT:
            case NOTIFICATION:
            case REPLICATEDMAP:
            case CUSTOM_ATTRIBUTE:
                return true;
            default:
                return false;
        }
    }
    
    public Category getActualCategory() { return mInnerSymbol != null ? mInnerSymbol.getActualCategory() : mCategory; }
    public TdfSymbol getActualSymbol() { return mInnerSymbol != null ? mInnerSymbol.getActualSymbol() : this; }             

    public boolean isCategoryBitField() { return isCategoryBitField(getCategory()); } 
    public boolean isActualCategoryBitField() { return isCategoryBitField(getActualCategory()); } 
    
    public boolean isCategoryString() { return isCategoryString(getCategory()); } 
    public boolean isActualCategoryString() { return isCategoryString(getActualCategory()); } 

    public boolean isCategoryBlob() { return isCategoryBlob(getCategory()); } 
    public boolean isActualCategoryBlob() { return isCategoryBlob(getActualCategory()); } 
    
    public boolean isCategoryMap() { return isCategoryMap(getCategory()); } 
    public boolean isActualCategoryMap() { return isCategoryMap(getActualCategory()); } 

    public boolean isCategoryList() { return isCategoryList(getCategory()); } 
    public boolean isActualCategoryList() { return isCategoryList(getActualCategory()); } 

    public boolean isCategoryStruct() { return isCategoryStruct(getCategory()); } 
    public boolean isActualCategoryStruct() { return isCategoryStruct(getActualCategory()); } 
    
    public boolean isCategoryClass() { return isCategoryClass(getCategory()); }
    public boolean isActualCategoryClass() { return isCategoryClass(getActualCategory()); }
    
    public boolean isCategoryUnion() { return isCategoryUnion(getCategory()); }
    public boolean isActualCategoryUnion() { return isCategoryUnion(getActualCategory()); }

    public boolean isCategoryPrimitive() { return isCategoryPrimitive(getCategory()); } 
    public boolean isActualCategoryPrimitive() { return isCategoryPrimitive(getActualCategory()); }

    public boolean isCategoryFloatPrimitive() { return isCategoryFloatPrimitive(getCategory()); } 
    public boolean isActualCategoryFloatPrimitive() { return isCategoryFloatPrimitive(getActualCategory()); }

    public boolean isCategoryIntPrimitive() { return isCategoryIntPrimitive(getCategory()); } 
    public boolean isActualCategoryIntPrimitive() { return isCategoryIntPrimitive(getActualCategory()); }
    
    public boolean isCategoryConst() { return isCategoryConst(getCategory()); } 
    public boolean isActualCategoryConst() { return isCategoryConst(getActualCategory()); }
    
    public boolean isCategoryEnum() { return isCategoryEnum(getCategory()); } 
    public boolean isActualCategoryEnum() { return isCategoryEnum(getActualCategory()); } 
    
    public boolean isCategoryEnumValue() { return isCategoryEnumValue(getCategory()); } 
    public boolean isActualCategoryEnumValue() { return isCategoryEnumValue(getActualCategory()); } 

    
    public boolean isCategoryVariable() { return isCategoryVariable(getCategory()); } 
    public boolean isActualCategoryVariable() { return isCategoryVariable(getActualCategory()); }

    public boolean isCategoryTypedef() { return isCategoryTypedef(getCategory()); } 
    public boolean isActualCategoryTypedef() { return isCategoryTypedef(getActualCategory()); }

    public boolean isCategoryExterned() { return isCategoryExterned(getCategory()); } 
    public boolean isActualCategoryExterned() { return isCategoryExterned(getActualCategory()); } 

    public boolean isCategoryTimeValue() { return isCategoryTimeValue(getCategory()); } 
    public boolean isActualCategoryTimeValue() { return isCategoryTimeValue(getActualCategory()); }

    public boolean isCategoryMajorEntity() { return isCategoryMajorEntity(getCategory()); } 
    public boolean isActualCategoryMajorEntity() { return isCategoryMajorEntity(getActualCategory()); }

    public boolean isCategoryNamespace() { return isCategoryNamespace(getCategory()); }
    
    public boolean isActualCategoryCollection() { return isCategoryCollection(getActualCategory()); }
    
    public static String createAttributedName(String name, ExtHashtable attributes, ArrayList<String> attrOrderList)
    {
        String result = name;
        if (attributes != null && attributes.size() > 0)
        {
            result += "<";
            int attribSize = attrOrderList.size();
            boolean addedFirst = false;
            for(String s : attrOrderList)            
            {             
                Object o = attributes.get(s);
                if (o != null)
                {
                    if (addedFirst)
                        result += ",";
                    addedFirst = true;
                    result += o.toString();
                }
            }
            result += ">";
        }
        
        return result;
    }

    public String toString()
    {
        return getFullName();
    }
    
    public static String decodeTag(int tag, boolean toLower)
    {
        char[] buf = new char[5];
        int size = 0;

        tag >>= 8;
        for(int i=3; i>=0; --i) {
            int sixbits = tag & 0x3f;
            if(sixbits != 0) {
                buf[i] = (char)(sixbits + 32);
                size++;
            } 
            else
            {
                buf[i] = '\0';
            }
            tag >>= 6;
        }
        buf[4] = '\0';
        
        String tagStr = new String(buf,0,size);
        if(toLower)
            tagStr = tagStr.toLowerCase();
        
        return tagStr;
    }
    
    public Object get(Object key)
    {
        //Check the super class to see if the key is defined
        Object value = super.get(key);
        if (value != null)
        {
            return value;
        }
        
        return invokeRegisteredHandler(this, key.toString());
    }    
}
