///////////////////////////////////////////////////////////////////////////////
/// \file       achievementmanager.cpp
/// This file contains the implementation of the AchievementManager. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#include <memory>
#include <QMetaType>
#include <QCryptographicHash>

#include "engine/achievements/achievementmanager.h"
#include "services/platform/TrustedClock.h"
#include "services/achievements/AchievementServiceClient.h"
#include "services/achievements/AchievementServiceResponse.h"
#include "services/config/OriginConfigService.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "engine/dirtybits/DirtyBitsClient.h"
#include "engine/login/LoginController.h"

#ifdef _WIN32
#include <shlobj.h>
#include <shobjidl.h>
#endif

#include "NodeDocument.h"
#include "serverReader.h"
#include "services/common/XmlUtil.h"
#include "services/crypto/SimpleEncryption.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"

using namespace Origin::Services::Achievements;
using namespace Origin::Services::Session;

namespace Origin
{
    namespace Engine
    {
        namespace Achievements
        {
            AchievementManager * AchievementManager::gInstance = NULL;

            const QList<AchievementPortfolioRef> & AchievementManager::achievementPortfolios() const
            {
                return mAchievementPortfolios;
            }

			const AchievementSetReleaseInfoList & AchievementManager::achievementSetReleaseInfo() const
			{
				return mAchievementSetReleaseInfo;
			}

			AchievementPortfolioRef AchievementManager::achievementPortfolioByUserId(const QString &userId) const
            {
                return mAchievementPortfolioMap[userId.toULongLong()];
            }

            AchievementPortfolioRef AchievementManager::achievementPortfolioByPersonaId(const QString &personaId) const
            {
                foreach(AchievementPortfolioRef portfolio, mAchievementPortfolios)
                {
                    if(personaId == portfolio->personaId())
                    {
                        return portfolio;
                    }
                }
                return AchievementPortfolioRef();
            }


            bool AchievementManager::enabled() const
            {
                return mEnabled;
            }

            QString AchievementManager::baseUrl() const
            {
                return mBaseUrl;
            }

            AchievementPortfolioRef AchievementManager::currentUsersPortfolio() const
            {
                SessionRef session = SessionService::instance()->currentSession();

                if(!session.isNull())
                {
                    QString userId = SessionService::instance()->nucleusUser(session);

                    AchievementPortfolioRef portfolio = mAchievementPortfolioMap[userId.toULongLong()];

                    Q_ASSERT(!portfolio.isNull() && "The achievement portfolio is not created yet.");

                    return portfolio;
                }
                return AchievementPortfolioRef();
            }

            void AchievementManager::onUserLoggedIn(Origin::Engine::UserRef user)
            {
                mConfigurationLoaded = true;

                // Load the off line achievements.
                serialize(false);
            }

            void AchievementManager::onUserOnlineStatusChanged()
            {
                UserRef user = LoginController::instance()->currentUser();
                if(user != NULL && Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()))
                {
                    mOnlineTimer.singleShot(20000, this, SLOT(onQueueAction()));
                }
            }

            void AchievementManager::onQueueAction()
            {
                grantQueuedAchievements();
                postQueuedEvents();
            }

            void AchievementManager::initializeEventHandler()
            {
                Origin::Engine::DirtyBitsClient::instance()->registerHandler(this, "granted", "ach");
            }


            void AchievementManager::clear()
            {
                mAchievementPortfolioMap.clear();

                foreach(AchievementPortfolioRef portfolio, mAchievementPortfolios)
                {
                    emit removed(portfolio);
                    portfolio->deleteLater();
                }

                mAchievementPortfolios.clear();

                if(mLastAchievementsSubmitted)
                {
                    mLastAchievementsSubmitted->deleteLater();
                }
                if(mLastEventsSubmitted)
                {
                    mLastEventsSubmitted->deleteLater();
                }
            }

