///////////////////////////////////////////////////////////////////////////////
// FriendServiceResponses.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include "services/rest/FriendServiceResponses.h"

namespace Origin
{
    namespace Services
    {
        RetrieveInvitationsResponse::RetrieveInvitationsResponse(AuthNetworkRequest reply) : OriginAuthServiceResponse(reply)
        {
        }

        bool RetrieveInvitationsResponse::parseSuccessBody(QIODevice *body)
        {
            QDomDocument doc;

            if (!doc.setContent(body))
            {
                // Not valid XML
                return false;
            }

            if (doc.documentElement().nodeName() != "invitations")
            {
                // Not the XML document we're looking for 
                return false;
            }

            QDomNodeList rootChildren = doc.documentElement().childNodes();

            for(int i = 0; i < rootChildren.length(); i++)
            {
                QDomElement inviteEl = rootChildren.at(i).toElement();

                if (inviteEl.isNull() ||
                    (inviteEl.tagName() != "invitation"))
                {
                    // Not an invite element
                    continue;
                }

                // Try to get our user ID
                QDomElement userIdEl = inviteEl.firstChildElement("inviter").firstChildElement("userId");

                if (userIdEl.isNull())
                {
                    // Fishy
                    continue;
                }

                IncomingInvitation invite;
                invite.nucleusId = userIdEl.text().toULongLong();

                // See if they have a comment
                QDomElement commentEl = inviteEl.firstChildElement("comment");

                if (!commentEl.isNull())
                {
                    invite.comment = commentEl.text();
                }

                m_invitations.append(invite);
            }

            return true;
        }


        PendingFriendsResponse::PendingFriendsResponse(AuthNetworkRequest reply) : OriginAuthServiceResponse(reply)
        {
        }

        bool PendingFriendsResponse::parseSuccessBody(QIODevice *body)
        {
            QDomDocument doc;

            if (!doc.setContent(body))
            {
                // Not valid XML
                return false;
            }

            if (doc.documentElement().nodeName() != "users")
            {
                // Not the XML document we're looking for 
                return false;
            }

            QDomNodeList rootChildren = doc.documentElement().childNodes();

            for(int i = 0; i < rootChildren.length(); i++)
            {
                QDomElement userEl = rootChildren.at(i).toElement();

                if (userEl.isNull() ||
                    userEl.tagName() != "user")
                {
                    // Not a user element
                    continue;
                }

                // Try to get our user ID
                QDomElement userIdEl = userEl.firstChildElement("userId");

                if (userIdEl.isNull())
                {
                    // Fishy
                    continue;
                }

                m_pendingFriends.append(userIdEl.text().toULongLong());
            }

            return true;
        }

        BlockedUsersResponse::BlockedUsersResponse(AuthNetworkRequest reply) : OriginAuthServiceResponse(reply)
        {
        }

        bool BlockedUsersResponse::parseSuccessBody(QIODevice *body)
        {
            QDomDocument doc;

            // This appears to be the case when no users are currently blocked.  Currently, an empty response is returned by getBlockUserList when
            // no users are blocked.
            if(body == NULL || body->bytesAvailable() == 0)
            {
                return true;
            }

            if (!doc.setContent(body))
            {
                // Not valid XML
                return false;
            }

            if (doc.documentElement().nodeName() != "users")
            {
                // Not the XML document we're looking for 
                return false;
            }

            QDomNodeList rootChildren = doc.documentElement().childNodes();

            for(int i = 0; i < rootChildren.length(); i++)
            {
                QDomElement userEl = rootChildren.at(i).toElement();

                if (userEl.isNull() ||
                    (userEl.tagName() != "user"))
                {
                    // Not an user element
                    continue;
                }

                // Try to get our user ID
                BlockedUser blockedUser;
                blockedUser.nucleusId =  userEl.firstChildElement("userId").text().toULongLong();
                blockedUser.email = userEl.firstChildElement("email").text();
                blockedUser.personaId = userEl.firstChildElement("personaId").text().toULongLong();
                blockedUser.name = userEl.firstChildElement("name").text();

                m_blockedUsers.append(blockedUser);
            }

            return true;
        }
    }
}
