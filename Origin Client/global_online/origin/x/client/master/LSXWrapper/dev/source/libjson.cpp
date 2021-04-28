#include "libjson.h"
#include <QString>
#include <QByteArray>
#include <string.h>
#include <iostream>

using namespace libjson;

int find_char(char ch, const char *pcontent, int pos)
{
	if(pos<0) return pos;

	char c;
	while((c=pcontent[pos]) != 0)
	{
		// skip white space
		if(c <= ' ')
		{
			pos++;
			continue;
		}

		if(c == ch)
			return pos;
		else
			return NOT_FOUND;
	}
	return UNEXPECTED_END_OF_FILE;
}
int find_any_char(const char * string, const char *pcontent, int pos) 
{
	if(pos<0) return pos;

	size_t len = strlen(string);

	char c;
	while((c=pcontent[pos]) != 0)
	{
		// skip white space
		if(c <= ' ')
		{
			pos++;
			continue;
		}

		// Match any of the characters.
		for(size_t i=0; i<len; i++)
		{
			if(c == string[i])
				return pos;
		}
		return NOT_FOUND;
	}
	return UNEXPECTED_END_OF_FILE;
}
int find_first_none_whitespace(const char *pcontent, int pos)
{
	if(pos<0) return pos;

	char c;
	while((c=pcontent[pos]) != 0)
	{
		// skip white space
		if(c <= ' ')
		{
			pos++;
			continue;
		}

		// first none white space.
		return pos;
	}
	return UNEXPECTED_END_OF_FILE;
}


int json_string::parse_string(const char *pcontent, int pos)
{
	if(pos<0)
		return pos;

	if(pcontent[pos] != '\"')
		return UNEXPECTED_CHARACTER;

	pos++;

	enum {SKIPPING_ESCAPE_CHAR, SKIPPING_ESCAPE_UNICODE_CHAR, FINDING_TRAILING_QUOTE} state = FINDING_TRAILING_QUOTE;
	int unicode = 0;
	char c;

	while((c=pcontent[pos]) != 0)
	{
		switch(state)
		{
		case FINDING_TRAILING_QUOTE:
			if(c == '\"')
				return pos;
			else if(c== '\\')
				state = SKIPPING_ESCAPE_CHAR;
			break;

		case SKIPPING_ESCAPE_CHAR:
			if(c == 'u')
			{
				unicode = 4;
				state = SKIPPING_ESCAPE_UNICODE_CHAR;
			}
			else
			{
				state = FINDING_TRAILING_QUOTE;
			}
			break;

		case SKIPPING_ESCAPE_UNICODE_CHAR:
			unicode--;
			if(unicode == 0)
				state = FINDING_TRAILING_QUOTE;
			break;
		}
		pos++;
	}

	return UNEXPECTED_END_OF_FILE;
}

int json_string::parse_token(const char *pcontent, int pos)
{
	if(pos<0)
		return pos;

	char c;
	while((c=pcontent[pos]) != 0)
	{
		if(c == ',' || c == '}' || c==']')
		{
			pos--;

			while((c=pcontent[pos]) <= ' ' && pos>start)
				pos--;

			return pos;
		}

		pos++;
	}

	return UNEXPECTED_END_OF_FILE;
}

int json_string::parse(const char * pcontent, int pos, bool hasToBeString)
{
	if(pos<0)
		return pos;

	// this is either a string, or a bool, or a null, or a number.
	pos = find_first_none_whitespace(pcontent, pos);

	if(pos<0)
		return pos;

	start = pos;

	if(pcontent[start] == '\"')
	{
		is_string = true;
		pos = parse_string(pcontent, pos);

		if(pos<0)
			return pos;

		end = pos + 1;

#ifdef _DEBUG
		debug_value.append(pcontent + start, pcontent + end);
#endif
		return pos;
	}
	else 
	{
		if(!hasToBeString)
		{
			is_string = false;

			pos = parse_token(pcontent, pos);

			if(pos<0)
				return pos;

			end = pos + 1;

#ifdef _DEBUG
		debug_value.append(pcontent + start, pcontent + end);
#endif
			return pos;
		}
		else
		{
			return UNEXPECTED_TOKEN;
		}
	}
}

