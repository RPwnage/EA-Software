#pragma once

#include <string>
#include <deque>
#include <list>
#include <map>
#include <vector>
#include <sstream>

namespace sonar
{
	typedef std::string String;
	typedef const std::string CString;

	// Only needed if Sonar IPC modules are in use
	typedef std::stringstream StringStream;

	#define SonarDeque std::deque
	#define SonarMap std::map
	#define SonarVector std::vector
	#define SonarList std::list

}
