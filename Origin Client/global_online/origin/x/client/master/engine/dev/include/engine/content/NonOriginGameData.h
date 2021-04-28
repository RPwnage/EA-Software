#ifndef NONORIGINGAMEDATA_H
#define NONORIGINGAMEDATA_H

#include "services/entitlements/BoxartData.h"
#include "services/publishing/CatalogDefinition.h"

#include <QDateTime>
#include <QRect>
#include <QString>

namespace Origin
{

namespace Engine
{

namespace Content
{

class NonOriginGameData
{
    public:

        NonOriginGameData();
        NonOriginGameData(const NonOriginGameData& other);

        NonOriginGameData& operator=(const NonOriginGameData& other);
    
        void setDisplayName(const QString& displayName);
        void setExecutableFile(const QString& executableFile);
        void setExecutableParameters(const QString& exeParams);
        void setEntitleDate(const QDateTime& entitleDate);
        void setIgoEnabled(bool isEnabled);

        QString getProductId() const;
        QString getContentId() const;
        QString getMasterTitleId() const;

        QString getDisplayName() const;
        QString getExecutableFile() const;
        QString getExecutableParameters() const;
        QDateTime getEntitleDate() const;
        Services::Publishing::IGOPermission getIgoPermission() const;
        bool isIgoEnabled() const;

    private:
    
        QString mDisplayName;
        QString mExecutableFile;
        QString mExecutableParameters;
        QDateTime mEntitleDate;
        bool mIsIgoEnabled;
};

}

}

}

Q_DECLARE_METATYPE(Origin::Engine::Content::NonOriginGameData);

#endif // NONORIGINGAMEDATA_H