            void AchievementManager::granted(QByteArray grantedAchievement)
            {
                QScopedPointer<INodeDocument> json(CreateJsonDocument());
                json->Parse(grantedAchievement);

                server::AchievementNotificationT notification;

                if(server::Read(json.data(), notification))
                {
                    AchievementPortfolioRef portfolio = mAchievementPortfolioMap[notification.uid];

                    if(!portfolio.isNull())
                    {
                        portfolio->granted(notification);
                    }
                }
            }

            AchievementPortfolioRef AchievementManager::createAchievementPortfolio(QString userId, QString personaId)
            {
                AchievementPortfolioRef portfolio = mAchievementPortfolioMap[userId.toULongLong()];

                if(portfolio.isNull())
                {
                    portfolio = AchievementPortfolioRef(new AchievementPortfolio(userId, personaId, this));

                    mAchievementPortfolioMap[userId.toULongLong()] = portfolio;
                    mAchievementPortfolios.append(portfolio);

                    ORIGIN_VERIFY_CONNECT(portfolio.data(), SIGNAL(granted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)),
                        this, SIGNAL(granted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)));

                    emit added(portfolio);
                }

                return portfolio;
            }


            void AchievementManager::grantQueuedAchievements()
            {
                QMutexLocker lock(&mAchievementGrantQueueMutex);
                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                if (session.isNull())
                    return;

                if(mLastAchievementsSubmitted.isNull())
                {
                    mLastAchievementsSubmitted = new Origin::Services::Setting("AchCnt-" + Origin::Services::Session::SessionService::nucleusUser(session), Origin::Services::Variant::ULongLong, 0, Origin::Services::Setting::LocalPerUser);
                }
                                
                qulonglong timestamp = readSetting(*mLastAchievementsSubmitted.data(), session);

                foreach(const AchievementGrantRecord &item, mAchievementGrantQueue)
                {
                    if(item.timestamp > timestamp)
                    {
                        /// try granting the achievement
                        AchievementGrantResponse * resp = Origin::Services::Achievements::AchievementServiceClient::grantAchievement(item.personaId, item.appId, item.setId, item.id, item.code, item.progress);

                        if(resp != NULL)
                        {
                            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(achievementGrantCompleted()));
                        }
                        break;
                    }
                    else
                    {
                        /// Already granted
                        mAchievementGrantQueue.pop_front();
                    }
                }

                if(mAchievementGrantQueue.size() == 0)
                {
                    // Reset the time to the current time.
                    writeSetting(*mLastAchievementsSubmitted.data(), (quint64)Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch(), session);
                }
            }

            void AchievementManager::achievementGrantCompleted()
            {
                Origin::Services::Achievements::AchievementGrantResponse* response = dynamic_cast<Origin::Services::Achievements::AchievementGrantResponse *>(sender());

                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http200Ok || response->httpStatus() == Origin::Services::Http404ClientErrorNotFound)
                    {
                        {
                            QMutexLocker lock(&mAchievementGrantQueueMutex);

                            if(mAchievementGrantQueue.size() > 0)
                            {
                                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                                if(mLastAchievementsSubmitted.isNull())
                                {
                                    mLastAchievementsSubmitted = new Origin::Services::Setting("AchCnt-" + Origin::Services::Session::SessionService::nucleusUser(session), Origin::Services::Variant::ULongLong, 0, Origin::Services::Setting::LocalPerUser);
                                }

                                const AchievementGrantRecord &record = mAchievementGrantQueue.front();

                                // Mark this item as done.
                                writeSetting(*mLastAchievementsSubmitted.data(), record.timestamp, session);

                                // Remove the item.
                                mAchievementGrantQueue.pop_front();
                            }
                        }

                        // Store the updated list.
                        AchievementManager::instance()->serialize(true);

                        // Grant the next achievement in the queue
                        grantQueuedAchievements();
                    }
                }
            }

            void AchievementManager::postQueuedEvents()
            {
                QMutexLocker lock(&mAchievementGrantQueueMutex);
                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                if(session.isNull())
                    return;

                if(mLastEventsSubmitted.isNull())
                {
                    mLastEventsSubmitted = new Origin::Services::Setting("EventCnt-" + Origin::Services::Session::SessionService::nucleusUser(session), Origin::Services::Variant::ULongLong, 0, Origin::Services::Setting::LocalPerUser);
                }

                qulonglong timestamp = readSetting(*mLastEventsSubmitted.data(), session);

                foreach(const PostEventRecord &item, mPostEventQueue)
                {
                    if(item.timestamp > timestamp)
                    {
                        /// try granting the achievement
                        PostEventResponse * resp = Origin::Services::Achievements::AchievementServiceClient::postEvent(item.personaId, item.appId, item.setId, item.events);

                        if(resp != NULL)
                        {
                            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(eventPostCompleted()));
                        }
                        break;
                    }
                    else
                    {
                        /// Already granted
                        mPostEventQueue.pop_front();
                    }
                }

                if(mPostEventQueue.size() == 0)
                {
                    // Reset the time to the current time.
                    writeSetting(*mLastEventsSubmitted.data(), (quint64)Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch(), session);
                }
            }

            void AchievementManager::eventPostCompleted()
            {
                Origin::Services::Achievements::PostEventResponse* response = dynamic_cast<Origin::Services::Achievements::PostEventResponse*>(sender());

                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http200Ok || response->httpStatus() == Origin::Services::Http404ClientErrorNotFound)
                    {
                        {
                            QMutexLocker lock(&mAchievementGrantQueueMutex);

                            if(mPostEventQueue.size() > 0)
                            {
                                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                                if(mLastEventsSubmitted.isNull())
                                {
                                    mLastEventsSubmitted = new Origin::Services::Setting("EventCnt-" + Origin::Services::Session::SessionService::nucleusUser(session), Origin::Services::Variant::ULongLong, 0, Origin::Services::Setting::LocalPerUser);
                                }

                                const PostEventRecord &record = mPostEventQueue.front();

                                // Mark this item as done.
                                writeSetting(*mLastAchievementsSubmitted.data(), record.timestamp, session);

                                // Remove the item.
                                mPostEventQueue.pop_front();
                            }
                        }

                        // Store the updated list.
                        AchievementManager::instance()->serialize(true);

                        // Grant the next achievement in the queue
                        postQueuedEvents();
                    }
                }
            }

            void AchievementManager::queueAchievementGrant(const QString personaId, const QString &appId, const QString &achievementSetId, const QString &achivementId, const QString &achievementCode, int progress)
            {
                {
                    QMutexLocker lock(&mAchievementGrantQueueMutex);
                    AchievementGrantRecord rec;
                    rec.personaId = personaId;
                    rec.appId = appId;
                    rec.setId = achievementSetId;
                    rec.id = achivementId;
                    rec.code = achievementCode;
                    rec.progress = progress;

                    rec.timestamp = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch();

                    mAchievementGrantQueue.append(rec);
                }

                serialize();
            }

            void AchievementManager::queueAchievementEvent(QString personaId, QString masterTitleId, QString achievementSetId, QString events)
            {
                {
                    QMutexLocker lock(&mAchievementGrantQueueMutex);
                    PostEventRecord rec;
                    rec.personaId = personaId;
                    rec.appId = masterTitleId;
                    rec.setId = achievementSetId;
                    rec.events = events;

                    rec.timestamp = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch();

                    mPostEventQueue.append(rec);
                }

                serialize();
            }

            void AchievementManager::serialize(bool bSave)
            {
                QMutexLocker lock(&mAchievementSerializationMutex);

                if(!mEnabled)
                    return;

                if(bSave)
                {
                    QDomDocument doc;

                    // Put the storing code here.
                    QDomElement root = doc.createElement("am");
                    doc.appendChild(root);

                    Origin::Util::XmlUtil::setInt(root, "version", ACHIEVEMENT_DOCUMENT_VERSION);

                    // Save the portfolio of the current user.
                    AchievementPortfolioRef portfolio = currentUsersPortfolio();
                    
                    if(portfolio == NULL)
                        return;

                    QDomElement p = Origin::Util::XmlUtil::newElement(root, "p");

                    Origin::Util::XmlUtil::setString(p, "uid", portfolio->userId());
                    Origin::Util::XmlUtil::setString(p, "pid", portfolio->personaId());

                    portfolio->serialize(p, ACHIEVEMENT_DOCUMENT_VERSION, bSave);

                    serializeAchievementGrantQueue(root, ACHIEVEMENT_DOCUMENT_VERSION, bSave);
                    serializePostEventQueue(root, ACHIEVEMENT_DOCUMENT_VERSION, bSave);

#ifdef _DEBUG
                    QFile file(cacheFolder() + "/a.xml");
                    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
                    {
                        file.write(doc.toByteArray());
                        file.close();
                    }
#endif

                    QFile encryptedFile(cacheFolder() + "/a.dat");
                    if(encryptedFile.open(QIODevice::WriteOnly))
                    {
                        QByteArray data = Origin::Services::Crypto::SimpleEncryption().encrypt(doc.toByteArray().data()).c_str();
                        QByteArray salt = "4893ahj0394ndskjf";
                        QByteArray check = QCryptographicHash::hash(data + salt, QCryptographicHash::Sha1).toHex().toLower();

                        encryptedFile.write(data);
                        encryptedFile.write("O");
                        encryptedFile.write(check);
                        encryptedFile.close();
                    }
                }
                else
                {
                    QDomDocument doc;

                    QFile file(cacheFolder() + "/a.dat");
                    if(file.open(QIODevice::ReadOnly))
                    {
                        QByteArray data = file.readAll();
                        QList<QByteArray> items = data.split('O');

                        if(items.size()==2)
                        {
                            QByteArray salt = "4893ahj0394ndskjf";
                            QByteArray check = QCryptographicHash::hash(items[0] + salt, QCryptographicHash::Sha1).toHex().toLower();

                            if(check == items[1])
                            {
                                QByteArray decryptedData = Origin::Services::Crypto::SimpleEncryption().decrypt(items[0].data()).c_str();

                                doc.setContent(decryptedData);
                                file.close();

                                // Put the loading code here.
                                QDomElement root = doc.firstChildElement("am");

                                int version = Origin::Util::XmlUtil::getInt(root, "version");

                                Q_ASSERT(ACHIEVEMENT_DOCUMENT_VERSION == version && "Achievement document version doesn't match. Add code to handle version differences.");

                                if(version <= ACHIEVEMENT_DOCUMENT_VERSION)
                                {
                                    for(QDomElement p=root.firstChildElement("p") ; !p.isNull(); p=p.nextSiblingElement("p"))
                                    {
                                        QString userId = Origin::Util::XmlUtil::getString(p, "uid");
                                        QString personaId = Origin::Util::XmlUtil::getString(p, "pid");

                                        AchievementPortfolioRef portfolio = createAchievementPortfolio(userId, personaId);

                                        portfolio->serialize(p, version, bSave);
                                    }

                                    serializeAchievementGrantQueue(root, version, bSave);
                                    serializePostEventQueue(root, version, bSave);
                                }
                            }
                        }
                    }
                }
            }

            void AchievementManager::serializeAchievementGrantQueue(QDomElement root, int version, bool bSave)
            {
                QMutexLocker lock(&mAchievementGrantQueueMutex);

                if(bSave)
                {
                    foreach(const AchievementGrantRecord &gr, mAchievementGrantQueue)
                    {
                        QDomElement agqe = root.ownerDocument().createElement("agqe");

                        root.appendChild(agqe);

                        Origin::Util::XmlUtil::setString(agqe, "pid", gr.personaId);
                        Origin::Util::XmlUtil::setString(agqe, "aid", gr.appId);
                        Origin::Util::XmlUtil::setString(agqe, "as", gr.setId);
                        Origin::Util::XmlUtil::setString(agqe, "id", gr.id);
                        Origin::Util::XmlUtil::setString(agqe, "c", gr.code);
                        Origin::Util::XmlUtil::setInt(agqe, "p", gr.progress);
                        Origin::Util::XmlUtil::setLongLong(agqe, "t", gr.timestamp ^ 0xFA56EF3423CB98D3ull);
                    }
                }
                else
                {
                    for(QDomElement agqe=root.firstChildElement("agqe") ; !agqe.isNull(); agqe=agqe.nextSiblingElement("agqe"))
                    {
                        AchievementGrantRecord rec;

                        rec.personaId = Origin::Util::XmlUtil::getString(agqe, "pid");
                        rec.appId = Origin::Util::XmlUtil::getString(agqe, "aid");
                        rec.setId = Origin::Util::XmlUtil::getString(agqe, "as");
                        rec.id = Origin::Util::XmlUtil::getString(agqe, "id");
                        rec.code = Origin::Util::XmlUtil::getString(agqe, "c");
                        rec.progress = Origin::Util::XmlUtil::getInt(agqe, "p");
                        rec.timestamp = ((quint64)Origin::Util::XmlUtil::getLongLong(agqe, "t")) ^ 0xFA56EF3423CB98D3ull;

                        bool found = false;

                        foreach(const AchievementGrantRecord &gr, mAchievementGrantQueue)
                        {
                            if(gr.timestamp == rec.timestamp && gr.setId == rec.setId && gr.id == rec.id)
                            {
                                // already in the list.
                                found = true;
                                break;
                            }
                        }

                        if(!found)
                        {
                            mAchievementGrantQueue.append(rec);
                        }
                    }
                }
            }

            void AchievementManager::serializePostEventQueue(QDomElement root, int version, bool bSave)
            {
                QMutexLocker lock(&mAchievementGrantQueueMutex);

                if(bSave)
                {
                    foreach(const PostEventRecord &per, mPostEventQueue)
                    {
                        QDomElement er = root.ownerDocument().createElement("er");

                        root.appendChild(er);

                        Origin::Util::XmlUtil::setString(er, "pid", per.personaId);
                        Origin::Util::XmlUtil::setString(er, "aid", per.appId);
                        Origin::Util::XmlUtil::setString(er, "as", per.setId);
                        Origin::Util::XmlUtil::setString(er, "ev", per.events);
                        Origin::Util::XmlUtil::setLongLong(er, "t", per.timestamp ^ 0xFA56EF3423CB98D3ull);
                    }
                }
                else
                {
                    for(QDomElement er = root.firstChildElement("er"); !er.isNull(); er = er.nextSiblingElement("er"))
                    {
                        PostEventRecord rec;

                        rec.personaId = Origin::Util::XmlUtil::getString(er, "pid");
                        rec.appId = Origin::Util::XmlUtil::getString(er, "aid");
                        rec.setId = Origin::Util::XmlUtil::getString(er, "as");
                        rec.events = Origin::Util::XmlUtil::getString(er, "ev");
                        rec.timestamp = ((quint64)Origin::Util::XmlUtil::getLongLong(er, "t")) ^ 0xFA56EF3423CB98D3ull;

                        bool found = false;

                        foreach(const PostEventRecord &er, mPostEventQueue)
                        {
                            if(er.timestamp == rec.timestamp && er.setId == er.setId && er.events == rec.events)
                            {
                                // already in the list.
                                found = true;
                                break;
                            }
                        }

                        if(!found)
                        {
                            mPostEventQueue.append(rec);
                        }
                    }
                }
            }

            AchievementManager::AchievementManager(QObject * owner) :
                QObject(owner),
                mEnabled(Origin::Services::OriginConfigService::instance()->achievementsConfig().enabled || Origin::Services::readSetting(Origin::Services::SETTING_AchievementsEnabled)),
                mConfigurationLoaded(false)
            {
                gInstance = this;

                ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)), this, SLOT(onUserLoggedIn(Origin::Engine::UserRef)));
                ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)),
                                      this, SLOT(onUserOnlineStatusChanged()));
            }

            AchievementManager::~AchievementManager()
            {
                clear();

                if(gInstance == this)
                {
                    gInstance = NULL;
                }
            }

            void AchievementManager::init(QObject * owner)
            {
                if(gInstance == NULL)
                {
                    new AchievementManager(owner);                        
                }
            }

            void AchievementManager::shutdown()
            {
                if(gInstance)
                {
                    delete gInstance;
                }
            }

            AchievementManager * AchievementManager::instance()
            {
                return gInstance;
            }

            QString AchievementManager::cacheFolder() const
            {
                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                if (!session.isNull())
                {
                    QString environment = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName);
                    QString userId = Origin::Services::Session::SessionService::nucleusUser(session);
                    userId = QCryptographicHash::hash(userId.toLatin1(), QCryptographicHash::Sha1).toHex().toUpper();

                    // compute the path to the achievement cache, which consists of a platform dependent
                    // root directory, hard-coded application specific directories, environment directory 
                    // (omitted in production), and a per-user directory containing the actual cache files.
                    QStringList path;
#if defined(ORIGIN_PC)
                    WCHAR buffer[MAX_PATH];
                    SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer );
                    path << QString::fromWCharArray(buffer) << "Origin";
