#ifndef _CLOUDSAVES_FILEFINGERPRINT
#define _CLOUDSAVES_FILEFINGERPRINT

#include <qglobal.h>

#include "services/plugin/PluginAPI.h"

// For GCC, we need to forward declare this before template instantiation occurs.
namespace Origin
{
namespace Engine
{ 
namespace CloudSaves 
{ 
    class FileFingerprint;
    uint qHash(const CloudSaves::FileFingerprint &key);
}
}
}

#include <QByteArray> 

class QFile;

namespace Origin
{
namespace Engine
{ 
namespace CloudSaves 
{
    class LocalStateCache;

    class ORIGIN_PLUGIN_API FileFingerprint
    {
    public:
        FileFingerprint(qint64 size, const QByteArray &md5) :
            m_size(size), m_md5(md5), m_valid(true)
        {
        }

        FileFingerprint() :
            m_size(-1), m_valid(false)
        {
        }

        bool isValid() const { return m_valid; }

        qint64 size() const { return m_size; }
        QByteArray md5() const { return m_md5; }

        static FileFingerprint fromFile(QFile &file);

        bool operator==(const FileFingerprint &other) const
        {
            return ((size() == other.size()) &&
                    (md5() == other.md5()));
        }

        bool operator<(const FileFingerprint &other) const
        {
            if (size() < other.size())
            {
                return true;
            }
            else if (size() > other.size())
            {
                return false;
            }
            
            // Fall back to MD5
            return md5() < other.md5();
        }

    protected:
        qint64 m_size;
        QByteArray m_md5;
        bool m_valid;
    };
}
}
}

#endif
