/*************************************************************************************************/
/*!
    \file   statstypes.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "statstypes.h"
#include "statsconfig.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{
namespace Stats
{

char8_t* StatTypeDb::getDbTypeString(char8_t* buf, size_t len) const
{
    char8_t* val = buf;

    if (len > 0)
        buf[0] = '\0';

    switch (mType)
    {
        case STAT_TYPE_INT:
            if (mSize == 1)
                blaze_snzprintf(buf, len, "tinyint(4)");
            else if (mSize == 2)
                blaze_snzprintf(buf, len, "smallint(6)");
            else if (mSize == 3)
                blaze_snzprintf(buf, len, "mediumint(8)");
            else if (mSize == 4)
                blaze_snzprintf(buf, len, "int(11)");
            else if(mSize == 8)
                blaze_snzprintf(buf, len, "bigint(20)");
            else
                val = nullptr;
            break;

        case STAT_TYPE_FLOAT:
            blaze_snzprintf(buf, len, "float");
            break;

        case STAT_TYPE_STRING:
            blaze_snzprintf(buf, len, "varchar(%d)", mSize);
            break;
    }
    return val;
}

bool StatTypeDb::isValidSize() const
{
    switch (mType)
    {
        case STAT_TYPE_INT:
        {
            // Only support int8_t to int64_t
            if (mSize == 8)
                return true;
            else
                return ((mSize >= 1) && (mSize <= 4));
        }

        case STAT_TYPE_FLOAT:
            // Size isn't currently used for floats
            return true;

        case STAT_TYPE_STRING:
            return ((mSize >= 1) && (mSize <= 65535));
    }
    return false;
}

// static
StatTypeDb StatTypeDb::getTypeFromDbString(const char8_t* type, uint32_t length)
{
    if (blaze_stristr(type, "bigint") != nullptr)
    {
        return StatTypeDb(STAT_TYPE_INT, 8);
    }
    else if (blaze_stristr(type, "tinyint") != nullptr)
    {
        return StatTypeDb(STAT_TYPE_INT, 1);
    }
    else if (blaze_stristr(type, "smallint") != nullptr)
    {
        return StatTypeDb(STAT_TYPE_INT, 2);
    }
    else if (blaze_stristr(type, "mediumint") != nullptr)
    {
        return StatTypeDb(STAT_TYPE_INT, 3);
    }
    else if (blaze_stristr(type, "int") != nullptr)
    {
        return StatTypeDb(STAT_TYPE_INT, 4);
    }
    else if (blaze_stristr(type, "float") != nullptr)
    {
        return StatTypeDb(STAT_TYPE_FLOAT, sizeof(float));
    }
    else if (blaze_stristr(type, "varchar") != nullptr)
    {
        return StatTypeDb(STAT_TYPE_STRING, length);
    }

    // Default to 32-bit int type
    return StatTypeDb(STAT_TYPE_INT, 4);
}

/*************************************************************************************************/
/*!
    \brief validateFormat

    Validate a format string.  This function is used to validate that a printf style
    formatting string specified in a config file is legal so that when it is used to print data
    into a string buffer it does not cause crashes.  This function will not necessarily allow
    every single possible legal string to be validated, but should cover any conceivably
    useful string as far as Blaze stats is concerned.
    
    \param[in]  statType - the defined type of stats
    \param[in]  format - the format string to validate
    \param[in]  typeSize - the defined size of type.
    \param[out] rFormat - the new generated format string will be returned.
    \param[in]  rFormatSize - the size of new returned format string.

    \return - true if valid, false otherwise
*/
/*************************************************************************************************/
bool validateFormat(StatType statType, const char8_t* format, int32_t typeSize, char8_t* rFormat , size_t rFormatSize)
{
    const char8_t* ch = format;

    if (ch == nullptr)
        return false;

    if ( nullptr == rFormat)
        return false;

    // Easy first check, must start with %
    if (*ch++ != '%')
        return false;

    // Next check for the five allowable flag chars, whether C++ allows repetitions or not,
    // we won't allow them.  There is no need for repetition, and we want to enforce good
    // clean config files.
    int32_t minus = 0;
    int32_t plus = 0;
    int32_t space = 0;
    int32_t zero = 0;
    int32_t pound = 0;
    bool doneFlag = false;
    while (*ch != '\0' && !doneFlag)
    {
        switch (*ch)
        {
        case '-': ++minus; break;
        case '+': ++plus; break;
        case ' ': ++space; break;
        case '0': ++zero; break;
        case '#': ++pound; break;
        default:
            doneFlag = true;
            if (minus > 1 || plus > 1 || space > 1 || zero > 1 || pound > 1)
                return false;
            continue;
        }
        ++ch;
    }

    // Parse a field width (if any) and limit it to 255
    if (*ch >= '0' && *ch <= '9')
    {
        char8_t* end = nullptr;
        if (strtol(ch, &end, 10) > 255)
            return false;
        ch = end;
    }

    // Parse a separator (if any)
    if (*ch == '.')
        ++ch;

    // Parse a precision (if any) and limit it to 255
    if (*ch >= '0' && *ch <= '9')
    {
        char8_t* end = nullptr;
        if (strtol(ch, &end, 10) > 255)
            return false;
        ch = end;
    }

    // While C++ allows for optional length types, some are platform-dependent thus they must not be 
    // specified in the config file and will instead be inserted here as needed

    // Parse the main type specifier
    switch (statType)
    {
        case STAT_TYPE_INT:
        {
            if (*ch != 'd' && *ch != 'i' && *ch != 'o' && *ch != 'x' &&
                *ch != 'X' && *ch != 'u' && *ch != 'c')
                return false;
            break;
        }
        case STAT_TYPE_FLOAT:
        {
            if (*ch != 'f' && *ch != 'e' && *ch != 'E' && *ch != 'g' && *ch != 'G')
                return false;
            break;
        }
        case STAT_TYPE_STRING:
        {
            if (*ch != 's') 
                return false;
            break;
        }
    }

    ch++;
    if (*ch != '\0') 
        return false;

    memset(rFormat, 0, rFormatSize);

    //generate a correct format string.
    if ( (typeSize == 8) && (statType == STAT_TYPE_INT))
    {
        // from '%d' clip off the 'd' here
        blaze_strnzcpy(rFormat, format, eastl::min(strlen(format), rFormatSize));
        // tack on the format specifier
        switch (*(--ch))
        {
            case 'd': blaze_strnzcat(rFormat, PRId64, rFormatSize); break;
            case 'i': blaze_strnzcat(rFormat, PRIi64, rFormatSize); break;
            case 'o': blaze_strnzcat(rFormat, PRIo64, rFormatSize); break;
            case 'u': blaze_strnzcat(rFormat, PRIu64, rFormatSize); break;
            case 'x': blaze_strnzcat(rFormat, PRIx64, rFormatSize); break;
            case 'X': blaze_strnzcat(rFormat, PRIX64, rFormatSize); break;
            default : return false; // Note: 'c' invalid for type size 8
        };
    }
    else
    {
        blaze_strnzcpy(rFormat, format, rFormatSize);
    }

    // If we get here, it is ok
    return true;

}

