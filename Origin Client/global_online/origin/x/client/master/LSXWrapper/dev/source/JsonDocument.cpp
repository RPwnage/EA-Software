#include "JsonDocument.h"

INodeDocument * CreateJsonDocument()
{
    return new JsonDocument();
}

JsonDocument::JsonDocument() :
    currentPair(-1),
    currentAttribute(-1),
    currentIndex(-1)
{

}

// Parse data from string.
bool JsonDocument::Parse(char *pSource)
{
    return doc.parse(pSource) >= 0;
}

bool JsonDocument::Parse(const char *pSource)
{
    return doc.parse(pSource) >= 0;
}

libjson::json_object * JsonDocument::CurrentObject()
{
    if(parents.size() == 0)
    {
        return doc.object;
    }
    else
    {
        libjson::json_pair *pair = CurrentPair();

        if(pair)
        {
            if(pair->value.isObject)
            {
                return &(pair->value.object);
            }
            else if(pair->value.isArray)
            {
                if(currentIndex>=0 && currentIndex<pair->value.array.size())
                {
                    pair->value.array[currentIndex]->isObject = true;
                    return &pair->value.array[currentIndex]->object;
                }
            }
        }
    }
    return NULL;
}

libjson::json_pair * JsonDocument::CurrentPair()
{
    if(parents.size() > 0 && currentPair>=0 && currentPair<parents.back()->pairs.size())
    {
        return parents.back()->pairs[currentPair];
    }

    return NULL;
}

libjson::json_pair * JsonDocument::CurrentAttribute()
{
    libjson::json_object *currentObject = CurrentObject();

    if(currentObject != NULL)
    {
        return currentObject->pairs[currentAttribute];
    }
    return NULL;
}



// Functions for navigating
bool JsonDocument::Root()
{
    parents.clear();
    currentPair = -1;
    currentIndex = -1;
    return true;
}

bool JsonDocument::FirstChild()
{
    // Find the first child object
    libjson::json_object *newParent = CurrentObject();

    currentIndex = -1;
    currentPair = -1;

    if(newParent != NULL)
    {
        parents.push_back(newParent);

        // Go through all the pairs to find the first child object.
        for(size_t i=0; i<newParent->pairs.size(); i++)
        {
            libjson::json_pair *pair = newParent->pairs[i];

            if(pair->value.isObject || pair->value.isArray)
            {
                currentPair = i;
                return true;
            }
        }
    }
    return false;
}

bool JsonDocument::FirstChild(const char *pName)
{
    // Find the first child object
    libjson::json_object *newParent = CurrentObject();

    currentIndex = -1;
    currentPair = -1;

    if(newParent != NULL)
    {
        parents.push_back(newParent);

        std::string name = '\"' + std::string(pName) + '\"';

        // Go through all the pairs to find the first child object.
        for(size_t i=0; i<newParent->pairs.size(); i++)
        {
            libjson::json_pair *pair = newParent->pairs[i];

            if(/*(pair->value.isObject || pair->value.isArray) &&*/ (pair->name.toString(doc.content) == name))
            {
                currentPair = i;
                return true;
            }
        }
    }
    return false;
}

bool JsonDocument::NextChild()
{
    // Find the first child object
    libjson::json_object *parent = parents.back();

    currentIndex = -1;

    if(parent != NULL)
    {
        // Go through all the pairs to find the first child object.
        for(size_t i=currentPair + 1; i<parent->pairs.size(); i++)
        {
            libjson::json_pair *pair = parent->pairs[i];

            if(pair->value.isObject)
            {
                currentPair = i;
                return true;
            }
        }
        currentPair = -1;
    }
    return false;
}

bool JsonDocument::NextChild(const char *pName)
{
    // Find the first child object
    libjson::json_object *current = CurrentObject();

    currentIndex = -1;

    if(current != NULL)
    {
        std::string name = '\"' + std::string(pName) + '\"';

        // Go through all the pairs to find the first child object.
        for(size_t i=currentPair + 1; i<current->pairs.size(); i++)
        {
            libjson::json_pair *pair = current->pairs[i];

            if(pair->value.isObject && (pair->name.toString(doc.content) == name))
            {
                currentPair = i;
                return true;
            }
        }

        currentPair = -1;
    }
    return false;
}