void json_string::set_value(json * doc, const char *pvalue)
{
#ifdef _DEBUG
    debug_value = pvalue; 
#endif

    // This needs an utf8toJson conversion.

    doc->add_string(pvalue, start, end);
}


std::string json_string::cvrtToUtf8(const std::string &src) const
{
    QByteArray ret = src.c_str();

    for(int i=0; i<ret.length(); i++)
    {
        if(ret[i] == '\\')
        {
            i++;
            switch(ret[i])
            {
            case '\"': { QByteArray left = ret.left(i-1); QByteArray right = ret.right(ret.length() - i - 1); ret = left + '\"' + right; } break;
            case '\\': { QByteArray left = ret.left(i-1); QByteArray right = ret.right(ret.length() - i - 1); ret = left + '\\' + right; } break;
            case 'b' : { QByteArray left = ret.left(i-1); QByteArray right = ret.right(ret.length() - i - 1); ret = left + '\b' + right; } break;
            case 'f' : { QByteArray left = ret.left(i-1); QByteArray right = ret.right(ret.length() - i - 1); ret = left + '\f' + right; } break;
            case 'n' : { QByteArray left = ret.left(i-1); QByteArray right = ret.right(ret.length() - i - 1); ret = left + '\n' + right; } break;
            case 'r' : { QByteArray left = ret.left(i-1); QByteArray right = ret.right(ret.length() - i - 1); ret = left + '\r' + right; } break;
            case 't' : { QByteArray left = ret.left(i-1); QByteArray right = ret.right(ret.length() - i - 1); ret = left + '\t' + right; } break;
            case 'u' : 
                { 
                    QByteArray left = ret.left(i-1); 
                    QByteArray right = ret.right(ret.length() - i - 5); 
                    
                    int d1 = ret[++i]; d1 = d1>0x39 ? d1 - 0x41 : d1 - 0x30;
                    int d2 = ret[++i]; d2 = d2>0x39 ? d2 - 0x41 : d2 - 0x30;
                    int d3 = ret[++i]; d3 = d3>0x39 ? d3 - 0x41 : d3 - 0x30;
                    int d4 = ret[++i]; d4 = d4>0x39 ? d4 - 0x41 : d4 - 0x30;

                    int val = (((d1)*16 + d2)*16 + d3)*16 + d4;

                    QString ch;
                    ch.append(val);

                    ret = left + ch.toUtf8() + right; 
                } 
                break;
            }
        }
    }

    return std::string(ret.data());
}

std::string json_string::toString(const char *pcontent, bool unmodified) const
{
	std::string ret;
	ret.append(pcontent + start, pcontent + end);
    if(!unmodified)
    {
        ret = cvrtToUtf8(ret);
    }

	return ret;
}

json_value::~json_value()
{
    for(size_t i=0; i<array.size(); i++)
    {
        delete array[i];
    }
}

int json_value::parse_array(const char * pcontent, int pos)
{
	if(pos<0)
		return pos;

	if(pcontent[pos] != '[')
		return UNEXPECTED_CHARACTER;

	pos++;
	
	while(pos >= 0)
	{
		int p = find_char(']', pcontent, pos);

		// Found the closing bracket or an error.
		if(p != NOT_FOUND)
			return p;

		json_value * val = new json_value();

		p = val->parse(pcontent, pos);

		if(p<0)
        {
            delete val;
			return p;
        }

		array.push_back(val);

		pos = find_char(',', pcontent, p + 1);

		if(pos>=0)
		{
			// Skip the comma.
			pos++;
		}
		else
		{
			if(pos == NOT_FOUND)
			{
				pos = p + 1;
			}
			else
			{
				return pos;
			}
		}
	}
	return pos;
}


int json_value::parse(const char * pcontent, int pos)
{
	if(pos<0)
		return pos;

	pos = find_first_none_whitespace(pcontent, pos);

	if(pos<0)
		return pos;

	switch(pcontent[pos])
	{
	case '[':
		isArray = true;
		pos = parse_array(pcontent, pos);
		break;
	case '{':
		isObject = true;
		pos = object.parse(pcontent, pos);
		break;
	default:
		pos = value.parse(pcontent, pos, false);
		break;
	}

	return pos;
}

