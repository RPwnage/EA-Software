#include "SubstitutionPaths.h"
#import <Cocoa/Cocoa.h>

namespace
{
    QString directoryForSearchPath(NSSearchPathDirectory directory)
    {
        NSArray *urlList = [[NSFileManager defaultManager] URLsForDirectory: directory inDomains: NSUserDomainMask];
        
        if ([urlList count] < 1)
        {
            // No URLs returned
            return QString();
        }
        
        NSURL *firstUrl = (NSURL*)[urlList objectAtIndex: 0];
        
        if (![firstUrl isFileURL])
        {
            // Not a file:/// URL 
            return QString();
        }
        
        NSString *path = [firstUrl path];
        return QString::fromUtf8([path UTF8String]);
    }
}


namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    const QMap<QString, QString> queryOSSubstitutionPaths()
    {
        QMap<QString, QString> ret;

        ret["%Home%"] = QString::fromUtf8([NSHomeDirectory() UTF8String]);
        ret["%Library%"] = directoryForSearchPath(NSLibraryDirectory);
        ret["%Document%"] = directoryForSearchPath(NSDocumentDirectory);
        
        return ret;
    }
}
}
}
