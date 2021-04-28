#ifndef MOUNTER_OSX_H
#define MOUNTER_OSX_H

#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Downloader
{
    class MounterQt;
}
    
namespace Platform
{

class ORIGIN_PLUGIN_API MounterOSX {

    public:

        MounterOSX();
        ~MounterOSX();

        void mount(const QString& path, Origin::Downloader::MounterQt* caller);
        void unmount(const QString &path, Origin::Downloader::MounterQt* caller); 

    private:

        class NSInternals;
        NSInternals *NS;
};

}
}

#endif // MOUNTER_OSX_H

