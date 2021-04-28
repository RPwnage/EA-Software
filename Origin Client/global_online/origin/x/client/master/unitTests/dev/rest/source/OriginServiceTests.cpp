#include <QTest>
#include "../OriginServiceTests.h"
#include "../MockHttpReplyTest.h"
#include "../FriendServiceClientTest.h"
#include "../PrivacyServiceClientTest.h"
#include "../AvatarServiceClientTest.h"
#include "../LoginRegistrationServiceClientTest.h"
#include "../AuthenticationServiceClientTest.h"
#include "../EntitlementsServiceClientTest.h"

SessionRef OriginServiceTests::gSession;
QString OriginServiceTests::gUserName("sr.jrivero@gmail.com");
QString OriginServiceTests::gPassword ("1q2w3e4r");


OriginServiceTests::OriginServiceTests(int &argc, char **argv) : QCoreApplication(argc, argv)
{
	MockHttpReplyTest mockHttpReplyTest;
	FriendServiceClientTest friendServiceClientTest;
	PrivacyServiceClientTest privacyServiceClientTest;
	AvatarServiceClientTest avatarServiceClientTest;
	LoginRegistrationServiceClientTest loginRegistrationServiceClientTest;
	AuthenticationClientTest authenicationServiceClientTest;
	EntitlementsClientTest entitlementsClientTest;

	QTest::qExec(&mockHttpReplyTest, argc, argv);
	QTest::qExec(&authenicationServiceClientTest, argc, argv);
	QTest::qExec(&friendServiceClientTest, argc, argv);
	QTest::qExec(&privacyServiceClientTest, argc, argv);
	QTest::qExec(&avatarServiceClientTest, argc, argv);
	QTest::qExec(&loginRegistrationServiceClientTest, argc, argv);
	QTest::qExec(&entitlementsClientTest, argc, argv);
	
}

void waitForSessionInit()
{
	QSignalSpy signalSpy(SessionService::instance(), SIGNAL(beginSessionComplete(Origin::Services::SessionError, Origin::Services::SessionRef)));
	while(signalSpy.empty())
	{
		QCoreApplication::processEvents();
	}
}

