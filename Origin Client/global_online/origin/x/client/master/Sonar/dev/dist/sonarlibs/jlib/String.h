#pragma once

#include <string>
#include <list>

namespace jlib
{

  class String
  {
  public:
		static std::string wideToUtf8(const std::wstring &wide);
		static std::wstring utf8ToWide(const std::string &utf8);

		typedef std::list<const std::wstring> List;
		static List tokenize(const std::wstring &input, int delim);

  };

}