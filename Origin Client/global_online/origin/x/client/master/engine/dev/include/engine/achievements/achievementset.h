///////////////////////////////////////////////////////////////////////////////
/// \file       achievementset.h
/// This file contains the declaration of an AchievementSet. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#ifndef __SDK_ACHIEVEMENT_SET_H__
#define __SDK_ACHIEVEMENT_SET_H__

#include <limits>
#include <QDateTime>
#include <QObject>
#include <QHash>
#include <QMap>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QDomDocument>

#include "services/achievements/AchievementServiceResponse.h"
#include "services/plugin/PluginAPI.h"
#include "engine/content/Entitlement.h"
#include "achievement.h"

namespace Origin
{
    namespace Engine
    {
        namespace Achievements
        {

            struct AchievementSetExpansion
            {
                AchievementSetExpansion() : order(-1), deleted(false) {}

                QString id;
                QString name;
                bool owned;
                int order;

                bool deleted;
            };

            typedef QSharedPointer<class AchievementSet> AchievementSetRef; 
            typedef QWeakPointer<class AchievementSet> AchievementSetWRef; 
            
            typedef QList<AchievementRef> AchievementList;
            typedef QList<AchievementSetExpansion> AchievementSetExpansionList;

            class ORIGIN_PLUGIN_API AchievementSet : public QObject
            {
                Q_OBJECT
            public:
                typedef QHash<QString, AchievementRef> AchievementMap;

                AchievementSet(const QString &achievementSetId, const QString &name = QString(), const QString &platform = QString(), Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::EntitlementRef());

                void updateFromEntitlement(Origin::Engine::Content::EntitlementRef entitlement);
                void setExpansionOwnershipFromEntitlement( Engine::Content::EntitlementRef entitlement );

            public:
                const AchievementList & achievements() const;	///< The list of achievements associated with this achievement set.
                const AchievementSetExpansionList & expansions() const; ///< The list of expansions associated with this achievement set.

                const QString &platform() const;
                const QString &displayName() const;
                
                const QString &achievementSetId() const;                        ///< returns the achievement set id.
                Origin::Engine::Content::EntitlementRef entitlement() const;    ///< Return the associated entitlement.             

                void update(const Origin::Services::Achievements::AchievementMap &achievements, const Origin::Services::Achievements::ExpansionList &expansions, const QString &name = QString(), const QString &platform = QString(), bool bSuppressGranted = false);

                void setSelf(AchievementSetRef self);  ///< To allow to have the achievement set giving away strong pointers to itself.

                AchievementRef getAchievement(QString id){ return mAchievementMap[id]; }

                void addExpansion(QString expansionId, QString expansionName, bool owned = false, int order=-1);

                void serialize(QDomElement &node, int version, bool bSave);

                QDateTime lastUpdated() const { return mLastUpdated; }
                bool isUpdated() const { return mUpdated; }
                void setUpdated(bool val) { mUpdated = val; emit updated(); }

                int achieved() const;
                int totalXp() const;
                int totalRp() const;
                int earnedXp() const;
                int earnedRp() const;

                AchievementSetExpansionList::iterator findExpansion(QString expansionId);

                void setDeleted(bool del){mDeleted = del;}
                bool deleted(){return mDeleted;}

            signals:
                void updated();                                                                 ///< Signals that the achievementset is updated with new data.
                void added(Origin::Engine::Achievements::AchievementRef newAchievement);        ///< Signals that a new achievement is added to the achievement set.
                void removed(Origin::Engine::Achievements::AchievementRef deletedAchievement);  ///< Signals that an achievement has been removed from the achievement set.
                void expansionAdded(QString);                                                   ///< Signals that an expansion has been added to the achievement set.
                void expansionRemoved(QString);                                                 ///< Signals that an expansion has been removed from the achievement set.
                void achievementGranted(Origin::Engine::Achievements::AchievementSetRef set, Origin::Engine::Achievements::AchievementRef achievement);     ///< Signals that one of the achievements in the set was granted.
                
            private slots:
                void granted(Origin::Engine::Achievements::AchievementRef achievement);  ///< When one of the achievements in the set gets updated with a newer count. This slot is called.

            private:
                AchievementRef createAchievement(QString index, const Origin::Services::Achievements::AchievementT & achievement);
                AchievementRef createAchievement(QString index);
                void setPlatformFromEntitlement( Engine::Content::EntitlementRef entitlement );

            private:
                QString mAchievementSetId;      ///< The name of the achievement set.
                QString mPlatform;              ///< The platform this achievement set is for.
                QString mDisplayName;                  ///< The name of the game associated with this achievement.

                bool mDeleted;                  ///< Indicate that this achievement set should not be serialized.
                bool mUpdated;                  ///< Indicates whether the achievementset was updated in the last request.
                QDateTime mLastUpdated;         ///< Giving the date/time in UTC when the achievement set was successfully updated last.

                AchievementMap mAchievementMap; ///< The achievements in the set.
                AchievementList mAchievements;
                Origin::Engine::Content::EntitlementWRef mEntitlement;

                AchievementSetExpansionList mExpansions;

                AchievementSetWRef mSelf;
            };

        }
    }
}

Q_DECLARE_METATYPE(Origin::Engine::Achievements::AchievementSetRef);


#endif //__SDK_ACHIEVEMENT_SET_H__