/*************************************************************************************************/
/*!
    \brief pushDependency

    In order to keep the core expression lexer/parser classes generic, we extend the
    DependencyContainer to store some state and provide the implementation of the pushDependency
    method.  This allows us to remember the category whose stats we are currently parsing
    derived formulae for - as many derived formulae will reference stats within their own
    category and thus not include any category name in the formula itself.

    \param[in]  context - unused here, only provided to match the interface
    \param[in]  nameSpaceName - the namespace (category) of the dependency being pushed
    \param[in]  name - the name of the stat being pushed
*/
/*************************************************************************************************/
void StatDependencyContainer::pushDependency(void* context, const char8_t* nameSpaceName, const char8_t* name)
{
    // If a stat in the derived formula has no namespace (category) it is implied that it is
    // from the same category as the derived stat itself
    if (nameSpaceName == nullptr)
        nameSpaceName = mCategoryName;

    // If the config file is bad, the category might not exist, this will be handled by the
    // expression parser creating an UnknownIdentifierExpression, so we just skip over this
    // if we hit an unknown category name
    CategoryMap::const_iterator iter = mCategoryMap->find(nameSpaceName);
    if (iter != mCategoryMap->end())
    {
        StatMap::const_iterator statIter = iter->second->getStatsMap()->find(name);
        if (statIter != iter->second->getStatsMap()->end())
        {
            (*mDependencyMap)[nameSpaceName].insert(statIter->second);
        }
    }
}