#elif defined(ORIGIN_MAC)
                    path << Origin::Services::PlatformService::commonAppDataPath();
#else
#error Must specialize for other platform.
#endif

                    path << "AchievementCache";
                    if ( environment != "production" )
                        path << environment;

                    path << userId; 

                    QString cachePath = path.join("/");

                    if(QDir(cachePath).exists() || QDir().mkpath(cachePath))
                        return cachePath;

                }
                return QString();
            }

			void AchievementManager::updateAchievementSetReleaseInfo()
			{
				AchievementSetReleaseInfoResponse * resp = AchievementServiceClient::achievementSetReleaseInfo(QLocale().name());

				if(resp != NULL)
				{
					ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(achievementSetReleaseInfoCompleted()));
				}				
			}

			void AchievementManager::achievementSetReleaseInfoCompleted()
			{
				Origin::Services::Achievements::AchievementSetReleaseInfoResponse* response = dynamic_cast<Origin::Services::Achievements::AchievementSetReleaseInfoResponse * >(sender());

				if(response)
				{
					if(response->httpStatus() == Origin::Services::Http200Ok)
					{
						mAchievementSetReleaseInfo.clear();

						foreach(const AchievementSetReleaseInfoT &releaseInfo, response->achievementSetReleaseInfo())
						{
							AchievementSetReleaseInfo info(releaseInfo.achievementSetId, releaseInfo.masterTitleId, releaseInfo.displayName, releaseInfo.platform);	

							mAchievementSetReleaseInfo.append(info);
						}
					}
					else
					{
						mAchievementSetReleaseInfo.clear();

						AchievementSetReleaseInfo bf3("BF_BF3_PC", "76889", "Battlefield 3(TM)", "PC");
						AchievementSetReleaseInfo bf4("51302_76889_50844", "76889", "Battlefield 4(TM)", "PC");
						AchievementSetReleaseInfo ds3("50563_52657_50844", "52657", "Dead Space (TM) 3", "PC");
						AchievementSetReleaseInfo ma3("68481_69317_50844", "69317", "Mass Effect(TM) 3", "PC");
						AchievementSetReleaseInfo sdk("50823_180977", "180977", "Origin SDK Tool - en_US", "PC,MAC");

						mAchievementSetReleaseInfo.append(ds3);
						mAchievementSetReleaseInfo.append(sdk);
						mAchievementSetReleaseInfo.append(bf4);
						mAchievementSetReleaseInfo.append(ma3);
						mAchievementSetReleaseInfo.append(bf3);
					}

					emit achievementSetReleaseInfoUpdated();
				}
                response->deleteLater();
			}


		}
    }
}


