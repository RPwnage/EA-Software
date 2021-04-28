///////////////////////////////////////////////////////////////////////////////
/// \file       achievementportfolio.h
/// This file contains the declaration of the AchievementPortfolio. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#ifndef __SDK_ACHIEVEMENT_PORTFOLIO_H__
#define __SDK_ACHIEVEMENT_PORTFOLIO_H__

#include <QObject>
#include <QHash>
#include <QList>
#include <QSharedPointer>
#include <QMutex>
#include <QStringList>
#include <QDomDocument>

#include "achievementset.h"
#include "services/session/SessionService.h"
#include "services/achievements/AchievementServiceResponse.h"
#include "services/plugin/PluginAPI.h"
#include "engine/content/Entitlement.h"

namespace Origin
{
    namespace Engine
    {
        namespace Achievements
        {
            class ORIGIN_PLUGIN_API AchievementPortfolio : public QObject
                {
                friend class AchievementManager;
                Q_OBJECT
            public:
                ~AchievementPortfolio();

                const QList<AchievementSetRef> &achievementSets() const; ///< Get the list of achievement sets.

            signals:
                void availableUpdated();                                ///< Signal that the portfolio tried

                void added(Origin::Engine::Achievements::AchievementSetRef addedAchievementSet);      ///< Signal indicating that an achievement set has been added to the portfolio.
                void removed(Origin::Engine::Achievements::AchievementSetRef removedAchievementSet);  ///< Signal indicating that an achievement set has been removed from the portfolio.

                void granted(Origin::Engine::Achievements::AchievementSetRef set, Origin::Engine::Achievements::AchievementRef achievement);               ///< Signal indicating that an achievement has been granted.
                void xpChanged(int newXPValue);                         ///< Signals the new value of the experience points.
                void rpChanged(int newRPValue);                         ///< Signals the new value of the reward points.

            private slots:
                void pointsUpdateCompleted();                           ///< Called from service when the point data is received.
                void achievementSetUpdateCompleted();                   ///< Called from service when an achievement set data is received.
                void achievementSetsUpdateCompleted();                  ///< Called from services when all achievements query returns.
                void achievementGranted(Origin::Engine::Achievements::AchievementSetRef set, Origin::Engine::Achievements::AchievementRef achievement);

            public slots:
                void clear();                                           ///< Removes the content of the achievement portfolio. 
                void refreshPoints();                                   ///< Update the experience points and reward points for the user.
                void updateAchievementSet(const QString &achievementSetId);     ///< Update the achievement set.

            public:
                bool available() const;                                 ///< Indication whether the achievement information is available for this user.

                int total() const;                                      ///< The total number of achievements for the user.
                int granted() const;                                    ///< The total number of achievements granted for the user.
                int xp() const;                                         ///< The current value of the experience points.
                int rp() const;                                         ///< The current value of the reward points.

                QString personaId() const;                              ///< The personaId associated with this portfolio
                QString userId() const;                                 ///< The userId associated with this portfolio
                bool isUser() const;                                    ///< Returns true if the portfolio belongs to the current user.

                void refresh();                                         ///< Refreshes all content contained in the achievement manager. 

                void updateAchievementSets(const QStringList &achievementSets = QStringList(), 
                    const QList<Engine::Content::EntitlementRef> &entitlements = QList<Engine::Content::EntitlementRef>());    ///< Adds/Updates the achievement sets in the achievement manager. (Deletion is not supported unless calling clear())

                AchievementSetRef getAchievementSet(QString achievementSetId);

                void serialize();                                       ///< Ask the achievement manager to serialize all achievement information for the user.
                                                                        ///< The operation may be ignored in cases where additional data is expected soon.
                void serialize(QDomElement &node, int version, bool bSave);
            protected:
                void granted(server::AchievementNotificationT notification);

            private:
                ///< Creates a new achievementset with provided achievementSetId. 
                 AchievementSetRef createAchievementSet(const QString &achievementSetId, const QString &name = QString(), const QString &platform = QString()); 

                explicit AchievementPortfolio(const QString &userId, const QString &personaId, QObject * parent);

                QStringList getAchievementSetIds(QString setId) const;

            private:
                bool mAvailable;                                        ///< Indicates whether the achievement portfolio is available for this user.
                bool mSuppressGranted;                                  ///< When true suppress granted notifications.

                int mXp;                                                ///< The current experience point value.
                int mRp;                                                ///< The current reward point value.
                int mNumAchievements;                                   /// <the current number of achievements

                QString mUserId;                                        ///< The userId associated with this portfolio
                QString mPersonaId;                                     ///< The personaId associated with this portfolio


                QHash<QString, AchievementSetRef>                       mAchievementSetMap;                ///< Find the achievement set depending on the set name.
                QList<AchievementSetRef>                                mAchievementSets;                  ///< The achievement set is this portfolio.
                QHash<QString, Engine::Content::EntitlementWRef>        mAchievementSetToEntitlementMap;   ///< Map the achievement set name to the entitlement.

                QMutex                                                  mPendingUpdateGuard;               ///< Protect the Pending Update list from being modified from multiple threads.
                QList<Origin::Services::Achievements::AchievementSetQueryResponse *>              mPendingUpdates;                   ///< The pending update list.

            };

            typedef QSharedPointer<AchievementPortfolio> AchievementPortfolioRef;
            typedef QWeakPointer<AchievementPortfolio> AchievementPortfolioWRef;

        }
    }
}

Q_DECLARE_METATYPE(Origin::Engine::Achievements::AchievementPortfolioRef);


#endif // __SDK_ACHIEVEMENT_PORTFOLIO_H__