
#include "NonOriginGameData.h"

namespace Origin
{

namespace Engine
{

namespace Content
{
        NonOriginGameData::NonOriginGameData() :
            mIsIgoEnabled(true)
        {

        }

        NonOriginGameData::NonOriginGameData(const NonOriginGameData& other)
        {
            if (this != &other)
            {
                this->mDisplayName = other.mDisplayName;
                this->mExecutableFile = other.mExecutableFile;
                this->mExecutableParameters = other.mExecutableParameters;
                this->mEntitleDate = other.mEntitleDate;
                this->mIsIgoEnabled = other.mIsIgoEnabled;
            }
        }

        NonOriginGameData& NonOriginGameData::operator=(const NonOriginGameData& other)
        {
            if (this != &other)
            {
                this->mDisplayName = other.mDisplayName;
                this->mExecutableFile = other.mExecutableFile;
                this->mExecutableParameters = other.mExecutableParameters;
                this->mEntitleDate = other.mEntitleDate;
                this->mIsIgoEnabled = other.mIsIgoEnabled;
            }

            return *this;
        }
    
        void NonOriginGameData::setDisplayName(const QString& displayName)
        {
            mDisplayName = displayName;
        }

        void NonOriginGameData::setExecutableFile(const QString& executableFile)
        {
            mExecutableFile = executableFile;
        }

        void NonOriginGameData::setExecutableParameters(const QString& exeParams)
        {
            mExecutableParameters = exeParams;
        }

        void NonOriginGameData::setEntitleDate(const QDateTime& entitleDate)
        {
            mEntitleDate = entitleDate;
        }

        void NonOriginGameData::setIgoEnabled(bool isEnabled)
        {
           mIsIgoEnabled = isEnabled;
        }

        QString NonOriginGameData::getProductId() const
        {
            return QString("NOG_%1").arg(qHash(mExecutableFile));
        }

        QString NonOriginGameData::getContentId() const
        {
            return getProductId();
        }

        QString NonOriginGameData::getMasterTitleId() const
        {
            return getProductId();
        }

        QString NonOriginGameData::getDisplayName() const
        {
            return mDisplayName;
        }

        QString NonOriginGameData::getExecutableFile() const
        {
            return mExecutableFile;
        }

        QString NonOriginGameData::getExecutableParameters() const
        {
            return mExecutableParameters;
        }

        QDateTime NonOriginGameData::getEntitleDate() const
        {
            return mEntitleDate;
        }

        Services::Publishing::IGOPermission NonOriginGameData::getIgoPermission() const
        {
            if (mIsIgoEnabled)
            {
               return Services::Publishing::IGOPermissionSupported;
            }

            return Services::Publishing::IGOPermissionBlacklisted;
        }

        bool NonOriginGameData::isIgoEnabled() const
        {
            return mIsIgoEnabled;
        }
}

}

}
