#ifndef _EBISUCOMMONTESTS_H
#define _EBISUCOMMONTESTS_H

#include <QCoreApplication> 
#include "services/session/SessionService.h"

class OriginServiceTests : public QCoreApplication
{
public:
	OriginServiceTests(int &argc, char **argv);
	static Origin::Services::SessionRef gSession;
	static QString gUserName;
	static QString gPassword;
};

void waitForSessionInit();

#endif