std::string json_value::toString(const char *pcontent, bool unmodified) const
{
	if(isArray)
	{
		std::string ret;

		ret = '[';

		for(size_t i=0; i<array.size(); i++)
		{
			if(i!=0)
				ret += ',';

            ret += array[i]->toString(pcontent, unmodified);
		}

		ret += ']';

		return ret;
	}
	else if(isObject)
	{
        return object.toString(pcontent, unmodified);
	}
	else
	{
		return value.toString(pcontent, unmodified);
	}
}

void json_value::set_value(json *doc, const char *pvalue)
{
    isArray = false;
    isObject = false;
    value.set_value(doc, pvalue);
}

int json_pair::parse(const char * pcontent, int pos)
{
	pos = name.parse(pcontent, pos);

	if(pos<0)
		return pos;

	pos = find_char(':', pcontent, pos + 1);

	if(pos<0)
		return pos;

	pos = value.parse(pcontent, pos + 1);

	return pos;
}

std::string json_pair::toString(const char *pcontent, bool unmodified) const
{
    return name.toString(pcontent, unmodified) + ':' + value.toString(pcontent, unmodified);
}

int json_object::parse(const char *pcontent, int pos)
{
	// find opening bracket
	pos = find_char('{', pcontent, pos);
	if(pos<0) return pos;

	pos++;

	while(pos>=0)
	{
		// check if we are at the end of the collection
		int p = find_char('}', pcontent, pos);

		// Found the closing bracket or an error.
		if(p != NOT_FOUND)
			return p;

		json_pair * pair = new json_pair();
        
		p = pair->parse(pcontent, pos);

		if(p>=0)
		{
			pairs.push_back(pair);
			pos = p + 1;

			// check if there are any more pairs.
			p = find_char(',', pcontent, pos);

			// No, this was the last one.
			if(p == NOT_FOUND)
				break; 

			// Check whether an error occured.
			if(p<0)
				return p;

			// skip the comma
			pos = p + 1;
		}
		else
		{
			// You only end up here if there is an error.
            delete pair;
			return p;
		}
	}

	// find closing bracket
	return find_char('}', pcontent, pos);
}

json_object::~json_object()
{
    for(size_t i=0; i<pairs.size(); i++)
    {
        delete pairs[i];
    }
}

std::string json_object::toString(const char *pcontent, bool unmodified) const
{
	std::string ret;

	ret = '{';

	for(size_t i=0; i<pairs.size(); i++)
	{
		if(i>0)
			ret += ',';

        ret += pairs[i]->toString(pcontent, unmodified);
	}

	ret += '}';

	return ret;
}

json::json() :
    object(new json_object()),
    content(NULL)
{
    
}

json::~json()
{
    delete object;
}


int json::parse(const char *pcontent)
{
	content = pcontent;
	return object->parse(content, 0);
}

std::string json::toString() const
{
	return object->toString(content, true);
}

void json::add_string(const char *pString, int &begin, int &end)
{
    QByteArray str = pString;
    
    for(int i=0; i<str.length(); i++)
    {
        switch(str[i])
        {
            case '\"': { QByteArray left = str.left(i); QByteArray right = str.right(str.length() - i - 1); str = left + "\\\"" + right; i++; } break;
            case '\\': { QByteArray left = str.left(i); QByteArray right = str.right(str.length() - i - 1); str = left + "\\\\" + right; i++; } break;
            case '\b' : { QByteArray left = str.left(i); QByteArray right = str.right(str.length() - i - 1); str = left + "\\b" + right; i++; } break;
            case '\f' : { QByteArray left = str.left(i); QByteArray right = str.right(str.length() - i - 1); str = left + "\\f" + right; i++; } break;
            case '\n' : { QByteArray left = str.left(i); QByteArray right = str.right(str.length() - i - 1); str = left + "\\n" + right; i++; } break;
            case '\r' : { QByteArray left = str.left(i); QByteArray right = str.right(str.length() - i - 1); str = left + "\\r" + right; i++; } break;
            case '\t' : { QByteArray left = str.left(i); QByteArray right = str.right(str.length() - i - 1); str = left + "\\t" + right; i++; } break;
            default: break;
        }
    }

    begin = storage.length();
    end = begin + str.length() + 2;

    storage.append("\"" + str + "\"");
    content = storage.c_str();
}