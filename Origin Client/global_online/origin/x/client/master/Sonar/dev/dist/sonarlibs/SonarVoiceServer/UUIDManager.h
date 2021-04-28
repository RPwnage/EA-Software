#pragma once 

#include <string>

namespace sonar
{

class UUIDManager
{

public:
	UUIDManager(int index);
	const std::string &get();

private:
	bool read (void);
	bool write (void);
	bool generate(void);
	const std::string getPath(void);

private:
	std::string m_currUUID;
	int m_index;

};

}