/*************************************************************************************************/
/*!
    \brief init

    Initialize a stat.
    
    \param[in] category - parent category of the stat
    \param[in] type - data type used to store the stat in the DB
    \param[in] format - stat format string
    \param[in] defaultIntVal - default value for this stat only if it is an int
    \param[in] defaultFloatVal - default value for this stat only if it is a float
    \param[in] defaultStringVal - default value for this stat only if it is a string
*/
/*************************************************************************************************/
void Stat::init(const StatCategory* category, const StatTypeDb& type, const char8_t* format,
                int64_t defaultIntVal, float_t defaultFloatVal, const char8_t* defaultStringVal)
{
    clean(); // clean fields

    mCategory = category;
    blaze_snzprintf(mQualifiedName, sizeof(mQualifiedName), "%s::%s", category->getName(), getName());
    mDefaultIntVal = defaultIntVal;
    mDefaultFloatVal = defaultFloatVal;
    blaze_strnzcpy(mDefaultStringVal, defaultStringVal, sizeof(mDefaultStringVal));
    mTypeDb.set(type.getType(), type.getSize());
    blaze_strnzcpy(mStatFormat, format, sizeof(mStatFormat));
    initDefaultValAsString();
}

void Stat::clean()
{
    if (mDerivedExpression != nullptr)
    {
        delete mDerivedExpression;
        mDerivedExpression = nullptr;
    }
    mCategory = nullptr;
    mDependencies.clear();
    mQualifiedName[0] = '\0';
    mDefaultIntVal = 0;
    mDefaultFloatVal = 0;
    mDefaultStringVal[0] = '\0';
    mTypeDb = StatTypeDb();
    mStatFormat[0] = '\0';
    mDefaultValueAsFormattedString[0] = '\0';
}

/*************************************************************************************************/
/*!
    \brief ~Stat

    Destroy and cleanup the memory owned by a stat.
*/
/*************************************************************************************************/
Stat::~Stat()
{
    clean();
}

/*************************************************************************************************/
/*!
    \brief initDefaultValAsString

    Prints the default value of a stat into a string buffer.
*/
/*************************************************************************************************/
void Stat::initDefaultValAsString()
{
    char8_t* buf = mDefaultValueAsFormattedString;
    const size_t len = sizeof(mDefaultValueAsFormattedString);

    switch (mTypeDb.getType())
    {
        case STAT_TYPE_INT:
        {
            if (mTypeDb.getSize() == 8)
                blaze_snzprintf(buf, len, mFormat, mDefaultIntVal);
            else
                blaze_snzprintf(buf, len, mFormat, static_cast<int32_t>(mDefaultIntVal));
            break;
        }
        case STAT_TYPE_FLOAT:
            blaze_snzprintf(buf, len, mFormat, mDefaultFloatVal);
            break;
        case STAT_TYPE_STRING:
            blaze_snzprintf(buf, len, mFormat, mDefaultStringVal);
            break;
        default:
            buf[0] = 0;
            break;
    }
}

/*************************************************************************************************/
/*!
\brief extractDefaultRawVal

Extract the default value of a stat into a raw stat value.

\param[out] statRawVal - the raw stat to print into
*/
/*************************************************************************************************/
void Stat::extractDefaultRawVal(StatRawValue& statRawVal) const
{
    switch (mTypeDb.getType())
    {
    case STAT_TYPE_INT:
        statRawVal.setIntValue(mDefaultIntVal);
        break;
    case STAT_TYPE_FLOAT:
        statRawVal.setFloatValue(mDefaultFloatVal);
        break;
    case STAT_TYPE_STRING:
        statRawVal.setStringValue(mDefaultStringVal);
        break;
    default:
        break;
    }
}

bool Stat::isValidIntVal(int64_t val) const
{
    // For 64-bit DB ints, any value is valid
    if (mTypeDb.getSize() == 8)
        return true;

    // For all other types, bit-shift down which will sign-extend, so valid positive values become 0,
    // and valid negative values become -1
    val >>= (8 * mTypeDb.getSize() - 1);
    return val == 0 || val == -1;
}

