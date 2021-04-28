///////////////////////////////////////////////////////////////////////////////
// VersionInfo.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _VERSIONINFO_H_
#define _VERSIONINFO_H_

#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
/// \brief Contains and converts information about a 4-part version number (major.minor.build.revision).
class ORIGIN_PLUGIN_API VersionInfo
{
public:
    /// \brief The VersionInfo constructor.
    /// \param major The major version.
    /// \param minor The minor version.
    /// \param build The build number.
    /// \param rev The revision number.
    VersionInfo( int major = 0, int minor = 0, int build = 0, int rev = 0 );
    
    /// \brief The VersionInfo constructor.
    /// \param str A string representing a 4-part version number.  Should be in the format "X.X.X.X".
    VersionInfo( const QString& str );
    
    /// \brief The VersionInfo constructor.
    /// \param str A string representing a 4-part version number.  Should be in the format "X.X.X.X".
    VersionInfo( const char* str );

    /// \brief Gets the major version number (i.e. the "1" in "1.2.3.4").
    /// \return The major version number.
    int GetMajor() const { return mMajor; }
    
    /// \brief Gets the minor version number (i.e. the "2" in "1.2.3.4").
    /// \return The minor version number.
    int GetMinor() const { return mMinor; }
    
    /// \brief Gets the build number (i.e. the "3" in "1.2.3.4").
    /// \return The build number.
    int GetBuild() const { return mBuild; }
    
    /// \brief Gets the revision number (i.e. the "4" in "1.2.3.4").
    /// \return The revision number.
    int GetRev() const { return mRev; }
    
    /// \brief Overloaded "==" operator.
    /// \param rhs The VersionInfo object to compare against.
    /// \return True if the two versions are equal.
    bool operator==( const VersionInfo &rhs) const;

    /// \brief Overloaded "!=" operator. Implemented in terms of operator==
    /// \param rhs The VersionInfo object to compare against.
    /// \return True if the two versions are different.
    bool operator!=( const VersionInfo &rhs) const;

    /// \brief Overloaded ">" operator.
    /// \param rhs The VersionInfo object to compare against.
    /// \return True if this version is prior to the given version.
    bool operator>( const VersionInfo &rhs) const;
    
    /// \brief Overloaded ">" operator.
    /// \param rhs The VersionInfo object to compare against.
    /// \return True if this version is prior to the given version.
    bool operator>=( const VersionInfo &rhs) const;

    /// \brief Overloaded "<" operator.
    /// \param rhs The VersionInfo object to compare against.
    /// \return True if this version is prior to the given version.
    bool operator<( const VersionInfo &rhs) const;

    /// \brief Overloaded "=" operator.
    /// \param rhs The VersionInfo object to copy from.
    /// \return The VersionInfo copied to.
    VersionInfo& operator=( const VersionInfo &rhs);

    /// \brief Converts the version number into a string in a given format, the default format being "X.X.X.X".
    /// \param format The format of the returned version string; FULL = "X.X.X.X", ABBREVIATED = "X.X.X".
    /// \return A string representing the version number.
    enum VersionFormat { FULL_VERSION, ABBREVIATED_VERSION };
    QString ToStr(VersionFormat format = FULL_VERSION) const;

private:
    /// \brief The major version number.
    int mMajor;

    /// \brief The minor version number.
    int mMinor;

    /// \brief The build number.
    int mBuild;

    /// \brief The revision number.
    int mRev;
};

} // namespace Origin

#endif
