///////////////////////////////////////////////////////////////////////////////
/// \file       achievement.h
/// This file contains the declaration of an achievement. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#ifndef __SDK_ACHIEVEMENT_H__
#define __SDK_ACHIEVEMENT_H__

#include <limits>
#include <QDateTime>
#include <QObject>
#include <QMetaType>
#include <QSharedPointer>
#include <QDomDocument>
#include <services/achievements/AchievementServiceResponse.h>
#include <services/plugin/PluginAPI.h>
namespace Origin
{
    namespace Engine
    {
        namespace Achievements
        {
            class AchievementSet;
            typedef QSharedPointer<class Achievement> AchievementRef; 
            typedef QWeakPointer<class Achievement> AchievementWRef; 

            class ORIGIN_PLUGIN_API Achievement : public QObject
            {
                friend class AchievementSet;

                Q_OBJECT
            public:
                //////////////////////////////////////////////////////////////////////////
                /// \brief achievement constructor
                ///
                /// \param [in] id The index of the achievement, no special meaning should be assigned to this (in general)
                /// \param [in] achievement The achievement information from the server.
                /// \param parent TBD.
                Achievement(QString id, const Origin::Services::Achievements::AchievementT & achievement, AchievementSet * parent);

                //////////////////////////////////////////////////////////////////////////
                /// \brief achievement constructor for serialization
                ///
                /// \param [in] id The index of the achievement, no special meaning should be assigned to this (in general)
                /// \param parent TBD.
                Achievement(QString id, AchievementSet * parent);

            public:
                QString id() const;                 ///< The identifier for this achievement.
                QString expansionId() const;        ///< The identifier for the expansion. If empty it is a base game achievement.

                QDateTime updateTime() const;	    ///< The time of the latest modification to the achievement.
                QDateTime expirationTime() const;   ///< The time when the achievement will expire.

                int progress() const;			    ///< The progress on the achievement.
                int total() const;				    ///< The total counts you need to get the achievement.
                int count() const;				    ///< The number of times awarded.
                int xp() const;                     ///< Experience points
                int rp() const;                     ///< Reward points 
                QString xp_t() const;               ///< Experience points visual
                QString rp_t() const;               ///< Reward points visual

                QString name() const;				///< The name of the achievement in the Origin locale.
                QString description() const;		///< A detailed description of the achievement.
                QString howto() const;			    ///< A description on how to get the achievement.

                QString getImageUrl(int size) const;      ///< Get the Image Url for a certain size.


                void setSelf(AchievementRef self);  ///< To allow to have the achievement giving away strong pointers to itself.

                void serialize(QDomElement &node, int version, bool bSave);

                AchievementSet * achievementSet(){return mAchievementSet;}

                void setDeleted(bool del){mDeleted = del;}
                bool deleted(){return mDeleted;}
            signals:
                void progressChanged(int progress, int total, int count);   ///< The progress of the achievement changed.
                void updated();             ///< Fired when the achievement configuration changes.
                void granted(Origin::Engine::Achievements::AchievementRef achievement);             ///< Fired when the achievement is granted.

            protected:
                void update(const Origin::Services::Achievements::AchievementT & achievement, bool bSuppressGranted = false);  ///< Updates an existing achievement.

                QString imageName() const;

                void updateImageCache() const;

            private:
                QString mId;                ///< The identifier for this achievement.

                QDateTime mUpdateTime;		///< The time of the latest modification to the achievement.
                QDateTime mExpirationTime;	///< The time when the achievement will expire.

                int mProgress;				///< The progress on the achievement.
                int mTotal;					///< The total counts you need to get the achievement.
                int mCount;					///< The number of times awarded.
                int mXp;                    ///< Experience points
                int mRp;                    ///< Reward points 
                QString mXp_t;              ///< Experience points visual
                QString mRp_t;              ///< Reward points visual

                QString mName;				///< The name of the achievement in the Origin locale.
                QString mDescription;		///< A detailed description of the achievement.
                QString mHowto;				///< A description on how to get the achievement.
                QString mImageName;         ///< The name of the image.
                
                QString mExpansionId;       ///< Reference to the expansion.

                QHash<int, QString> mIcons; ///< Urls to achievement icons.

                AchievementSet * mAchievementSet; ///< A weak reference to the achievement set.

                AchievementWRef mSelf;      ///< Weak reference to self.

                bool mDeleted;              ///< Indicator that this achievement shouldn't be serialized to the offline cache.
            };
        }
    }
}

Q_DECLARE_METATYPE(Origin::Engine::Achievements::AchievementRef);

#endif //__SDK_ACHIEVEMENT_H__