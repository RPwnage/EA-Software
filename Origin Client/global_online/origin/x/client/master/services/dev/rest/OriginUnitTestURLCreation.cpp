///////////////////////////////////////////////////////////////////////////////
// GcsUnitTestURLCreation.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/rest/FriendServiceClient.h"
#include "services/rest/PrivacyServiceClient.h"
#include "services/rest/AvatarServiceClient.h"
#include "services/rest/SSOTicketServiceClient.h"
#include "services/rest/UserAgent.h"
#include "services/rest/AtomServiceClient.h"

#include <QStringBuilder> 
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

// globals
const static bool DO_FRIEND(false);
const static bool DO_PRIVACY(false);
const static bool DO_AVATAR(false);
const static bool DO_SSOTICKET(false);

struct testUser
{
	quint64 mNucleusId;
	QString mEmail;
	QString mEAID;
	testUser() {}
	testUser(quint64 nucleusId, const QString& email, const QString& eaid) : mNucleusId(nucleusId), mEmail(email) {}
};

const static quint64 MAINUSER_FC_QA = 12294874251ULL; // FC.QA: jrivero@contractor.ea.com/1q2w3e4r
const static quint64 SECUSER_FC_QA = 12296734602ULL; // FC.QA: sr.jrivero@gmail.com/1q2w3e4r

void dumpCurlCommandLine(const QUrl &url, const QString& postData = QString());
///
/// generates GCS request URLs
///

using namespace Origin::Services;

OriginServiceResponse* resp = (OriginServiceResponse *)NULL;

