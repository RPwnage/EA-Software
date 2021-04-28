#ifndef __LIBJSON_H__
#define __LIBJSON_H__

#include <string>
#include <vector>

namespace libjson
{
	/*

	object								string											int
		{}									""												digit
		{ members }							"chars"											digit1-9 digits 
																							- digit
	members								chars												- digit1-9 digits
		pair								char
		pair , members						char chars									frac
																							. digits
	pair								char
		string : value						any-Unicode-character-						exp
												except-"-or-\-or-							e digits
	array										control-character
		[]									\"											digits
		[ elements ]						\\												digit
											\/												digit digits
	elements								\b											
		value 								\f											e
		value , elements					\n												e
											\r												e+
	value									\t												e-
		string								\u four-hex-digits								E
		number																				E+
		object							number												E-
		array								int											
		true								int frac									
		false								int exp										
		null								int frac exp

	*/

#define NOT_FOUND -1
#define UNEXPECTED_END_OF_FILE -2
#define UNEXPECTED_TOKEN -3
#define UNEXPECTED_CHARACTER -4

    class json;
	class json_object;
	class json_pair;
	class json_value;

	class json_string
	{
    public:
		json_string() : is_string(false), start(0), end(0) {}
		int parse_string(const char *pcontent, int pos);
		int parse_token(const char *pcontent, int pos);
		int parse(const char *pcontent, int pos, bool hasToBeString = true);
		std::string toString(const char *pcontent, bool unmodified = false) const;

        void set_value(json *doc, const char *pvalue);

		bool isString();

#ifdef _DEBUG
		std::string debug_value;
#endif
		bool is_string;
		int start;
		int end;

    private:
        std::string cvrtToUtf8(const std::string &src) const;
	};


	class json_object
	{
    public:
        ~json_object();

		int parse(const char *pcontent, int pos);
		std::string toString(const char *pcontent, bool unmodified = false) const;

		std::vector<json_pair *> pairs;
	};

	class json_value
	{
    public:
		json_value() : isArray(false), isObject(false), value() {}
        ~json_value();

		int parse_array(const char *pcontent, int pos);
		int parse(const char *pcontent, int pos);
		std::string toString(const char *pcontent, bool unmodified = false) const;

        void set_value(json *doc, const char *pvalue);

		bool isArray;
		bool isObject;

		json_string value;						// contains : string, number, true, false, null
		std::vector<json_value *> array;			// need isArray flag set.
		json_object object;	
	};

	class json_pair
	{
    public:
		int parse(const char *pcontent, int pos);
		std::string toString(const char *pcontent, bool unmodified = false) const;

		json_string name;
		json_value value;
	};


	class json
	{
    public:
        json();
        ~json();

		int parse(const char *pcontent);
		std::string toString() const;

        void add_string(const char *pString, int &begin, int &end);

		json_object * object;
		const char * content;

        std::string storage;
	};


}

#endif //__LIBJSON_H__