bool JsonDocument::Parent()
{
    if(parents.size()>1)
    {
        libjson::json_pair *pair = CurrentPair();

        // Are we pointing onto an array object.
        if(pair && pair->value.isArray)
        {
            // Point to the array.
            currentIndex = -1;
        }
        else
        {
            libjson::json_object * child = parents.back();

            parents.pop_back();

            libjson::json_object * parent = parents.back();

            // Find the previous parent in the current parent.
            for(size_t i=0; i<parent->pairs.size(); i++)
            {
                libjson::json_pair * pair = parent->pairs[i];

                if(pair->value.isArray)
                {
                    for(int j=0; j<pair->value.array.size(); j++)
                    {
                        libjson::json_value * obj = pair->value.array[j];

                        if(obj && obj->isObject && &obj->object == child)
                        {
                            currentPair = i;
                            currentIndex = j;
                            return true;
                        }
                    }
                }
                else
                {
                    if(&pair->value.object == child)
                    {
                        currentPair = i;
                        currentIndex = -1;
                        return true;
                    }
                }
            }
        }
    }
    else if(parents.size() == 1)
    {
        Root();
    }
    return false;
}

bool JsonDocument::FirstAttribute()
{
    // Find the first child object
    libjson::json_object *current = CurrentObject();

    if(current != NULL)
    {
        // Go through all the pairs to find the first child object.
        for(size_t i=0; i<current->pairs.size(); i++)
        {
            libjson::json_pair *pair = current->pairs[i];

            if(!pair->value.isObject && !pair->value.isArray)
            {
                currentAttribute = i;
                return true;
            }
        }
    }
    return false;
}

bool JsonDocument::NextAttribute()
{
    // Find the first child object
    libjson::json_object *current = CurrentObject();

    if(current != NULL)
    {
        // Go through all the pairs to find the first child object.
        for(size_t i=currentAttribute + 1; i<current->pairs.size(); i++)
        {
            libjson::json_pair *pair = current->pairs[i];

            if(!pair->value.isObject && !pair->value.isArray)
            {
                currentAttribute = i;
                return true;
            }
        }
    }
    return false;
}

bool JsonDocument::GetAttribute(const char* pName)
{
    // Find the first attribute
    libjson::json_object *current = CurrentObject();

    if(current != NULL)
    {
        std::string name = '\"' + std::string(pName) + '\"';

        // Go through all the pairs to find the attribute.
        for(size_t i=0; i<current->pairs.size(); i++)
        {
            libjson::json_pair *pair = current->pairs[i];

            if(!pair->value.isObject && !pair->value.isArray && (pair->name.toString(doc.content) == name))
            {
                currentAttribute = i;
                return true;
            }
        }
    }
    return false;
}

bool JsonDocument::FirstValue()
{
    libjson::json_pair *pair = CurrentPair();

    if(pair && pair->value.isArray && pair->value.array.size()>0)
    {
        currentIndex = 0;
        return true;
    }
    currentIndex = -1;
    return false;
}

bool JsonDocument::NextValue()
{
    libjson::json_pair *pair = CurrentPair();

    if(pair && pair->value.isArray && currentIndex>=0 && currentIndex<pair->value.array.size()-1)
    {
        currentIndex++;
        return true;
    }
    currentIndex = -1;
    return false;
}

// Functions for querying current state.
const char * JsonDocument::GetName()
{
    libjson::json_pair *pair = CurrentPair();

    if(pair != NULL)
    {
        temp = pair->name.toString(doc.content);

        if(temp.length()>=2 && temp[0] == '\"')
        {
            // Strip the leading, and trailing quotes.
            temp = temp.substr(1, temp.length()-2);
        }

        return temp.c_str();
    }
    return "";
}

const char * JsonDocument::GetValue()
{
    libjson::json_pair *pair = CurrentPair();

    if(pair != NULL)
    {
        temp.clear();

        if(pair->value.isArray)
        {
            if(currentIndex>=0 && currentIndex<pair->value.array.size())
            {
                temp = pair->value.array[currentIndex]->value.toString(doc.content);
            }
        }
        else
        {
            temp = pair->value.toString(doc.content);
        }

        if(temp.length()>=2 && temp[0] == '\"')
        {
            // Strip the leading, and trailing quotes.
            temp = temp.substr(1, temp.length()-2);
        }
        return temp.c_str();
    }
    return "";
}

const char * JsonDocument::GetAttributeValue(const char* name)
{
    if(GetAttribute(name))
    {
        libjson::json_pair *pair = CurrentAttribute();

        temp = pair->value.toString(doc.content);

        if(temp.length()>=2 && temp[0] == '\"')
        {
            // Strip the leading, and trailing quotes.
            temp = temp.substr(1, temp.length()-2);
        }

        return temp.c_str();
    }
    return "";
}

const char * JsonDocument::GetAttributeName()
{
    libjson::json_pair *pair = CurrentAttribute();

    if(pair != NULL)
    {
        temp = pair->name.toString(doc.content);

        if(temp.length()>=2 && temp[0] == '\"')
        {
            // Strip the leading, and trailing quotes.
            temp = temp.substr(1, temp.length()-2);
        }
        return temp.c_str();
    }
    return "";
}

