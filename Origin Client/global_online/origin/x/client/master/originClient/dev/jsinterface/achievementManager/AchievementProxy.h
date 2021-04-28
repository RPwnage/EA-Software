#ifndef _ACHIEVEMENTPROXY_H
#define _ACHIEVEMENTPROXY_H

/**********************************************************************************************************
* This class is part of Origin's JavaScript bindings and is not intended for use from C++
*
* All changes to this class should be reflected in the documentation in jsinterface/doc
*
* See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
* ********************************************************************************************************/

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVariant>
#include <services/settings/Setting.h>
#include <engine/achievements/achievement.h>
#include <services/plugin/PluginAPI.h>

namespace Origin
{
    namespace Client
    {
        /// \brief <span style="font-size:14px" >Please view the</span> <a href="./bridgedocgen/index.xhtml" style="color:orange;font-size:14px;font-weight:bold">JavaScript Bridge Documentation</a>.
        ///
        /// <span style="font-size:14pt;color:#404040;"><b>See the relevant wiki docs:</b></span> <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/Origin+Javascript+API">Origin Javascript API</a> <span style="font-size:14pt;color:#404040;">and</span> <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets">Working with Web Widgets</a><span style="font-size:14pt;color:#404040;">; also, please be familiar with %Origin %Client Development's</span> <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/JavaScript+Coding+Standard">JavaScript Coding Standard</a>.
        namespace JsInterface
        {
            class AchievementSetProxy;

            class ORIGIN_PLUGIN_API AchievementProxy : public QObject
            {
                friend class AchievementSetProxy;
                Q_OBJECT

            public:
                Q_PROPERTY(QString id READ id)			                    ///< The identifier of the achievement.
                Q_PROPERTY(QString expansionId READ expansionId);           ///< The identifier of the expansion this achievement belongs to. "" is base game.

                Q_PROPERTY(QDateTime updateTime READ updateTime)			///< The time of the latest modification to the achievement.
                Q_PROPERTY(QDateTime expirationTime READ expirationTime)	///< The time when the achievement will expire.

                Q_PROPERTY(int progress READ progress)						///< The progress on the achievement.
                Q_PROPERTY(int total READ total)							///< The total counts you need to get the achievement.
                Q_PROPERTY(int count READ count)							///< The number of times awarded.

                Q_PROPERTY(QString xp READ xp_t)							///< The experience points visual.
                Q_PROPERTY(QString rp READ rp_t)							///< The reward points visual.

                Q_PROPERTY(QString name READ name)							///< The name of the achievement in the Origin locale.
                Q_PROPERTY(QString description READ description)			///< A detailed description of the achievement.
                Q_PROPERTY(QString howto READ howto)						///< A description on how to get the achievement.

                Q_PROPERTY(QObject * achievementSet READ achievementSet)    ///< Get the owning achievementset.

                // Helper properties
                Q_PROPERTY(bool achieved READ achieved )                    ///< Indicator that the achievement is achieved.
                Q_PROPERTY(float percent READ percent )                     ///< [0, 1] normalized progress.

                Q_PROPERTY(bool pinned READ pinned WRITE setPinned)         ///< Indicates whether the achievement is pinned.
                Q_PROPERTY(bool withinReachHidden READ withinReachHidden WRITE setWithinReachHidden)         ///< Indicates whether the achievement is hidden in some view.
               
                Q_INVOKABLE QString getImageUrl(int size) const;		    ///< Get the image URL.

            signals:
                void achievementGranted();                                             ///< Fired when the achievement is granted.
                void achievementProgressChanged(int progress, int total, int count);   ///< Fires when progress is made on the achievement.   
                void achievementUpdated();                                             ///< Fired when any of the configuration parameters changed. (Strings etc.)
                void pinnedChanged(bool bPinned);                                      ///< Fired when the pinned value changes.
                void withinReachHiddenChanged(bool bWithinReachHidden);                ///< Fired when the hidden value changes.

            public:
                QString id() const;                     ///< The identifier of the achievement.
                QString expansionId() const;            ///< The identifier of the expansion this achievement belongs to.

                QDateTime updateTime() const;		///< The time of the latest modification to the achievement.
                QDateTime expirationTime() const;   ///< The time when the achievement will expire.

                int progress() const;				///< The progress on the achievement.
                int total() const;				    ///< The total counts you need to get the achievement.
                int count() const;				    ///< The number of times awarded.

                int xp() const;				        ///< The experience points.
                int rp() const;				        ///< The reward points.
                QString xp_t() const;				///< The experience points.
                QString rp_t() const;				///< The reward points.

                QString name() const;				///< The name of the achievement in the Origin locale.
                QString description() const;		///< A detailed description of the achievement.
                QString howto() const;			    ///< A description on how to get the achievement.

                QObject * achievementSet() const;

                // Helper functions
                bool achieved() const;              ///< Returns true when the achievement is granted, else false. In some cases the progress may go to zero when the achievement is achieved. Always check whether achieved() returns true
                float percent() const;              ///< Returns the normalized progress [0, 1]

                bool pinned() const;                ///< Indicator whether this achievement is pinned.
                void setPinned(bool bPinned);       ///< Modify the pinned flag

                bool withinReachHidden() const;                ///< Indicator whether this achievement is hidden in some views. Don't hide in normal achievement lists.
                void setWithinReachHidden(bool bHidden);       ///< Modify the pinned flag
            private:
                explicit AchievementProxy(Origin::Engine::Achievements::AchievementRef achievement, 
                                          AchievementSetProxy * parent);

                Origin::Engine::Achievements::AchievementRef mAchievement;
                
                Origin::Services::Setting mPinned;
                Origin::Services::Setting mbWithinReachHidden;

                AchievementSetProxy * mAchievementSet;
            };
        }
    }
}

#endif //_ACHIEVEMENTPROXY_H
