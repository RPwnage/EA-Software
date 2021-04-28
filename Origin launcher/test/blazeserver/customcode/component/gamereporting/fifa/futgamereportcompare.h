/*************************************************************************************************/
/*!
    \file   futgamereportcompare.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_FUT_REPORT_COMPARE_H
#define BLAZE_GAMEREPORTING_FUT_REPORT_COMPARE_H

#include "gamereporting/gamereportcompare.h"
#include "fifa/tdf/futreport_server.h"

namespace Blaze
{
namespace GameReporting
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Utility class to compare 2 reports based on the supplied config. 
//  return false if the reports differs or if any one of the reports not matching the config structure

class FUTGameReportCompare: public GameReportCompare
{
public:
    FUTGameReportCompare() 
		: mMaxCollisionListSize(20)
	{}

    virtual ~FUTGameReportCompare() {}

	// Use the configuration the comparison
	void setConfig(const FUT::CollatorConfig& collatorConfig);
	
	//  returns false if the two GameReports differ.
    bool compare(const GameType& gameType, EA::TDF::Tdf& report, const EA::TDF::Tdf& reportReference);

	// Get the list of comparison failures.
	FUT::CollisionList& getCollisionList() { return mCollisionList; }

protected:
    void compareReports(const GameTypeReportConfig& gameTypeReportConfig, const GRECacheReport& greCache, EA::TDF::Tdf& report, const EA::TDF::Tdf& reportReference);

    //  visit top-level game type of the config subreport
    virtual bool visitSubreport(const GameTypeReportConfig& subReportConfig, const GRECacheReport& greSubReport, const GameReportContext& context, EA::TDF::Tdf& tdfOut, bool& traverseChildReports);
    //  visit the map of events for the subreport
    virtual bool visitEventUpdates(const GameTypeReportConfig::EventUpdatesMap& events, const GRECacheReport::EventUpdates& greEvents, const GameReportContext& context, EA::TDF::Tdf& tdfOut) { return true; }
    //  visit the map of achievements for the subreport
    virtual bool visitAchievementUpdates(const GameTypeReportConfig::AchievementUpdates& achievements, const GRECacheReport::AchievementUpdates& greAchievements, const GameReportContext& context, EA::TDF::Tdf& tdfOut) { return true; }

protected:
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue);
    virtual bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue);
    virtual bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue);

	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue = false);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue = 0);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue = 0);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue = 0);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue = 0);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue = 0);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue = 0);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue = 0);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue = 0);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue = 0.0f);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue = 0);

	virtual bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue = "", const uint32_t maxLength = 0);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue);
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue);
	
	virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue = 0);

private:
	void recordMismatch(const char8_t* a, const char8_t* b, EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag);

private:
	FUT::ComparisonConfigMap mComparisonConfig;
	FUT::CollisionList mCollisionList;
	size_t mMaxCollisionListSize;

private:
	template <class T>
	auto toString(T const& t) -> decltype(eastl::to_string(t))
	{
		return eastl::to_string(t);
	}

	template <class T>
	auto toString(T const& t) -> decltype(t.toString())
	{
		return t.toString();
	}

	auto toString(TimeValue const& t) -> eastl::string
	{		
		char8_t buf[16];
		t.toString(buf, sizeof(buf));

		eastl::string value(buf);

		return value;
	}

	template <typename A, typename B>
	bool compareFromString(A& a, B& b, const EA::TDF::TdfString& value, const char8_t* key)
	{
		ERR_LOG("[FUTGameReportCompare:" << this << "].compareFromString(): Unknown type to convert from String value for key =" << key);
		return a != b;
	}

	template <typename A, typename B> 
	inline bool compareValue(A& a, B& b, EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag)
	{
		bool retVal = false;

		// if skipping comparison for this subreport, do not allow traversal into child subreports of this subreport node.
		if (mStateStack.top().config->getSkipComparison())
		{
			retVal = false;
		}
		else
		{
			const EA::TDF::TdfMemberInfo* memberInfo = NULL;
			parentTdf.getMemberInfoByTag(tag, memberInfo);

			eastl::fixed_string<char8_t, FUT::MAX_COMPARISONCONFIGMAP_KEY_LEN> key;
			key.sprintf("%s.%s", parentTdf.getClassName(), memberInfo->getMemberName());

			auto config = mComparisonConfig.find(key.c_str());
			if( config != mComparisonConfig.end() )
			{
				// Override the exact comparison with a different comparison
				retVal = compareFromString(a, b, config->second, key.c_str());
			}
			else
			{
				// Exact binary comparison
				retVal = (a != b);
			}
		}

		// Record the difference
		if (retVal == true)
		{
			setDifferenceFoundFlag(true);

			auto aString = toString(a);
			auto bString = toString(b);

			recordMismatch(aString.c_str(), bString.c_str(), rootTdf, parentTdf, tag);
		}

		return retVal;
	}
};

template<>
bool FUTGameReportCompare::compareFromString<uint32_t, const uint32_t>(uint32_t& a, const uint32_t& b, const EA::TDF::TdfString& value, const char8_t* key)
{
	// Allow the integer to be different within the defined range
	uint32_t diff = a > b ? (a - b) : (b - a);

	return (diff > EA::StdC::AtoU32(value.c_str()) );
}

template<>
bool FUTGameReportCompare::compareFromString<int32_t, const int32_t>(int32_t& a, const int32_t& b, const EA::TDF::TdfString& value, const char8_t* key)
{
	// Allow the integer to be different within the defined range
	int32_t diff = a > b ? (a - b) : (b - a);

	return (diff > EA::StdC::AtoI32(value.c_str()));
}


template<>
bool FUTGameReportCompare::compareFromString<uint16_t, const uint16_t>(uint16_t& a, const uint16_t& b, const EA::TDF::TdfString& value, const char8_t* key)
{
	// Allow the integeter to be different within the defined range
	uint16_t diff = a > b ? (a - b) : (b - a);
	return (diff > static_cast<uint16_t>(EA::StdC::AtoU32(value.c_str())));
}

}; //namespace GameReporting
}; //namespace Blaze

#endif // BLAZE_GAMEREPORTING_FUT_REPORT_COMPARE_H
