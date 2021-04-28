#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>

namespace Origin
{
namespace Platform
{
    bool installLaunchAgent(const char* helperToolName);
    
    bool uninstallLaunchAgent(const char* helperToolName);
}
}