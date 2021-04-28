#ifndef ORIGINTOASTLAYOUTMANAGER_H
#define ORIGINTOASTLAYOUTMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include "UIScope.h"
#include "services/settings/Setting.h"
#include "engine/igo/IIGOWindowManager.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API OriginToastLayoutManager : public QObject, public Origin::Engine::IIGOWindowManager::IScreenListener
        {
	        Q_OBJECT
	
        public:

            static const int GenericID = 0;

            OriginToastLayoutManager(const UIScope& scope, const int poolMax, const bool vertical);
            ~OriginToastLayoutManager();
            void add(QWidget* widget){add(widget, GenericID);};
            // The id parameter is associated with the widget to add. If not set to 
            // 'GenericID', we lookup any pre-existing instance of that widget and automatically remove it.
            void add(QWidget*, int id);
            void remove(QWidget*);

        public slots:
            // IScreenListener impl
            virtual void onSizeChanged(uint32_t width, uint32_t height);
            void onDesktopSizeChanged();
            void onToastClosing();
            void update();

        private:
            void initIGOLayout();
            void initDesktopLayout();
            void verticalUpdate();
            void horizontalUpdate();
            void repositionDesktopToast(QWidget* toast, QPoint position);
            void repositionIGOToast(QWidget* toast, QPoint position);

            const UIScope  mScope;
            int            mPoolMax;
            bool           mIsVertical;
            QPoint         mScreenOffset;
            int            mScreenWidth;
            int            mScreenHeight;
            QVector<QPair<int, QPointer<QWidget>>>  mPool;

        };
    } // namespace Client
} // namespace Origin
#endif // ORIGINTOASTLAYOUTMANAGER_H