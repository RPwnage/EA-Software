#ifndef MOUNTER_QT_H
#define MOUNTER_QT_H

#include <QObject>
#include <QString>

#include "services/plugin/PluginAPI.h"


namespace Origin
{
        
namespace Platform
{
    
#ifdef __APPLE__
    class MounterOSX;
#endif
    
}

namespace Downloader
{

class ORIGIN_PLUGIN_API MounterQt : public QObject
{
    Q_OBJECT

    public:

        MounterQt();
        ~MounterQt();

        QString mountPath() { return mount_path; }
    
    public slots:

        void mount(const QString &path);
        void unmount();

        // Callbacks for the platform mounter
        void mount_terminated(int status, QString output);
        void unmount_terminated(int status, QString output);

    signals:

        void mountComplete(int status, QString output);
        void unmountComplete(int status);
    
    private:

        void release();
    
        QString mount_path, mount_dev;
    
#ifdef __APPLE__
        Origin::Platform::MounterOSX* mounty;
#endif
    
};

}
}


#endif // MOUNTER_QT_H
