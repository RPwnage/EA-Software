///////////////////////////////////////////////////////////////////////////////
// BaseGameServiceResponse.cpp
//
// Author: Hans van Veenendaal
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/achievements/AchievementServiceResponse.h"
#include "services/debug/DebugService.h"

#include <QDomDocument>
#include <NodeDocument.h>
#include <ReaderCommon.h>
#include "serverReader.h"

namespace Origin
{
    namespace Services
    {
        namespace Achievements
        {

            void ReadClass(INodeDocument * pDoc, AchievementT &achievement)
            {
                // Copied from  bool ReadClass(INodeDocument * doc, AchievementT &data)
                ReadAttribute(pDoc, "u", achievement.u);
                ReadAttribute(pDoc, "e", achievement.e);
                ReadAttribute(pDoc, "p", achievement.p);
                ReadAttribute(pDoc, "t", achievement.t);
                ReadAttribute(pDoc, "cnt", achievement.cnt);
                ReadAttribute(pDoc, "img", achievement.img);
                ReadAttribute(pDoc, "name", achievement.name);
                ReadAttribute(pDoc, "desc", achievement.desc);
                ReadAttribute(pDoc, "howto", achievement.howto);
                ReadAttribute(pDoc, "xp", achievement.xp);
                ReadAttribute(pDoc, "xp_t", achievement.xp_t);

                // Not all achievement service instances support this attribute yet.
                if(achievement.xp_t == "")achievement.xp_t = QString::number(achievement.xp);

                ReadAttribute(pDoc, "rp", achievement.rp);
                ReadAttribute(pDoc, "rp_t", achievement.rp_t);

                // Not all achievement service instances support this attribute yet.
                if(achievement.rp_t == "")achievement.rp_t = QString::number(achievement.rp);

                // read image data
                if(pDoc->FirstChild("icons"))
                {
                    if(pDoc->FirstAttribute())
                    {
                        do 
                        {
                            int size = QString(pDoc->GetAttributeName()).toInt();
                            achievement.icons[size] = QString(pDoc->GetAttributeValue());

                        } while (pDoc->NextAttribute());
                    }
                }
                pDoc->Parent();

                achievement.hasExp = false;

                if(pDoc->FirstChild("xpack"))
                {
                    achievement.hasExp = true;
                    ReadAttribute(pDoc, "id", achievement.expId);
                    ReadAttribute(pDoc, "name", achievement.expName);
                }
                pDoc->Parent();
            }

            void ReadClass(INodeDocument * pDoc, ExpansionT &expansion)
            {
                // Copied from  bool ReadClass(INodeDocument * doc, AchievementT &data)
                ReadAttribute(pDoc, "id", expansion.expansionId);
                ReadAttribute(pDoc, "name", expansion.expansionName);
            }

            void Read( INodeDocument * pDoc, AchievementSetT &set )
            {
                set.name = QString::fromUtf8(pDoc->GetAttributeValue("name"));
                set.platform = QString(pDoc->GetAttributeValue("platform"));

                if(pDoc->FirstChild("achievements"))
                {
                    // Iterate over the map of achievements.
                    if(pDoc->FirstChild())
                    {
                        do 
                        {
                            AchievementT achievement;

                            QString Id = QString(pDoc->GetName());

                            ReadClass(pDoc, achievement);

                            set.achievements[Id] = achievement;
                        }
                        while (pDoc->NextChild());
                    }
                    pDoc->Parent();
                }
                pDoc->Parent();

                if(pDoc->EnterArray("expansions"))
                {
                    // Iterate over the map of achievements.
                    if(pDoc->FirstValue())
                    {
                        do 
                        {
                            ExpansionT expansion;

                            ReadClass(pDoc, expansion);

                            set.expansions.append(expansion);
                        }
                        while (pDoc->NextValue());
                    }
                    pDoc->LeaveArray();
                }
                pDoc->Parent();
            }
			

            AchievementSetQueryResponse::AchievementSetQueryResponse(const QString &personaId, const QString &achievementSet, AuthNetworkRequest reply ) :
                OriginAuthServiceResponse(reply)
            {
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));

                mSet.setId = achievementSet;
            }

            bool AchievementSetQueryResponse::parseSuccessBody( QIODevice *body )
            {
                QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

                QByteArray data = body->readAll();
                if(pDoc->Parse(data.data()))
                {
                    Read(pDoc.data(), mSet);
                    return true;
                }

                return false;
            }

            AchievementSetsQueryResponse::AchievementSetsQueryResponse(const QString &personaId, AuthNetworkRequest reply ) :
                OriginAuthServiceResponse(reply)
            {
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
            }

            bool AchievementSetsQueryResponse::parseSuccessBody( QIODevice *body )
            {
                QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

                QByteArray data = body->readAll();

                if(pDoc->Parse(data.data()))
                {
                    // Iterate over the map of achievement sets.
                    if(pDoc->FirstChild())
                    {
                        do    
                        {
                            AchievementSetT set;

                            set.setId = QString(pDoc->GetName());

                            Read(pDoc.data(), set);

                            mAchievementSets.append(set);
                        } 
                        while (pDoc->NextChild());
                    }
                    return true;
                }

                return false;
            }

			AchievementSetReleaseInfoResponse::AchievementSetReleaseInfoResponse( AuthNetworkRequest reply ) :
				OriginAuthServiceResponse(reply)
			{
				ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
			}


			void Read( INodeDocument * pDoc, AchievementSetReleaseInfoT &releaseInfo )
			{
				releaseInfo.masterTitleId = QString(pDoc->GetAttributeValue("masterTitleId"));
				releaseInfo.displayName = QString::fromUtf8(pDoc->GetAttributeValue("name"));
				releaseInfo.platform = QString(pDoc->GetAttributeValue("platform"));
			}

			bool AchievementSetReleaseInfoResponse::parseSuccessBody(QIODevice * body)
			{
				QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

				QByteArray data = body->readAll();

				if(pDoc->Parse(data.data()))
				{
					// Iterate over the map of achievement sets.
					if(pDoc->FirstChild())
					{
						do    
						{
							AchievementSetReleaseInfoT releaseInfo;

							releaseInfo.achievementSetId = QString(pDoc->GetName());

							Read(pDoc.data(), releaseInfo);

							mAchievementSetReleaseInfo.append(releaseInfo);
						} 
						while (pDoc->NextChild());
					}
					return true;
				}

				return false;
			}

            AchievementProgressQueryResponse::AchievementProgressQueryResponse(AuthNetworkRequest reply ) :
                OriginAuthServiceResponse(reply)
            {
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
            }

            bool AchievementProgressQueryResponse::parseSuccessBody( QIODevice *body )
            {
                QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

                QByteArray data = body->readAll();
                if(pDoc->Parse(data.data()))
                {
                    if(server::Read(pDoc.data(), mProgress))
                    {
                        return true;
                    }
                }
                return false;
            }

            AchievementGrantResponse::AchievementGrantResponse(AuthNetworkRequest reply ) :
                OriginAuthServiceResponse(reply)
            {
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
            }

            bool AchievementGrantResponse::parseSuccessBody( QIODevice *body )
            {
                QByteArray data = body->readAll();

                // TODO: Do something useful with the body.

                return true;
            }

            PostEventResponse::PostEventResponse(AuthNetworkRequest reply) :
                OriginAuthServiceResponse(reply)
            {
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
            }

            bool PostEventResponse::parseSuccessBody(QIODevice *body)
            {
                QByteArray data = body->readAll();

                // TODO: Do something useful with the body.

                return true;
            }
        }
    }
}
