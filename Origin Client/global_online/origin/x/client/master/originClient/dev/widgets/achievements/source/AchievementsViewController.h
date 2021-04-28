//    Copyright © 2012, Electronic Arts
//    All rights reserved.

#ifndef ACHIEVEMENTSVIEWCONTROLLER_H
#define ACHIEVEMENTSVIEWCONTROLLER_H

#include <QObject>
#include <QUrl>

#include "engine/login/User.h"
#include "services/plugin/PluginAPI.h"

class QWidget;

namespace WebWidget
{
    class WidgetView;
}

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API AchievementsViewController : public QObject
        {
            Q_OBJECT
        public:
            AchievementsViewController(Engine::UserRef user, QWidget *parent = 0);
            ~AchievementsViewController();

            QWidget *view() const;

            void showAchievementSetDetails(const QString& achievementSetId, const QString& userId = "", const QString& gameTitle = "");
            void showAchievementHome(bool recordNavigation);
        public slots:
            void onLoadUrl(QString);
            
        protected slots:
            void onUrlChanged(const QUrl& url);

        private:
            WebWidget::WidgetView *mAchievementsView;

            enum LastLink
            {
                NoLink
                ,Home
                ,Details
            } mLastLink;

        };

    }
} 

#endif //MYGAMESVIEWCONTROLLER_H
