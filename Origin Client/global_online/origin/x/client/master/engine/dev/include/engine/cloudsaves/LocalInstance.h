#ifndef _CLOUDSAVES_LOCALINSTANCE_H
#define _CLOUDSAVES_LOCALINSTANCE_H

#include <qglobal.h>

// For GCC, we need to forward declare this before template declaration/instantiation occurs
namespace Origin
{
namespace Engine
{
namespace CloudSaves
{ 
    class LocalInstance;
}
}
}

#include <limits>
#include <QDateTime>
#include <QString>
#include <QFileInfo>
#include <QHash>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
   /**
    * Represents a local instance of a remote file
    *
    * Note that for identical files there can be multiple local instances for
    * a single remote
    */
   class ORIGIN_PLUGIN_API LocalInstance
   {
   public:
      enum FileTrust
      {
         // Normal for local files
         Trusted,
         // Indicates we are on the global blacklist
         Untrusted,
         // Normal for remote files
         UnknownTrust
      };

      // Useless default constructor - mostly so we can have structs etc
      // containing us
      LocalInstance() :
            m_null(true),
            m_trust(UnknownTrust)
      {
      }

      LocalInstance(const QString &path, const QDateTime &lastModified, const QDateTime &created, FileTrust trust = UnknownTrust) : 
         m_null(false),
         m_path(path),
         m_lastModified(lastModified.toUTC()),
         m_created(created.toUTC()),
         m_trust(trust)
      {
      }

      LocalInstance withTrust(FileTrust trust) const
      {
         LocalInstance clone = *this;
         clone.m_trust = trust;
         return clone;
      }
      
      static LocalInstance fromFileInfo(const QFileInfo &info, FileTrust trust = UnknownTrust)
      {
         return LocalInstance(info.canonicalFilePath(), info.lastModified(), info.created(), trust);
      }

      bool isNull() const { return m_null; }

      const QString &path() const { return m_path; }

      // We consider identical paths identical; everything else is advisory metadata
      bool operator==(const LocalInstance &other) const 
      {
         return other.path() == path();
      }

      const QDateTime &lastModified() const { return m_lastModified; }
      const QDateTime &created() const { return m_created; }
      FileTrust trust() const { return m_trust; }

   private:
      bool m_null;

      QString m_path;
      QDateTime m_lastModified;
      QDateTime m_created;

      FileTrust m_trust;
   };
    
    inline uint qHash(const Origin::Engine::CloudSaves::LocalInstance &key)
    {
        return qHash(key.path());
    }
}
}
}

#endif
