tree grammar TDFRelationshipResolver;

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

scope NotificationIndexer
{
   int i;
}

@header
{
import java.util.*;
import org.antlr.runtime.BitSet;
import java.io.File;
}




@members
{
   
    TdfSymbolTable mSymbolTable;
  

    Stack<Stack<HashToken>> mTokenOwnersStackStack = new Stack<Stack<HashToken>>();
    Stack<HashToken> mTokenOwners;

    void pushTokenOwnersStack() { mTokenOwners = new Stack<HashToken>(); mTokenOwnersStackStack.push(mTokenOwners); }
    void popTokenOwnersStack() 
    { 
        mTokenOwnersStackStack.pop(); 
        mTokenOwners = mTokenOwnersStackStack.size() > 0 ? mTokenOwnersStackStack.peek() : null; 
    }

    void pushTokenOwner(HashTree node) { pushTokenOwner(node.getHashToken()); }
    void pushTokenOwner(HashToken token) { mTokenOwners.push(token); }
    void popTokenOwner() { mTokenOwners.pop(); }
    void addOwnedToken(String type, HashTree node) { addOwnedToken(type, node.getHashToken()); }
    void addOwnedToken(String type, HashToken token) 
    {
       mTokenOwners.peek().putInList(type, token); 
       token.put("Parent", mTokenOwners.peek());
    }
    void addOwnedToken(String type, HashTree node, int index) { addOwnedToken(type, node.getHashToken(), index); }
    void addOwnedToken(String type, HashToken token, int index) 
    { 
        index = (index >= 0) ? index : mTokenOwners.size() - 1 + index;
        mTokenOwners.elementAt(index).putInList(type, token); 
        token.put("Parent",mTokenOwners.elementAt(index));
    }

    HashToken getCurrentTokenOwner() { return mTokenOwners.peek(); }
    
    
    TdfSymbol getCurrentNamespace()
    {
	for (int counter = mTokenOwners.size() - 1; counter >= 0; counter--)
	{
	   if (mTokenOwners.elementAt(counter) instanceof TdfSymbol)
	   {
               TdfSymbol symbol = (TdfSymbol) mTokenOwners.elementAt(counter);
               if (symbol.isCategoryNamespace())
               	return symbol;
	   }
	}
	
	return null;
    }
               
    void addSelfTyperef(HashTree node)
    {
        TdfSymbol symbol = (TdfSymbol) node.getToken();
        symbol.put("TypeRef", new TdfSymbolRef(getCurrentNamespace(), symbol));
    }
    
    void verifyReplicatedMapTypes(HashTree mapNode)
    {
        TdfSymbol mapSymbol = (TdfSymbol) mapNode.getToken();
        TdfSymbol tdfSymbol = ((TdfSymbolRef)(mapSymbol.get("tdf_type"))).getSymbol();
        if (tdfSymbol.get("tdfid") == null)
        {
            throw new NullPointerException("The tdf type " + tdfSymbol.get("Name") + " must be tagged with a tdfid to be used in the replicated map " + mapSymbol.get("Name"));
        }
        
        Boolean trackChanges = (Boolean)tdfSymbol.get("trackChanges");
        if ((trackChanges == null) || (!trackChanges.booleanValue()))
        {
            throw new NullPointerException("The tdf type " + tdfSymbol.get("Name") + " must be attributed with \'trackChanges = true\' to be used in the replicated map \'" + mapSymbol.get("Name") + "\'");
        }
    }

    void fixupClassMembers(HashTree classNode)
    {
        Set<String> tagSet = new TreeSet<String>(String.CASE_INSENSITIVE_ORDER);

        ArrayList<HashToken> members =  (ArrayList<HashToken>)mTokenOwners.peek().get("Members");
        
        TdfSymbol classSymbol = (TdfSymbol) classNode.getToken();
        Boolean classTrackChanges = (Boolean)classSymbol.get("trackChanges");
        boolean trackChanges = classTrackChanges != null && classTrackChanges.booleanValue();
        
     	if (members != null)
        {
           ArrayList<HashToken> sortedList = new ArrayList<HashToken>(members);
           Collections.sort(sortedList, new ClassMemberComparator("TagValue"));
           classNode.put("SortedMembers", sortedList);

           for (int i = 0; i < members.size(); i++)
               sortedList.get(i).put("SortedMemberNumber", i);

           for (int i = 0; i < (members.size() - 1); i++)
               sortedList.get(i).put("NextMemberByTag", sortedList.get(i+1));
           
           for (int i = 0; i < members.size(); i++)
           {
                validateMemberDefault((TdfSymbol)members.get(i));
                //check for duplicate tags   
                String tag = (String) members.get(i).get("tag");
                if (tag != null)
                { 

                   if (!tagSet.contains(tag))
                   {
                       tagSet.add(tag);
                   }
                   else
                   {
                       throw new NullPointerException("Class " + classNode.get("Name") + " has multiple members with tag '" + tag + "'. Each tag can only be used once, regardless of case.");
                   }
                }
                
                //ensure that all tdf members of a tdf that has change tracking enabled also have change tracking enabled
                if (trackChanges)
                {
                    TdfSymbol memberSymbol = ((TdfSymbolRef)members.get(i).get("TypeRef")).getSymbol();
                    Boolean memberTrackChanges = (Boolean)memberSymbol.get("trackChanges");
                    if ((memberSymbol.isActualCategoryClass() || memberSymbol.isActualCategoryUnion()) &&
                    ((memberTrackChanges == null) || (!memberTrackChanges.booleanValue())))
                    {
                        throw new NullPointerException("Class " + classNode.get("Name") + " has change tracking enabled, but class declaration of member m" 
                        + members.get(i).get("Name") + " does not." + " Add attribute \'trackChanges = true\' to the definition of " + memberSymbol.get("Name"));
                    }
                }
            }
        }           
    }
    
    void validateMemberDefault(TdfSymbol member)
    {
        TdfSymbol memberSymbol = ((TdfSymbolRef)member.get("TypeRef")).getSymbol();
        Object defaultVal = member.get("default");
        if (defaultVal != null)
        {
            switch (memberSymbol.getActualCategory())
            {
                case INT_PRIMITIVE:
                {
                    if (!(defaultVal instanceof Boolean) && !(defaultVal instanceof Integer))
                    {
                        boolean isValid = false;
                        // the default is a symbol (likely a const)
                        if (defaultVal instanceof TdfSymbolRef)
                        {
                            TdfSymbol defaultSymbol = ((TdfSymbolRef)defaultVal).getActualSymbol();
                            isValid = defaultSymbol.isActualCategoryConst() && (defaultSymbol.get("IsString") == null);
                        }
                        else
                        {
                            Object isMinMax = member.get("defaultIsMinMax");
                            isValid = (isMinMax != null) && (Boolean)isMinMax;
                        }
                        
                        if (!isValid)
                        {
                            throw new NullPointerException("Member \'" + member.get("Name") + "\' in \'" + member.getParent().get("Name") + 
                                "\' with default value " + defaultVal.toString() + " must specify an integer (or boolean for bool members) as it's default value." +
                                "  Please ensure that the default value is not enclosed in quotes.");
                        }
                    }
                    break;
                }
                case FLOAT_PRIMITIVE:
                {
                    if (!(defaultVal instanceof Float))
                    {
                        throw new NullPointerException("Member \'" + member.get("Name") + "\' in \'" + member.getParent().get("Name") + 
                            "\' with default value " + defaultVal.toString() + " must specify a float as it's default value." +
                                "  Please ensure that the default value is not enclosed in quotes.");
                    }
                    else
                    {
                        //the default toString on a float in java does not include the trailing format specifier which will in turn 
                        // result in compilation errors in generated code in windows (C4305).  Here we just convert it to the string
                        // representation once it has been validated.
                        
                        member.put("default", defaultVal + "f");
                    }
                    break;
                }
                case STRING:
                {
                    if (!(defaultVal instanceof String))
                    {
                        throw new NullPointerException("Member \'" + member.get("Name") + "\' in \'" + member.getParent().get("Name") + 
                            "\' with default value " + defaultVal.toString() + " must specify a string as it's default value." +
                            "  Please ensure that the default value is enclosed in quotes.");
                    }
                    break;
                }
                case ENUM:
                 {
                    boolean isValid = false;
                    if (defaultVal instanceof TdfSymbolRef)
                    {
                        TdfSymbol defaultSymbol = ((TdfSymbolRef)defaultVal).getActualSymbol();
                        isValid = defaultSymbol.isActualCategoryEnumValue();
                    }
                    
                    if (!isValid)
                    {
                        throw new NullPointerException("Member \'" + member.get("Name") + "\' in \'" + member.getParent().get("Name") + 
                            "\' with default value " + defaultVal.toString() + " must specify an enum member as it's default value." +
                            "  Please ensure that the default value is not enclosed in quotes.");
                    }
                    break;
                }
                case TIMEVALUE:
                {
                    if (!(defaultVal instanceof Integer) && (!(defaultVal instanceof String) || ((String)defaultVal).isEmpty()))
                    {
                        throw new NullPointerException("Member \'" + member.get("Name") + "\' in \'" + member.getParent().get("Name") + 
                            "\' with default value " + defaultVal.toString() + " must specify a string as it's default value." +
                            "  Please ensure that the default value is enclosed in quotes and is parsable by TimeValue.");
                    }
                    break;
                }
                case BITFIELD:
                case BLOB:
                case VARIABLE:
                case MAP:
                case LIST:
                case TYPEDEF:
                case CONST:
                case BLAZE_OBJECT_TYPE:
                case BLAZE_OBJECT_ID:
                {
                    throw new NullPointerException("Member \'" + member.get("Name") + "\' in \'" + member.getParent().get("Name") + "\' with default value " + defaultVal.toString() +
                        " is a type that does not support default values. Please remove the default from the tdf definition");
                }
                default:
                    break;
            }
        }
    }
    
    boolean mIsCentralFile;
    final int RPC_MASK_MASTER = 0x8000;       

    void fixupSubcomponent(HashTree subNode)
    {
        HashToken compToken = getCurrentTokenOwner();
        HashToken subToken = subNode.getHashToken();
        subToken.setParentTable(compToken);
        compToken.put(subToken.getString("Type"), subToken);
        mTokenOwners.get(1).putInList("Subcomponents", subNode.getToken());
    }
    
     
    void checkUniqueIds(HashTree subNode)
    {
        checkUniqueIdsImpl(subNode, "Commands");
        checkUniqueIdsImpl(subNode, "Notifications");
        checkUniqueIdsImpl(subNode, "Events");
    }
    
    void checkUniqueIdsImpl(HashTree subNode, String collectionType)
    {
        ArrayList<HashToken> nodesToCheck = (ArrayList<HashToken>)subNode.get(collectionType);
        Set<Integer> idsInUse = new TreeSet<Integer>();
        
        if (nodesToCheck != null)
        {
            for (int i = 0; i < nodesToCheck.size(); ++i)
            {
                Integer id = nodesToCheck.get(i).getT("id");
                
                if (!idsInUse.contains(id))
                {
                    idsInUse.add(id);
                }
                else
                {
                    throw new NullPointerException("Member " + nodesToCheck.get(i).getT("Name") + " of " + collectionType + " in component " + subNode.getT("Name") + " has duplicate id " + id);
                }                
               
            }
        }        
    }
    
    void fixupCommands(HashTree subNode)
    {
        //this set stores all tdfs referenced by commands or notifications in this component, and their child tdfs.
        Set<TdfSymbolRef> allTypes = new TreeSet<TdfSymbolRef>(new SymbolMemberComparator("Name"));
        ArrayList<HashToken> commands = (ArrayList<HashToken>)mTokenOwners.peek().get("Commands");
        
        if (commands != null)
        {
            Collections.sort(commands, new HashTokenMemberComparator("Name"));

            for (int i = 0; i < commands.size(); ++i)
            {
                TdfSymbolRef request = (TdfSymbolRef)commands.get(i).getT("RequestType");
                TdfSymbolRef response = (TdfSymbolRef)commands.get(i).getT("ResponseType");
                TdfSymbolRef error = (TdfSymbolRef)commands.get(i).getT("ErrorType");
               
                addSubTypes(request, allTypes);
                addSubTypes(response, allTypes);
                addSubTypes(error, allTypes);							
            }
        }
        
        ArrayList<HashToken> notifications = (ArrayList<HashToken>)mTokenOwners.peek().get("Notifications");
        if (notifications != null)
        {
            Collections.sort(notifications, new HashTokenMemberComparator("Name"));
            
            for (int i = 0; i < notifications.size(); ++i)
            {
                TdfSymbolRef notification = (TdfSymbolRef)notifications.get(i).getT("NotificationType");                   
                addSubTypes(notification, allTypes);
            }
        }
        
        subNode.put("AllTypes", new ArrayList<TdfSymbolRef>(allTypes));
    }
    
    void addSubTypes(TdfSymbolRef node, Set<TdfSymbolRef> allTypes)
    {
        if (node != null)
        {
            TdfSymbol symbolToAdd = node.getActualSymbol();
            if ((symbolToAdd != null) && symbolToAdd.isActualCategoryClass() || symbolToAdd.isActualCategoryUnion())
            {
                if (!allTypes.contains(node))
                    allTypes.add(node);
                    
                ArrayList<HashToken> children = (ArrayList<HashToken>)node.getActualSymbol().getT("Members");
                if (children != null)
                {
                    for (int j = 0; j < children.size(); ++j)
                    {
                        TdfSymbolRef child = ((HashToken)children.get(j)).getT("TypeRef");
                        if (child != null)
                        { 
                            TdfSymbol tempSymbol = child.getActualSymbol();
                            if (tempSymbol != null)
                            {
                                if (tempSymbol.isActualCategoryMap())
                                {
                                    addSubTypes((TdfSymbolRef)tempSymbol.getT("KeyTypeRef"), allTypes);
                                    addSubTypes((TdfSymbolRef)tempSymbol.getT("ValueTypeRef"), allTypes);
                                }
                                else if (tempSymbol.isActualCategoryList())
                                {
                                    addSubTypes((TdfSymbolRef)tempSymbol.getT("ValueTypeRef"), allTypes);
                                }
                                else
                                {
                                    addSubTypes(child, allTypes);                    
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    void calcEntityIndexRange(HashTree subNode)
    {
        calcEntityIndexRangeImpl(subNode, "Commands");
        calcEntityIndexRangeImpl(subNode, "Events");
        calcEntityIndexRangeImpl(subNode, "Notifications");
    }

    void calcEntityIndexRangeImpl(HashTree subNode, String entityName)
    {
        Object nextIdxObj = mTokenOwners.get(0).get(entityName + "NextIdx");
        ArrayList<HashToken> enityList = (ArrayList<HashToken>)subNode.get(entityName);

        int startIdx = (nextIdxObj != null) ? (int)(Integer)nextIdxObj : 0;
        int endIdx = startIdx + ((enityList != null) ? enityList.size() : 0);

        if (entityName == "Commands")
        {
            // notificationSubscribe RPC
            endIdx++;
            if (subNode.get("ReplMaps") != null)
            {
                // replicationSubscribe RPC
                endIdx++;
            }
        }
        else if (entityName == "Notifications")
        {
            if (subNode.get("ReplMaps") != null)
            {
                // Replication notifications:
                //     REPLICATION_MAP_CREATE_NOTIFICATION
                //     REPLICATION_MAP_DESTROY_NOTIFICATION
                //     REPLICATION_OBJECT_UPDATE_NOTIFICATION
                //     REPLICATION_OBJECT_ERASE_NOTIFICATION
                //     REPLICATION_SYNC_COMPLETE_NOTIFICATION
                //     REPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION
                //     REPLICATION_OBJECT_INSERT_NOTIFICATION
                endIdx += 7;
            }
        }

        mTokenOwners.get(0).put(entityName + "NextIdx", endIdx);
        subNode.put(entityName + "StartIdx", startIdx);
        subNode.put(entityName + "EndIdx", endIdx);
    }

    void calcComponentId(HashTree compNode)
    {
        List<String> idList = (List<String>) compNode.get("id");
       
        int typeVal = 0;
        if (idList.get(0).equals("framework"))
        {
            typeVal = 15;
            compNode.put("IsCompTypeFramework", true);
        } 
        else if (idList.get(0).equals("core"))
        {
            typeVal = 0;
            compNode.put("IsCompTypeCore", true);
        }
        else if (idList.get(0).equals("custom"))
        {
            typeVal = 1;
            compNode.put("IsCompTypeCustom", true);
        }
        else if (idList.get(0).equals("arson"))
        {
            typeVal = 14;
            compNode.put("IsCompTypeArson", true);
        }

        int id = typeVal << 11 | Integer.parseInt(idList.get(1));
        compNode.put("CompId", id);
    }
    
    void calcSubComponentId(HashTree subNode)
    {
        Object isSlave = subNode.get("IsSlave");
        int componentId = (Integer)subNode.get("CompId");
        if((isSlave != null) && (Boolean)isSlave)
        {
            subNode.put("SubComponentId", componentId);
        }
        else
        {
            subNode.put("SubComponentId", (componentId | RPC_MASK_MASTER));
        }
    }
    
    void calcErrValue(HashTree errNode)
    {
        Integer compId = getCurrentTokenOwner().getT("CompId");
        Integer errVal = errNode.getT("Value");
                
        int fullVal = (errVal.intValue() << 16) + ((compId != null) ? compId.intValue() : 0);
        errNode.put("FullValue", fullVal);
    }

    void fixupCommandDetails(HashTree command)
    {
        Object val = command.get("details");
        if (val != null)
        {
            String details = (String)val;
            command.put("strippedDetails", details.replace('\r', ' ').replace('\n', ' '));
        }
        
        TdfSymbolRef masterCommand = (TdfSymbolRef) command.get("passthrough");
        if(masterCommand != null)
        {
            addOwnedToken("PassthroughCommands", command);
        
            List<String> masterErrors = (List<String>)masterCommand.getSymbol().get("errors");
            command.put("errors", masterErrors);
        }        
    }
    
    void fixupErrors(HashTree command)
    {
        Object errors = command.get("errors");
        if (errors != null)
        {
            ArrayList<HashToken> matchingTokens = new ArrayList<HashToken>();
            ArrayList<HashToken> tokens = (ArrayList<HashToken>)getCurrentTokenOwner().getT("Errors");
            ArrayList<String> e = (ArrayList<String>)errors;
            if (tokens != null)
            {
                for (int i = 0; i < e.size(); ++i)
                {
                    String error = e.get(i);
                    HashToken matchingToken = null;

                    for (int j = 0; j < tokens.size(); ++j)
                    {
                        if (error.equals(tokens.get(j).getT("Name")))
                        {
                            matchingToken = tokens.get(j);
                            matchingTokens.add(matchingToken);
							break;
                        }
                    }
                    if (matchingToken == null)
                    {
                        String commandName = (String)((TdfSymbol)command.getToken()).get("Name");
                        throw new NullPointerException("The error '" + error + "' on command '" + commandName + "' is not defined.");
                    }
                }
            }
            command.put("ResolvedErrors", matchingTokens);
        }
    }


    void fixupHttp(HashTree command)
    {
        ExtHashtable httpBlock = (ExtHashtable)command.get("http");
        if (httpBlock != null)
        {
            httpBlock.putIfNotExists("addEncodedPayload", true);
            String commandName = (String)((TdfSymbol)command.getToken()).get("Name");
            String responsePayload = (String)httpBlock.get("responsePayloadMember");
            if (responsePayload != null)
            {
                TdfSymbolRef responseType = (TdfSymbolRef)command.getT("ResponseType");
                
                if (responseType == null)
                {
                    throw new NullPointerException("The responsePayloadMember '" + responsePayload + "' is defined on command '" + commandName + "' which does not have a response type");
                }              
                
                ArrayList<Integer> responseTags = new ArrayList<Integer>();
                if (!fetchPayloadTags(responseType.getActualSymbol(), responsePayload, responseTags))
                {   
                    throw new NullPointerException("The responsePayloadMember '" + responsePayload + "' specified on command '" + commandName + "' does not have a corresponding tdf member");
                }
                ((TdfSymbol)command.getToken()).put("ResponsePayloadMemberTags", responseTags);
            }
            
            String requestPayload = (String)httpBlock.get("requestPayloadMember");
            if (requestPayload != null)
            {
                TdfSymbolRef requestType = (TdfSymbolRef)command.getT("RequestType");
                
                if (requestType == null)
                {
                    throw new NullPointerException("The requestPayloadMember '" + requestPayload + "' is defined on command '" + commandName + "' which does not have a request type");
                }              
                
                ArrayList<Integer> requestTags = new ArrayList<Integer>();
                if (!fetchPayloadTags(requestType.getActualSymbol(), requestPayload, requestTags))
                {
                    throw new NullPointerException("The requestPayloadMember '" + requestPayload + "' specified on command '" + commandName + "' does not have a corresponding tdf member");
                }
                ((TdfSymbol)command.getToken()).put("RequestPayloadMemberTags", requestTags);
            }

			String resource  = httpBlock.getT("resource");
			if (resource != null)
			{
				ArrayList<ExtHashtable> listOfResourceComponents = new ArrayList<ExtHashtable>();
			
				//split the resource up into a set of strings with an array of tags following it
				StringTokenizer st = new StringTokenizer(resource, "{}", true);
				ExtHashtable currentTable = new ExtHashtable();
				while (st.hasMoreElements())
				{
				    String currentSubstring = st.nextToken();
					if (currentSubstring.equals("{"))
					{
						if (st.hasMoreTokens())
						{
							currentSubstring = st.nextToken();
							if (currentSubstring.equals("{")) { throw new NullPointerException("Found a '{' inside a token specifier in resource '" + resource + "' in command '" + commandName); }				
							if (currentSubstring.equals("}")) { throw new NullPointerException("Found an empty token specifier in resource '" + resource + "' in command '" + commandName); }										
							
							currentTable.put("token", currentSubstring);
							
							TdfSymbolRef requestType = (TdfSymbolRef)command.getT("RequestType");               
							if (requestType == null)
							{
								throw new NullPointerException("The resource '" + resource + "' has request mappings, but is defined for command '" + commandName + "' which does not have a request type");
							}
							
							ArrayList<Integer> tokenTags = new ArrayList<Integer>();
							if (!fetchPayloadTags(requestType.getActualSymbol(), currentSubstring, tokenTags))
							{
								throw new NullPointerException("The token + '" + currentSubstring + "' specified in the resource string for command '" + commandName + "' does not have a corresponding tdf member");
							}	
							
							currentTable.put("token_tags", tokenTags);
							listOfResourceComponents.add(currentTable);
							currentTable = new ExtHashtable();
							
							if (!st.hasMoreTokens() || !st.nextToken().equals("}"))
							{
								throw new NullPointerException("Found an unterminated token specifier in resource '" + resource + "' in command '" + commandName);
							}							
						}
						else
						{
							throw new NullPointerException("Found an unterminated token specifier in resource '" + resource + "' in command '" + commandName);
						}
					}
					else if (currentSubstring.equals("}"))
					{
						throw new NullPointerException("Found a '}' outside of a token specifier in resource '" + resource + "' in command '" + commandName);
					}
					else
					{
					    currentTable.put("fragment", currentSubstring);
					}
				}
				
				//last table might just be a fragment
				if (!currentTable.isEmpty())
				{
				    listOfResourceComponents.add(currentTable);
				}
				
				httpBlock.put("resource_components", listOfResourceComponents);
			}

			
			evaluateHttpTagFields(httpBlock, "custom_request_headers", "Custom request headers", commandName, (TdfSymbolRef) command.getT("RequestType"), "request");
			evaluateHttpTagFields(httpBlock, "custom_response_headers", "Custom response headers", commandName, (TdfSymbolRef) command.getT("ResponseType"), "response");
			evaluateHttpTagFields(httpBlock, "custom_error_headers", "Custom error headers", commandName, (TdfSymbolRef) command.getT("ErrorType"), "error");
			evaluateHttpTagFields(httpBlock, "url_params", "Url params", commandName, (TdfSymbolRef) command.getT("RequestType"), "request");

		    ExtHashtable successCodes = httpBlock.getT("success_error_codes");
			if (successCodes != null)
			{
				TdfSymbolRef responseType = (TdfSymbolRef)command.getT("ResponseType");               
				if (responseType == null)
				{
					throw new NullPointerException("Success code mappings are defined for command '" + commandName + "' which does not have a response type");
				}         
			
				ExtHashtable newCodes = new ExtHashtable();
				for(Map.Entry<Object,Object> item : successCodes.entrySet() )
				{	
					ArrayList<Integer> codeTags = new ArrayList<Integer>();
					if (!fetchPayloadTags(responseType.getActualSymbol(), (String) item.getKey(), codeTags))
					{
						throw new NullPointerException("The success mapping for '" + item.getKey() + "' specified on command '" + commandName + "' does not have a corresponding tdf member");
					}				   
					
					newCodes.put(item.getKey(), codeTags);
				}
				httpBlock.put("success_error_codes_tags", newCodes);
				
			}

		    ExtHashtable statusCodeErrorMap = httpBlock.getT("status_code_errors");
			if (statusCodeErrorMap != null)
			{
				ExtHashtable errorMap = new ExtHashtable();
				Object errors = command.get("ResolvedErrors");
				
				if (!statusCodeErrorMap.isEmpty() && errors == null)
				{
					throw new NullPointerException("No errors defined in " + commandName + "'.");
				}
				
				for (Map.Entry<Object,Object> item : statusCodeErrorMap.entrySet())
				{			
					HashToken matchingToken = null;
					ArrayList<HashToken> e = (ArrayList<HashToken>)errors;
					ArrayList<String> s = (ArrayList<String>)item.getValue();
					for (int i = 0; i < s.size(); ++i)
					{
						for (int j = 0; j < e.size(); ++j)
						{
							String error = e.get(j).getT("Name");
							if (error.equals(s.get(i)))
							{
								matchingToken = e.get(j);
								break;
							}
						}
						if (matchingToken != null)
						{
							break;
						}
					}
					if (matchingToken == null)
					{
						throw new NullPointerException("The error '" + item.getValue() + "' in http status error mapping of command '" + commandName + "' is not defined.");
					}
					errorMap.put(item.getKey(), matchingToken);
				}				
				httpBlock.put("status_code_errors_tags", errorMap);
            }		
        }
    }
    
	void evaluateHttpTagFields(ExtHashtable httpBlock, String attribName, String niceName, String commandName, TdfSymbolRef baseTdf, String tdfType)
	{
		ExtHashtable headers = httpBlock.getT(attribName);
		if (headers != null)
		{
			if (baseTdf == null)
			{
				throw new NullPointerException(niceName + " are specified for command '" + commandName + "' which does not have a " + tdfType + " type");
			}

			ExtHashtable newHeaders = new ExtHashtable();
			for(Map.Entry<Object,Object> item : headers.entrySet() )
			{	
				ArrayList<Integer> headerTags = new ArrayList<Integer>();
				if (!fetchPayloadTags(baseTdf.getActualSymbol(), (String) item.getValue(), headerTags))
				{
					throw new NullPointerException("The value + '" + item.getValue() + "' for header/param '" + item.getKey() + "' specified on command '" + commandName + "' does not have a corresponding tdf member");
				}				   
				
				newHeaders.put(item.getKey(), headerTags);
			}
			httpBlock.put(attribName + "_tags", newHeaders);
		}		
	}	
	
	
	
    boolean fetchPayloadTags(TdfSymbol rootSymbol, String payload, ArrayList<Integer> responseTags)
    {
		String logString = "";
	
        TdfSymbol symbol = rootSymbol;
        String[] split = payload.split("\\.");
        boolean foundMatch = false;
        ArrayList<Integer> tempTags = new ArrayList<Integer>();
        
        for (int i = 0; i < split.length; ++i)
        {
			logString += logString + "Looking for member " + split[i] + " in class " + symbol.getName() + "\n";
		
            if (foundMatch)
            {
                break;
            }
            ArrayList<TdfSymbol> members = (ArrayList<TdfSymbol>)symbol.get("Members");

            if ((members != null) && (members.size() > 0))
            {
                for (int j = 0; j < members.size(); ++j)
                {
                    TdfSymbol currentMember = members.get(j);
                    String memberName = (String)currentMember.get("Name");								

					logString += "...checking member " + memberName + "\n";
				
				
                    if (split[i].equalsIgnoreCase(memberName))
                    {
                        symbol = ((TdfSymbolRef)currentMember.get("TypeRef")).getSymbol();
                        tempTags.add((Integer)currentMember.get("TagValue"));
                        if (i == (split.length - 1))
                        {
                            foundMatch = true;
                            for (int k = 0; k < tempTags.size(); ++k)
                            {
                                responseTags.add(tempTags.get(k));
                            }
                        }
                        break;
                    }
                }
            }
        }
		
		if (!foundMatch)
			System.out.println(logString);
		
        return foundMatch;
    }
	
	
}

/* ================================================================================
                                          Highest Level 
   ================================================================================*/
//TODO file params
file 	[TdfSymbolTable symbolTable, boolean isCentralFile]
        @init { mSymbolTable = symbolTable; mIsCentralFile = isCentralFile; }
        :      file_def
        ;
        
/* ================================================================================
                                          Main Elements
   ================================================================================*/
file_def 
        @init { 
        	pushTokenOwnersStack(); 
        	pushTokenOwner(mSymbolTable.getRootSymbol());
        }
        @after { popTokenOwner(); popTokenOwnersStack(); }
        :       ^(FILE { pushTokenOwner($FILE); }file_member*)
        ;

file_member
        : namespace_member 
        | include_block 
        | group_block
        ;
        
include_block 
        : ^( INCLUDE
        {        
            addOwnedToken("Defs", $INCLUDE);
            addOwnedToken("Includes", $INCLUDE);
        }  file_def ) -> INCLUDE
        | INCLUDE
        {
            addOwnedToken("Defs", $INCLUDE);
            addOwnedToken("Includes", $INCLUDE);
        }
        | FAKE_INCLUDE
        { 
            addOwnedToken("Defs", $FAKE_INCLUDE);
            addOwnedToken("Includes", $FAKE_INCLUDE);
        }
        ;

namespace_block 
        : ^(NAMESPACE 
        {
           addOwnedToken("Defs", $NAMESPACE); 
           addOwnedToken("Namespaces", $NAMESPACE); 
           pushTokenOwner($NAMESPACE);
        } namespace_member*)
        { popTokenOwner(); }
        ;
        
namespace_member
        : namespace_block
        | type_definition_block 
        | root_component_block
        | component_block
		| custom_attribute_block
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
        ;

custom_attribute_block	
	: ^(CUSTOM_ATTRIBUTE 	
		{	  	
			addOwnedToken("Defs", $CUSTOM_ATTRIBUTE);
			addOwnedToken("CustomAttribute", $CUSTOM_ATTRIBUTE);		
			addSelfTyperef($CUSTOM_ATTRIBUTE);
			pushTokenOwner($CUSTOM_ATTRIBUTE);
		} custom_attribute_member*)
	{
	    ArrayList<HashToken> members =  (ArrayList<HashToken>)mTokenOwners.peek().get("Members");
            if (members != null)
	    {
                 ArrayList<HashToken> sortedList = new ArrayList<HashToken>(members);
                 Collections.sort(sortedList, new CustomAttributeMemberInstanceComparator("Name"));
                 $CUSTOM_ATTRIBUTE.put("Members", sortedList);
	    }
	    popTokenOwner();
	}		
    ;

custom_attribute_member
	: ^(MEMBER_DEF custom_attribute_type)
       { 
           addOwnedToken("Members", $MEMBER_DEF);            
       }
	;


custom_attribute_type
	: INT_PRIMITIVE 
    | CHAR_PRIMITIVE
	| STRING;

class_fwd_decl_block
        : CLASS_FWD_DECL
         { 
           addOwnedToken("Defs", $CLASS_FWD_DECL); 
           addSelfTyperef($CLASS_FWD_DECL);
         }
        ;

class_block
        :        ^(CLASS
        	{	  	
                        addOwnedToken("Defs", $CLASS);
                        addOwnedToken("Classes", $CLASS);
                        addSelfTyperef($CLASS);
                        pushTokenOwner($CLASS);
	        } (type_definition_block | class_member)*)
	        {
         	        fixupClassMembers($CLASS);
                        
        	        popTokenOwner();
	        }
        ;

class_member
        scope { HashToken t; }
        :       ^(MEMBER_DEF  {$class_member::t = $MEMBER_DEF.getHashToken(); } type)                 
                { 
                    addOwnedToken("Members", $MEMBER_DEF);                
                }
        ;

union_block
	 @after { popTokenOwner(); }
        :       ^(UNION 
        	{
        	    addOwnedToken("Defs", $UNION);
                    addOwnedToken("Unions", $UNION);
                    addSelfTyperef($UNION);
                    pushTokenOwner($UNION); 
        	}
        	union_member*)
            {
                fixupClassMembers($UNION);
            }
        ;

union_member
        :       ^(MEMBER_DEF type) 
               { 
                   addOwnedToken("Members", $MEMBER_DEF); 
               }
        ;
        

typedef_block
        :       ^(TYPEDEF type) 
                { 
                    addOwnedToken("Defs", $TYPEDEF); 
                }
        ;       
        
const_block
        :       CONST { addOwnedToken("Defs", $CONST); }
        |       STRING_CONST { addSelfTyperef($STRING_CONST); addOwnedToken("Defs", $STRING_CONST); }
        ;
        

bitfield_block
        @after { popTokenOwner(); }
        :       ^(BITFIELD 
       		{
                   addOwnedToken("Defs", $BITFIELD); 
                   pushTokenOwner($BITFIELD); 
                } (MEMBER_DEF { addOwnedToken("Members", $MEMBER_DEF); })*) 
        ;
                                      
        
enum_block     
        @after { popTokenOwner(); }
        :       ^(ENUM
                { 
                                  addOwnedToken("Defs", $ENUM);
                  addSelfTyperef($ENUM);
                  pushTokenOwner($ENUM); 
                } (MEMBER_DEF 
                { 
                    addOwnedToken("Members", $MEMBER_DEF); 
                    addSelfTyperef($MEMBER_DEF);
                })*) 
        ;

/* ================================================================================
                                        RPC  Types
   ================================================================================*/

root_component_block 
	: ^(ROOTCOMPONENT root_component_members*)
	;
	
root_component_members
	: ^(ERRORS (ERROR 
        { 
                addOwnedToken("Errors", $ERROR, 1); 
                calcErrValue($ERROR);
        } )*)
	|  ^(SDK_ERRORS (ERROR { addOwnedToken("SdkErrors", $ERROR, 1); calcErrValue($ERROR); })*)
  | ^(PERMISSIONS (PERMISSION 
        { 
                addOwnedToken("permissions", $PERMISSION, 1); 
                calcErrValue($PERMISSION);
        } )*)
        ;
        

component_block    
        @after { popTokenOwner(); }   
        : ^(COMPONENT 
                {                 
                  calcComponentId($COMPONENT);
                  addOwnedToken("Components", $COMPONENT, 1); 
                  pushTokenOwner($COMPONENT);
                } component_members*)               
        ;
        
        
component_members
        : subcomponent_block
        | errors_block
        | TYPE { addOwnedToken("Types", $TYPE); }
        | permissions_block
        ;
        
subcomponent_block
        scope NotificationIndexer;
        @init { $NotificationIndexer::i = 0; }
        @after { popTokenOwner(); }
        : ^(SLAVE  { fixupSubcomponent($SLAVE);  calcSubComponentId($SLAVE); pushTokenOwner($SLAVE);  } subcomponent_members*) 
                   {
                        checkUniqueIds($SLAVE);
                        fixupCommands($SLAVE);
                        if (mIsCentralFile) 
                            calcEntityIndexRange($SLAVE); 
                    }
        | ^(MASTER { fixupSubcomponent($MASTER); calcSubComponentId($MASTER); pushTokenOwner($MASTER); } subcomponent_members*) 
                   {
                        checkUniqueIds($MASTER);
                        if (mIsCentralFile) 
                            calcEntityIndexRange($MASTER); 
                   }
        ;
        
subcomponent_members
        : COMMAND {addOwnedToken("Commands", $COMMAND);  fixupCommandDetails($COMMAND); fixupErrors($COMMAND); fixupHttp($COMMAND);}
        | NOTIFICATION { addOwnedToken("Notifications", $NOTIFICATION); $NOTIFICATION.put("HandlerIndex", $NotificationIndexer::i++); }
        | EVENT { addOwnedToken("Events", $EVENT); }
        | DYNAMIC_MAP 
            {
                addOwnedToken("ReplMaps", $DYNAMIC_MAP); 
                addOwnedToken("DynamicMaps", $DYNAMIC_MAP); 
                verifyReplicatedMapTypes($DYNAMIC_MAP);
            }
        | STATIC_MAP 
            {
                addOwnedToken("ReplMaps", $STATIC_MAP); 
                addOwnedToken("StaticMaps", $STATIC_MAP); 
                verifyReplicatedMapTypes($STATIC_MAP);
            }
        ;
        
errors_block
        : ^(ERRORS (ERROR 
        {
                addOwnedToken("Errors", $ERROR); 
                calcErrValue($ERROR);
        } )*)
        ;
        

sdk_errors_block
        : ^(SDK_ERRORS (ERROR { addOwnedToken("SdkErrors", $ERROR); })*)
        ;
        
permissions_block
        : ^(PERMISSIONS (PERMISSION 
        { 
                addOwnedToken("permissions", $PERMISSION); 
                calcErrValue($PERMISSION);
        } )*)
        ;
        

/* ================================================================================
                                          Types
   ================================================================================*/
type    :   non_collection_type
        |   ^(MAP non_collection_type type) -> MAP
        |   ^(LIST type) -> LIST
        ;

non_collection_type 
        :   INT_PRIMITIVE 
        |   CHAR_PRIMITIVE
        |   FLOAT_PRIMITIVE
        |   BLAZE_OBJECT_TYPE
        |   BLAZE_OBJECT_ID
        |   BLAZE_OBJECT_PRIMITIVE 
        |   TIMEVALUE 
        |   BLOB
        |   VARIABLE 
            {
                if ($class_member.size() > 0)
                {                
                    addOwnedToken("VarMembers", $class_member::t);
                }
            }
        |   STRING
            {
        	    TdfSymbol genConst = (TdfSymbol) $STRING.get("GenConst");
        	    if (genConst != null)
        	    {
 			        addOwnedToken("Defs", genConst);
 		        }
            }

        |   TYPE_REF
        ;