const char * JsonDocument::GetAttributeValue()
{
    libjson::json_pair *pair = CurrentAttribute();

    if(pair != NULL)
    {
        temp = pair->value.toString(doc.content);

        if(temp.length()>=2 && temp[0] == '\"')
        {
            // Strip the leading, and trailing quotes.
            temp = temp.substr(1, temp.length()-2);
        }
        return temp.c_str();
    }
    return "";
}

// Functions for creating Nodes

bool JsonDocument::EnterArray(const char *pName)
{
    // Find the first child object
    libjson::json_object *newParent = CurrentObject();

    if(newParent != NULL)
    {
        parents.push_back(newParent);

        std::string name = '\"' + std::string(pName) + '\"';

        // Go through all the pairs to find the first child object.
        for(size_t i = 0; i < newParent->pairs.size(); i++)
        {
            currentPair = i;
            {
                libjson::json_pair *pair = newParent->pairs[i];

                if(pair->value.isArray && pair->name.toString(doc.content) == name)
                {
                    currentIndex = -1;
                    currentAttribute = -1;
                    return true;
                }
            }
        }
        Parent();
    }
    return true;
}

bool JsonDocument::AddArray( const char *Name )
{
    libjson::json_object *newParent = CurrentObject();

    if(newParent != NULL)
    {
        parents.push_back(newParent);

        libjson::json_pair * newChild = new libjson::json_pair();
        newChild->name.set_value(&doc, Name);
        newChild->value.isArray = true;

        currentPair = newParent->pairs.size();
        newParent->pairs.push_back(newChild);

        currentIndex = -1;
        currentAttribute = -1;

        return true;
    }

    return false;
}

bool JsonDocument::LeaveArray()
{
    libjson::json_pair * array = CurrentPair();

    if(array && array->value.isArray)
    {
        currentPair = -1;
        return true;
    }
    return false;
}

bool JsonDocument::AddChild(const char *Name)
{
    libjson::json_pair * pair = CurrentPair();

    // If the current pair is an array, but we haven't a current index, add the child as an array element.
    if(pair && pair->value.isArray /*&& currentIndex == -1*/)
    {
        libjson::json_value* newValue = new libjson::json_value();
        
        currentIndex = pair->value.array.size();
        pair->value.array.push_back(newValue);

        return true;
    }
    //else if(pair && pair->value.isArray)  // If the current pair is an array, and we have a currentIndex add the object as a value to the array element.
    //{
    //    libjson::json_object *newParent = CurrentObject();

    //     if(newParent)
    //     {
    //         // But the array on the stack
    //         parents.push_back(newParent);

    //         libjson::json_value * val = pair->value.array[currentIndex];

    //         val->isObject = true;

    //         currentPair = -1;

    //         return true;
    //     }
    //}
    else
    {
        libjson::json_object *newParent = CurrentObject();

        if(newParent != NULL)
        {
            parents.push_back(newParent);

            libjson::json_pair * newChild = new libjson::json_pair();
            newChild->name.set_value(&doc, Name);
            newChild->value.isObject = true;

            currentPair = newParent->pairs.size();
            newParent->pairs.push_back(newChild);

            return true;
        }
    }

    return false;
}

bool JsonDocument::AddChild(const char *Name, const char *Value)
{
    if(AddChild(Name))
        if(SetValue(Value))
            return true;

    return false;
}

bool JsonDocument::AddAttribute(const char *Name, const char *Value)
{
    libjson::json_object *current = CurrentObject();

    if(current != NULL)
    {
        libjson::json_pair * newAttribute = new libjson::json_pair();
        newAttribute->name.set_value(&doc, Name);
        newAttribute->value.set_value(&doc, Value);

        currentAttribute = current->pairs.size();
        current->pairs.push_back(newAttribute);
        return true;
    }
    
    return false;
}

bool JsonDocument::SetValue(const char *Value)
{
    libjson::json_pair *pair = CurrentPair();

    if(pair && pair->value.isArray)
    {
        libjson::json_value *val = pair->value.array[currentIndex];

        val->value.set_value(&doc, Value);
    }
    else
    {
        if(pair != NULL)
        {
            pair->value.set_value(&doc, Value);
            return true;
        }
    }
    return false;
}


bool JsonDocument::AddDocument(INodeDocument *pDoc)
{
	return false;
}


// Parse Nodes to string.
QByteArray JsonDocument::Print() const
{
    return doc.toString().c_str();
}

// Dispose of the NodeDocument
void JsonDocument::Release()
{
    delete this;
}



