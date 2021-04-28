#ifndef QTUIUTILS_H
#define QTUIUTILS_H

#include <QRect>
#include <QString>
#include <QRegion>
#include <QUrl>
#include <QMenu>

#include "services/plugin/PluginAPI.h"

class QWidget;


#if (QT_VERSION < QT_VERSION_CHECK(5, 2, 0))

#ifndef QT_NO_QOBJECT
template <typename T>
struct QScopedPointerObjectDeleteLater
{
    static inline void cleanup(T *pointer) { if (pointer) pointer->deleteLater(); }
};

class QObject;
typedef QScopedPointerObjectDeleteLater<QObject> QScopedPointerDeleteLater;
#endif

#endif

namespace Origin
{
    namespace Services
    {
        ORIGIN_PLUGIN_API QString FormatToolTip(const QString & str, const QString & wordToWatch = "", int iNumAsia = 20, int iNumNotAsia = 50);
        ORIGIN_PLUGIN_API QString getFormatedWidgetSize(QWidget *w);
        ORIGIN_PLUGIN_API QRect setFormatedWidgetSizePosition(const QString& formatedSizePos, const QSize& preferredSize, bool fromUI, bool centerAsDefault=true);
   
        // This helper function locates the supported locale that is 
        // closest to the system locale
        ORIGIN_PLUGIN_API QString getNearestLocale(const QStringList& langs);


        // Sets the font for the widget to have QFont::PreferAntialias set.
        // This is useful for forcing font anti-aliasing where otherwise the
        // default might be disabled.
        ORIGIN_PLUGIN_API void EnableFontAA(QWidget* pWidget, bool bRecursive);

        // use a image to draw round corner for a menu
        ORIGIN_PLUGIN_API bool DrawRoundCornerForMenu(QMenu *menu, int r, bool flatTop =false);
        ORIGIN_PLUGIN_API QRegion roundedRect(const QRect& rect, int r,bool flatTop);

#ifdef WIN32

        // Prevent the app from being listed in the recent menu
        // and also be pinned to the taskbar in Win7
        ORIGIN_PLUGIN_API bool HideAppFromRecentMenu(const QString& fileName);

        enum TaskbarPosition {
            NoTaskbar		 = 0x00000000,
            TaskbarOnBottom  = 0x00000001,
            TaskbarOnTop	 = 0x00000002,
            TaskbarOnLeft	 = 0x00000003,
            TaskbarOnRight	 = 0x00000004
        };
        ORIGIN_PLUGIN_API TaskbarPosition CalculateTaskbarPosition();
        //Get windows account
        ORIGIN_PLUGIN_API QString GetWindowsAccount();
#endif
        ORIGIN_PLUGIN_API void OpenAddOrRemovePrograms();

        /// \brief returns available physical memory
        ORIGIN_PLUGIN_API unsigned int memory();

        ORIGIN_PLUGIN_API QByteArray EncodeUrlForTelemetry(const QUrl& url);

        // This object can be used to delete objects in a separate thread and send a notification when the objects are actually gone before deleting itself.
        // Simply create an instance on your thread with the list of objects to delete, connect to its objectsDeleted() signal, 
        // move the object to the thread that is actually managing the objects to be deleted, and call 'start()'
        // If you don't want to use the default 'deleteLater()' method to delete the object, you can specify your own in the 
        // object constructor - however you are then responsible for calling 'deleteLater()' (which is required to properly clean up the
        // reaper object)
        class ORIGIN_PLUGIN_API ThreadObjectReaper : public QObject
        {
            Q_OBJECT

        public:
            ThreadObjectReaper(QObject* obj, const QString& deleteMethodName = "");
            ThreadObjectReaper(const QVector<QObject*>& objs, const QString& deleteMethodName = "");

        signals:
            void start();
            void objectsDeleted();

        private slots:
            void onStart();
            void onDelete(QObject* obj);

        private:
            QString mDeleteMethodName;
            QVector<QObject*> mThreadObjects;
        };
    }
}

#endif //QTUIUTILS_H
