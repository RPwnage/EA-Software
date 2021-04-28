/*************************************************************************************************/
/*!
    \file   futgamereportcompare.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "gamereporting/fifa/futgamereportcompare.h"

namespace Blaze
{
namespace GameReporting
{

void FUTGameReportCompare::recordMismatch(const char8_t* a, const char8_t* b, EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag)
{
	if (mCollisionList.size() < mMaxCollisionListSize)
	{
		const EA::TDF::TdfMemberInfo* memberInfo = NULL;
		parentTdf.getMemberInfoByTag(tag, memberInfo);

		//TRACE_LOG("[FUTGameReportCompare:" << this << "].compareValue(): diff root:" << rootTdf.getClassName() << " parent:" << parentTdf.getClassName() << " member:" << memberInfo->memberName << " a:" << aString.c_str() << " b:" << bString.c_str() << "");

		using namespace Blaze::GameReporting::FUT;

		CollisionInstance* collision = mCollisionList.allocate_element();

		if (collision != NULL)
		{
			collision->setRoot(rootTdf.getClassName());
			collision->setParent(parentTdf.getClassName());
			collision->setMember(memberInfo->getMemberName());
			collision->setType(memberInfo->getTypeId());
			collision->setValue0(a);
			collision->setValue1(b);

			mCollisionList.push_back(collision);

			TRACE_LOG("[FUTGameReportCompare:" << this << "].compareValue(): collision:" << *collision);
		}
		else
		{
			ERR_LOG("[FUTGameReportCompare:" << this << "].compareValue(): collision is NULL");
		}
	}
}

void FUTGameReportCompare::setConfig(const FUT::CollatorConfig& collatorConfig)
{
	mMaxCollisionListSize = collatorConfig.getMaxCollisionListSize();
	collatorConfig.getComparisonConfig().copyInto(mComparisonConfig);
}

bool FUTGameReportCompare::compare(const GameType& gameType, EA::TDF::Tdf& report, const EA::TDF::Tdf& reportReference)
{
	// TODO Config should update the mCollisionLimit;

	return GameReportCompare::compare(gameType, report, reportReference);
}

//  visit top-level game type of the config subreport
bool FUTGameReportCompare::visitSubreport(const GameTypeReportConfig& subReportConfig, const GRECacheReport& greSubReport, const GameReportContext& context, EA::TDF::Tdf& tdfOut, bool& traverseChildReports)
{   
    if (subReportConfig.getSkipComparison())
    {
        traverseChildReports = false;
        TRACE_LOG("[GameReportCompare:" << this << "].visitSubreport(): skipping comparision of node, tdf '" << tdfOut.getFullClassName() << "' within '" << context.tdf->getFullClassName() << "'");
        return true;        // continue parent node traversal (skipping children re: traverseChildReports)
    }
    TRACE_LOG("[GameReportCompare:" << this << "].visitSubreport(): compared tdf '" << tdfOut.getFullClassName() << "' within '" << context.tdf->getFullClassName() << "'");
    return true;
}

bool FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{   
    if (mStateStack.top().config->getSkipComparison())
    {
        TRACE_LOG("[GameReportCompare:" << this << "].visit(): skipping compare of '" << value.getFullClassName() << "' within '" << rootTdf.getFullClassName() << "'");
        return false;
    }

    TRACE_LOG("[GameReportCompare:" << this << "].visit(): comparing of '" << value.getFullClassName() << "' within '" << rootTdf.getFullClassName() << "'");

    //  thunk down to the parent's visit method which will traverse any subreports within this tdf - this is necessary
    //  to ensure that our current config on the stack matches the child subreport.
    return ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue);
}

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{
	if (mStateStack.top().config->getSkipComparison())
	{
		return;
	}

	if (value.vectorSize() != referenceValue.vectorSize())
	{
		setDifferenceFoundFlag(true);

		auto valueString = toString(value.vectorSize());
		auto referenceValueString = toString(referenceValue.vectorSize());

		recordMismatch(valueString.c_str(), referenceValueString.c_str(), rootTdf, parentTdf, tag);
	}
	
	if (value.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_TDF && value.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE)
	{
		value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
	}
};

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
	if (mStateStack.top().config->getSkipComparison())
	{
		return;
	}
	else if (value.mapSize() != referenceValue.mapSize())
	{
		setDifferenceFoundFlag(true);

		auto valueString = toString(value.mapSize());
		auto referenceValueString = toString(referenceValue.mapSize());

		recordMismatch(valueString.c_str(), referenceValueString.c_str(), rootTdf, parentTdf, tag);

		const EA::TDF::TdfMemberInfo* memberInfo = NULL;
		parentTdf.getMemberInfoByTag(tag, memberInfo);

		if (memberInfo != NULL)
		{
			WARN_LOG("[GameReportCompare:" << this << "].visit(TdfMapBase): skipping comparision of node due to size diff, tdf '" << memberInfo->getMemberName() << "' within '" << rootTdf.getFullClassName() << "'");
		}
	}
	else if (value.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_TDF && value.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE)
	{
		value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
	}
	else
	{
		const EA::TDF::TdfMemberInfo* memberInfo = NULL;
		parentTdf.getMemberInfoByTag(tag, memberInfo);

		if (memberInfo != NULL)
		{
			WARN_LOG("[GameReportCompare:" << this << "].visit(TdfMapBase): skipping comparision of node, tdf '" << memberInfo->getMemberName() << "' within '" << rootTdf.getFullClassName() << "'");
		}
	}
}

bool FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue)
{
	if (mStateStack.top().config->getSkipComparison())
	{
		return false;
	}

	visitReference(rootTdf, parentTdf, tag, value.getValue(), &referenceValue.getValue());

    return true;
}

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
    if (blaze_strcmp(referenceValue.c_str(), value.c_str())!=0)
    {
		setDifferenceFoundFlag(true);

		recordMismatch(value.c_str(), referenceValue.c_str(), rootTdf, parentTdf, tag);
    }
}

bool FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue)
{ 
	if (mStateStack.top().config->getSkipComparison())
	{
		return false;
	}

	value.visit(*this, rootTdf, referenceValue);

    return true;
}

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue)
{
    if (mStateStack.top().config->getSkipComparison())
    {
       return;
    }

    if (value.getCount() != referenceValue.getCount())
    {
		setDifferenceFoundFlag(true);

		auto valueString = toString(value.getCount());
		auto referenceValueString = toString(referenceValue.getCount());

		recordMismatch(valueString.c_str(), referenceValueString.c_str(), rootTdf, parentTdf, tag);
    }
	else if (value.getSize() != referenceValue.getSize())
	{
		setDifferenceFoundFlag(true);

		auto valueString = toString(value.getSize());
		auto referenceValueString = toString(referenceValue.getSize());

		recordMismatch(valueString.c_str(), referenceValueString.c_str(), rootTdf, parentTdf, tag);
	}

    //  NOTE: should deep compare actually dig into blob data?
}

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue)
{
    if (mStateStack.top().config->getSkipComparison())
    {    
       return;
    }
    if (value.getBits()!=referenceValue.getBits())
    {
		setDifferenceFoundFlag(true);

		auto valueString = toString(value.getBits());
		auto referenceValueString = toString(referenceValue.getBits());

		recordMismatch(valueString.c_str(), referenceValueString.c_str(), rootTdf, parentTdf, tag);
    }
}

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag); ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag); ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag); ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);}

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue) 
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue)
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag); ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, enumMap, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue)
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag); ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue)
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag); ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

void FUTGameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue)
{ compareValue(value, referenceValue, rootTdf, parentTdf, tag); ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

} //namespace Blaze
} //namespace GameReporting