void createGcsUnitTestData()
{
	QMap<quint64, testUser> usersList;
	usersList[MAINUSER_FC_QA] = testUser(MAINUSER_FC_QA, "jrivero@contactor.ea.com", "jj_rivero");
	usersList[SECUSER_FC_QA] = (testUser(SECUSER_FC_QA, "sr.jrivero@gmail.com", "zumba_el_gato"));

	///
	/// Execute these commands to obtain the relevant cUrl command lines
	///
	if(DO_SSOTICKET)
	{
		///
		/// SSO Ticket
		///
		ORIGIN_LOG_ERROR << "#////////////////////////////////////////////////////////";
		ORIGIN_LOG_ERROR << "#/// SSOTICKET ";

		/// retrieves SSO Ticket
		resp = SSOTicketServiceClient::ssoTicket(Session::SessionService::currentSession());
		ORIGIN_LOG_ERROR << "# **SSOTicketServiceClient";
		dumpCurlCommandLine(resp->reply()->url());
	}
	///
	// fixtures/friendservice
	///
	if(DO_FRIEND)
	{
		ORIGIN_LOG_ERROR << "#////////////////////////////////////////////////////////";
		ORIGIN_LOG_ERROR << "#/// FRIEND SERVICE";

		resp = FriendServiceClient::retrievePendingFriends(Session::SessionService::currentSession());
		ORIGIN_LOG_ERROR << "# **FriendServiceClient::retrievePendingFriends();";
		dumpCurlCommandLine(resp->reply()->url());

		resp = FriendServiceClient::retrieveInvitations(Session::SessionService::currentSession());
		ORIGIN_LOG_ERROR << "# **FriendServiceClient::retrieveInvitations();";
		dumpCurlCommandLine(resp->reply()->url());

		resp = FriendServiceClient::inviteFriend(Session::SessionService::currentSession(),usersList[SECUSER_FC_QA].mNucleusId, "TEST");
		ORIGIN_LOG_ERROR << QString("# **FriendServiceClient::inviteFriend(%1, \"TEST\")").arg(usersList[SECUSER_FC_QA].mNucleusId);
		dumpCurlCommandLine(resp->reply()->url());

		resp = FriendServiceClient::inviteFriendByEmail(Session::SessionService::currentSession(),usersList[SECUSER_FC_QA].mEmail);
		ORIGIN_LOG_ERROR << QString("# **FriendServiceClient::inviteFriendByEmail(\"%1\");").arg(usersList[SECUSER_FC_QA].mEmail);
		dumpCurlCommandLine(resp->reply()->url());

		resp = FriendServiceClient::confirmInvitation(Session::SessionService::currentSession(),usersList[SECUSER_FC_QA].mNucleusId);
		ORIGIN_LOG_ERROR << QString("# **FriendServiceClient::confirmInvitation(%1);").arg(usersList[SECUSER_FC_QA].mNucleusId);
		dumpCurlCommandLine(resp->reply()->url());

		resp = FriendServiceClient::rejectInvitation(Session::SessionService::currentSession(),usersList[SECUSER_FC_QA].mNucleusId);
		ORIGIN_LOG_ERROR << QString("# **FriendServiceClient::rejectInvitation(%1);").arg(usersList[SECUSER_FC_QA].mNucleusId);
		dumpCurlCommandLine(resp->reply()->url());

		resp = FriendServiceClient::blockUser(Session::SessionService::currentSession(),usersList[SECUSER_FC_QA].mNucleusId);
		ORIGIN_LOG_ERROR << QString("# **FriendServiceClient::blockUser(%1);").arg(usersList[SECUSER_FC_QA].mEmail);
		dumpCurlCommandLine(resp->reply()->url());

		resp = FriendServiceClient::unblockUser(Session::SessionService::currentSession(),usersList[SECUSER_FC_QA].mNucleusId);
		ORIGIN_LOG_ERROR << QString("# **FriendServiceClient::unblockUser(%1);").arg(usersList[SECUSER_FC_QA].mEmail);
		dumpCurlCommandLine(resp->reply()->url());

		resp = FriendServiceClient::blockedUsers(Session::SessionService::currentSession());
		ORIGIN_LOG_ERROR << "# **FriendServiceClient::blockedUsers();";
		dumpCurlCommandLine(resp->reply()->url());

		resp = FriendServiceClient::deleteFriend(Session::SessionService::currentSession(),usersList[SECUSER_FC_QA].mNucleusId);
		ORIGIN_LOG_ERROR << QString("# **FriendServiceClient::deleteFriend(%1);").arg(usersList[SECUSER_FC_QA].mEmail);
		dumpCurlCommandLine(resp->reply()->url());
	}

	///
	/// Privacy setting
	/// fixtures/privacyservice
	/// Execute these commands to obtain the relevant cUrl command lines
	///
	if(DO_PRIVACY)
	{
		ORIGIN_LOG_ERROR << "#////////////////////////////////////////////////////////";
		ORIGIN_LOG_ERROR << "#/// PRIVACY ";

		/// retrieves privacy setting
		// friends profile visibility
		QList<quint64> list;

		list.append(usersList[MAINUSER_FC_QA].mNucleusId); 
		list.append(usersList[SECUSER_FC_QA].mNucleusId); 

		resp = PrivacyServiceClient::friendsProfileVisibility(Session::SessionService::currentSession(),list);
		ORIGIN_LOG_ERROR << "# **PrivacyServiceClient::friendsProfileVisibility();";
		dumpCurlCommandLine(resp->reply()->url());

		// check that the specified friends profile is visible to the current user
		resp = PrivacyServiceClient::isFriendProfileVisibility(Session::SessionService::currentSession(),usersList[SECUSER_FC_QA].mNucleusId);
		ORIGIN_LOG_ERROR << QString("# **PrivacyServiceClient::isFriendProfileVisibility(%1);").arg(usersList[SECUSER_FC_QA].mNucleusId);
		dumpCurlCommandLine(resp->reply()->url());

		// Check friends rich presence visibility
		// usage:
		// friendRichPresencePrivacy(friendsId);
		// to obtain jj_rivero's profile visibility sign in as zumba_el_gato
		// to obtain zumba_el_gato's profile visibility sign in as jj_rivero
		resp = PrivacyServiceClient::friendRichPresencePrivacy(Session::SessionService::currentSession(),usersList[SECUSER_FC_QA].mNucleusId);
		ORIGIN_LOG_ERROR << QString("# **PrivacyServiceClient::friendRichPresencePrivacy(%1);").arg(usersList[SECUSER_FC_QA].mNucleusId);
		dumpCurlCommandLine(resp->reply()->url());

		// Retrieve rich presence privacy setting
		resp = PrivacyServiceClient::richPresencePrivacy(Session::SessionService::currentSession());
		ORIGIN_LOG_ERROR << "# **PrivacyServiceClient::richPresencePrivacy();";
		dumpCurlCommandLine(resp->reply()->url());

		/// Gets the current user's profile privacy setting.
		resp = PrivacyServiceClient::profilePrivacy(Session::SessionService::currentSession());
		ORIGIN_LOG_ERROR << "# **PrivacyServiceClient::profilePrivacy();";
		dumpCurlCommandLine(resp->reply()->url());
	}
	/// AVATAR
	if(DO_AVATAR)
	{
		ORIGIN_LOG_ERROR << "#////////////////////////////////////////////////////////";
		ORIGIN_LOG_ERROR << "#/// AVATAR ";

		resp = AvatarServiceClient::allAvatarTypes();
		ORIGIN_LOG_ERROR << "# **AvatarServiceClient::allAvatarTypes();";
		dumpCurlCommandLine(resp->reply()->url());

		resp = AvatarServiceClient::existingAvatarType(AvatarTypeNormal);
		ORIGIN_LOG_ERROR << "# **AvatarServiceClient::existingAvatarType();";
		dumpCurlCommandLine(resp->reply()->url());

		resp = AvatarServiceClient::userAvatarChanged(Session::SessionService::currentSession(),AvatarTypeNormal);
		ORIGIN_LOG_ERROR << "# **AvatarServiceClient::userAvatarChanged(AvatarTypeNormal);";
		dumpCurlCommandLine(resp->reply()->url());

		resp = AvatarServiceClient::avatarById(Session::SessionService::currentSession(),AvatarTypeNormal);
		ORIGIN_LOG_ERROR << "# **AvatarServiceClient::avatarById(AvatarTypeNormal);";
		dumpCurlCommandLine(resp->reply()->url());

		resp = AvatarServiceClient::avatarsByGalleryId(Session::SessionService::currentSession(),1); // TODO testing gallery  ID
		ORIGIN_LOG_ERROR << "# **AvatarServiceClient::avatarsByGalleryId(1);";
		dumpCurlCommandLine(resp->reply()->url());

		QList<quint64> list;
		list.append(usersList[MAINUSER_FC_QA].mNucleusId); 
		list.append(usersList[SECUSER_FC_QA].mNucleusId); 
		resp = AvatarServiceClient::avatarsByUserIds(Session::SessionService::currentSession(),list);
		ORIGIN_LOG_ERROR << "# **AvatarServiceClient::avatarsByUserIds(list);";
		dumpCurlCommandLine(resp->reply()->url());

		resp = AvatarServiceClient::defaultAvatar(Session::SessionService::currentSession());
		ORIGIN_LOG_ERROR << "# **AvatarServiceClient::defaultAvatar();";
		dumpCurlCommandLine(resp->reply()->url());

		resp = AvatarServiceClient::recentAvatarList(Session::SessionService::currentSession());
		ORIGIN_LOG_ERROR << "# **AvatarServiceClient::recentAvatarList();";
		dumpCurlCommandLine(resp->reply()->url());

		resp = AvatarServiceClient::supportedDimensions(Session::SessionService::currentSession());
		ORIGIN_LOG_ERROR << "# **AvatarServiceClient::supportedDimensions();";
		dumpCurlCommandLine(resp->reply()->url());

	}
}

