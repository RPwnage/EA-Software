#include <windows.h>
#include <shlobj.h> 
#include <QDir>

#include "SubstitutionPaths.h"

namespace
{
    QString directoryForCSIDL(int csidl) 
    {
        wchar_t pathBuffer[MAX_PATH + 1];
        
        // Needs to be per-user as game saves can be stored in the user's directory
        // Shouldn't be roaming as it only applies to the local computer
        SHGetFolderPath(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, pathBuffer);

        return QDir(QString::fromWCharArray(pathBuffer)).canonicalPath();
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
        ret["%AppData%"] = directoryForCSIDL(CSIDL_APPDATA);
        ret["%LocalAppData%"] = directoryForCSIDL(CSIDL_LOCAL_APPDATA);
        ret["%Documents%"] = directoryForCSIDL(CSIDL_MYDOCUMENTS);
        ret["%ProgramData%"] = directoryForCSIDL(CSIDL_COMMON_APPDATA);
        
        // This is Vista+ only
        typedef HRESULT (WINAPI *SGKFP)(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR*);
        SGKFP pGetKnownFolderPath = (SGKFP)GetProcAddress(GetModuleHandle(L"shell32"), "SHGetKnownFolderPath");
        
        if (pGetKnownFolderPath)
        {
            PWSTR path;

            if (pGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, &path) == S_OK)
            {
                ret["%SavedGames%"] = QString::fromUtf16(path);
                CoTaskMemFree(path);
            }
        }
        
        ret["%Profile%"] = directoryForCSIDL(CSIDL_PROFILE);

        return ret;
    }
}
}
}
