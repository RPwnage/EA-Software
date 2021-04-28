#include "services/common/VersionInfo.h"
//#include "WideCharUtils.h"
//#include "patch_util.h"            // for splittoken

#include <stdlib.h>
#include <string>

namespace Origin
{

std::string SplitToken(std::string& str, const char* pToken)
{
        if (pToken == 0)
                return str;

        const size_t nLength = strlen(pToken);
        if (nLength == 0)
                return str;

        const size_t nIndex = str.find(pToken, 0, nLength);
        if (nIndex == (size_t)-1)
                return std::string();

        const std::string sLeft(str, 0, nIndex);
        str.erase(0, nIndex + nLength);

        return sLeft;
}

VersionInfo::VersionInfo( int major /*= 0*/, int minor /*= 0*/, int build /*= 0*/, int rev /*=0*/)
        : mMajor(major), mMinor(minor), mBuild(build), mRev(rev)
{}

VersionInfo::VersionInfo( const char* str )
        : mMajor(0), mMinor(0), mBuild(0), mRev(0)
{
        if (!str)
                return;

        std::string sVersion(str);

        std::string sMajor = SplitToken(sVersion, ".");
        if (sMajor.empty())
        {
                mMajor = atoi( sVersion.c_str() );
                return;    }
        mMajor = atoi(sMajor.c_str());

        std::string sMinor = SplitToken(sVersion, ".");
        if (sMinor.empty())
        {
                mMinor = atoi( sVersion.c_str() );
                return;
        }
        mMinor = atoi(sMinor.c_str());

        std::string sBuild = SplitToken(sVersion, ".");
        if (sBuild.empty())
        {
                mBuild = atoi( sVersion.c_str() );
                return;
        }
        mBuild = atoi(sBuild.c_str());

        mRev = atoi(sVersion.c_str());
}

VersionInfo::VersionInfo( const QString& str )
{
        char* aStr = new char[str.size() + 1]();
        memcpy(aStr, str.toLatin1(), str.size());
        *this = VersionInfo(aStr);

        delete[] aStr;
}

bool VersionInfo::operator==(const VersionInfo &rhs) const
{
        if ( this->mMajor != rhs.GetMajor() ) { return false; }
        if ( this->mMinor != rhs.GetMinor() ) { return false; }
        if ( this->mBuild != rhs.GetBuild() ) { return false; }
        if ( this->mRev   != rhs.GetRev()   ) { return false; }

        return true;
}

bool VersionInfo::operator!=(const VersionInfo &rhs) const
{
    return !operator==(rhs);
}

bool VersionInfo::operator>=(const VersionInfo &rhs) const
{
    return (operator>(rhs) || operator==(rhs));
}

bool VersionInfo::operator>(const VersionInfo &rhs) const
{
        if( mMajor > rhs.GetMajor() )
                return true;
        if( mMajor < rhs.GetMajor() )
                return false;

        if( mMinor > rhs.GetMinor() )
                return true;
        if( mMinor < rhs.GetMinor() )
                return false;

        if( mBuild > rhs.GetBuild() )
                return true;
        if( mBuild < rhs.GetBuild() )
                return false;

        if( mRev > rhs.GetRev() )
                return true;

        return false; // either < or ==
}

bool VersionInfo::operator<(const VersionInfo &rhs) const
{
        if( mMajor < rhs.GetMajor() )
                return true;
        if( mMajor > rhs.GetMajor() )
                return false;

        if( mMinor < rhs.GetMinor() )
                return true;
        if( mMinor > rhs.GetMinor() )
                return false;

        if( mBuild < rhs.GetBuild() )
                return true;
        if( mBuild > rhs.GetBuild() )
                return false;

        if( mRev < rhs.GetRev() )
                return true;

        return false; // either > or ==
}

VersionInfo& VersionInfo::operator=(const VersionInfo &rhs)
{
        this->mMajor = rhs.GetMajor();
        this->mMinor = rhs.GetMinor();
        this->mBuild = rhs.GetBuild();
        this->mRev   = rhs.GetRev();

        return *this;
}

QString VersionInfo::ToStr(VersionInfo::VersionFormat format) const
{
        QString sRtn;
        sRtn.sprintf( "%d.%d.%d", GetMajor(), GetMinor(), GetBuild() );
        if (format == FULL_VERSION)
            sRtn.append(QString(".%1").arg(GetRev()));
        return sRtn;
}

} // namespace Origin