///
/// Dumps the cUrl command line.
/// If there is POST data to set, it will come in the postData param.
/// The generated output can be directly put into a bash sh file to be executed.
///
void dumpCurlCommandLine(const QUrl &url, const QString& postData)
{
	
	QString token = Session::SessionService::accessToken(Session::SessionService::currentSession());

	QString fileName = url.path();
	QUrlQuery urlQuery(url);
    fileName.remove(0, 1);

	// Add the query parts in sorted order so we don't depend on the order
	// they were added
    QList<QPair<QString, QString> > queryItems = urlQuery.queryItems();
	qSort(queryItems.begin(), queryItems.end());

	for(QList<QPair<QString, QString> >::ConstIterator it = queryItems.constBegin();
		it != queryItems.end();
		it++)
	{
		// Don't use ? as it's reserved - delimit all pairs with ;
		fileName += ";" + (*it).first + "=" + (*it).second; 
	}

	QString commandLine = "curl -A \"";
	commandLine += userAgentHeader();
	commandLine += "\" -H \"Content-Type: text/plain\" ";
	if (postData.size() > 0)
		commandLine += QString(" -d ")  + postData;

	commandLine += " -H \"AuthToken:" % token % "\" --insecure -i \"" % url.toString() % "\" > \"" % fileName % "\"\n";

	ORIGIN_LOG_ERROR << "#---------";
	ORIGIN_LOG_ERROR << commandLine.toUtf8();
	ORIGIN_LOG_ERROR << "#---------";
}