/*************************************************************************************************/
/*!
    \brief init

    Initialize a StatDesc.
    
    \param[in] stat - stat that this description describes
    \param[in] group - parent stat group of this stat
    \param[in] scopeMap - optional scope name value pairs associated with this stat desc
    \param[in] periodType - optional period type value associated with this stat desc
    \param[in] format - a printf style formatting string used to convert stat value to string
    \param[in] index - index value describing the order of stat descs within the group
*/
/*************************************************************************************************/
void StatDesc::init(const Stat* stat, const StatGroup* group, const ScopeNameValueMap* scopeMap,
    int32_t periodType, const char8_t* format, int32_t index)
{
    clean(); // clean fields

    mStat = stat;
    mGroup = group;
    mScopeMap = scopeMap;
    mStatPeriodType = periodType;
    blaze_strnzcpy(mStatFormat, format, sizeof(mStatFormat));
    mIndex = index;
}

void StatDesc::clean()
{
    mScopeMap = nullptr;
    mStat = nullptr;
    mGroup = nullptr;
    mStatPeriodType = 0;
    mStatFormat[0] = '\0';
    mIndex = 0;
}

/*************************************************************************************************/
/*!
    \brief ~StatDesc

    Destroy and cleanup the memory owned by a StatDesc.
*/
/*************************************************************************************************/
StatDesc::~StatDesc()
{
    clean();
}

// ------------------------------ Leaderboards -----------------------------

/*************************************************************************************************/
/*!
    \brief init

    Initialize a GroupStat.
    
    \param[in] stat - stat that this description describes
    \param[in] group - parent stat group of this stat
    \param[in] scopeMap - optional scope name value pairs associated with this stat desc
    \param[in] periodType - optional period type value associated with this stat desc
    \param[in] format - a printf style formatting string used to convert stat value to string
    \param[in] index - index value describing the order of stat descs within the group
*/
/*************************************************************************************************/
void GroupStat::init(const Stat* stat, const LeaderboardGroup* group, const EA::TDF::tdf_ptr<ScopeNameValueListMap>& scopeMap,
    int32_t periodType, const char8_t* format, int32_t index)
{
    clean(); // clean fields

    mStat = stat;
    mGroup = group;
    mScopeMap = scopeMap;
    mStatPeriodType = periodType;
    blaze_strnzcpy(mStatFormat, format, sizeof(mStatFormat));
    mIndex = index;
}

void GroupStat::clean()
{
    mScopeMap = nullptr;
    mStat = nullptr;
    mGroup = nullptr;
    mStatPeriodType = 0;
    mStatFormat[0] = '\0';
    mIndex = 0;
}

/*************************************************************************************************/
/*!
    \brief ~GroupStat

    Destroy a GroupStat object and all memory it owns.
*/
/*************************************************************************************************/
GroupStat::~GroupStat()
{
    clean();
}

/*************************************************************************************************/
/*!
    \brief init

    Initialize a StatLeaderboardData.
    
    \param[in] scopeNameValueListMap - optional scope name value pairs associated with this Leaderboard
    \param[in] periodType - optional period type value associated with this Leaderboard
    \param[in] size - size of ranking table
    \param[in] extra - The extra data for leaderboard
    \param[in] level - index value describing the order of Leaderboard within the list
*/
/*************************************************************************************************/
void StatLeaderboardData::init(ScopeNameValueListMap* scopeNameValueListMap,
        int32_t periodType, int32_t size, int32_t extra, int32_t level)
{
    clean(); // clean fields

    setScopeNameValueListMap(scopeNameValueListMap);
    setPeriodType(periodType);
    setSize(size);
    setExtra(extra);
    setLevel(level);
}

void StatLeaderboardData::clean()
{
    mScopeNameValueListMap = nullptr;
    mChildCount = 0;
    mFirstChild = 0;
    mExtra = 0;
    mSize = 0;
    mPeriodType = 0;
    mParent = 0;
    mLevel = 0;
}

/*************************************************************************************************/
/*!
    \brief ~StatLeaderboardData
    Destructor
    \param[in] none
*/
/*************************************************************************************************/
StatLeaderboardData::~StatLeaderboardData()
{
    clean();
}

} // Stats
} // Blaze
