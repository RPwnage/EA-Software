#include "services/downloader/MounterQt.h"
#include "services/log/LogService.h"

#include <QStringList>

#ifdef __APPLE__
#include "services/platform/MounterOSX.h"
#endif

namespace Origin
{
namespace Downloader
{

MounterQt::MounterQt() : QObject(0)
{
#ifdef __APPLE__
    
    mounty = 0;
    
#endif
    
}
    
MounterQt::~MounterQt()
{
    release();
}
    
void MounterQt::release()
{


#ifdef __APPLE__
    if (mounty)
    {
        delete mounty;
        mounty = 0;
    }
#endif
        
}

void MounterQt::mount(const QString &path)
{
    // An NSTask can only be executed once or it throws an exception on reentry
    // So make sure we have a fresh one
    //
    release();
    
    mount_path = "";
    mount_dev = "";
    
#ifdef __APPLE__
    mounty = new Origin::Platform::MounterOSX();
    mounty->mount(path, this);
#else
    
    ORIGIN_LOG_EVENT << "Installing from disk image currently only supported on Mac!";

#endif
    
}

void MounterQt::mount_terminated(int status, QString output)
{
    QString local_stdout = output;
    
    QStringList columns = local_stdout.split("\t");
      
    if (status == 0)
    {
        for(int i = 0; i < columns.size(); i++)
        {
            QString trimmed = columns.at(i).trimmed();
    
            // First /dev/ string is our moint point
            if (mount_dev.isEmpty() && trimmed.startsWith("/dev/"))
            {
                mount_dev = trimmed;
            }
        
            // First /Volumes/ string is our mount path
            if (mount_path.isEmpty() && trimmed.startsWith("/Volumes/"))
            {
                mount_path = trimmed;
            }
        
        }
    }
    else
    {
        // Failed to mount, stick hdiutil's output as our 'mount_path' for display back up the chian
        mount_path = output;
    }
    
    emit mountComplete(status, mount_path);
}
    
void MounterQt::unmount_terminated(int status, QString output)
{       
    mount_path = "";
    mount_dev = "";

    emit unmountComplete(status);
}

void MounterQt::unmount()
{
    release();
    

#ifdef __APPLE__
    
    if (!mount_dev.isEmpty())
    {
        mounty = new Origin::Platform::MounterOSX();
        mounty->unmount(mount_dev, this);
    }
    else
    {
        ORIGIN_LOG_EVENT << "Attempted to unmount null device, simulating successful unmount";
        emit unmount_terminated(0, "");
    }
#else
    
    ORIGIN_LOG_EVENT << "Tried to unmount an image from an supported (non-Mac) platform";
    
#endif
    
}

}
}
