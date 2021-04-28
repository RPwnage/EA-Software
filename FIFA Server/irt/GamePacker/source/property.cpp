#include <EASTL/sort.h>
#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/property.h"
#include "gamepacker/packer.h"

namespace Packer
{

PropertyDescriptor::PropertyDescriptor()
{
    mDescriptorIndex = INVALID_PROPERTY_DESCRIPTOR_INDEX;
}

eastl::string PropertyDescriptor::getFullFieldName(const eastl::string& fieldName) const
{
    return eastl::string(eastl::string::CtorSprintf(), PACKER_PROPERTY_FIELD_TOKEN, mPropertyName.c_str(), fieldName.c_str());
}

void intrusive_ptr_add_ref(const FieldDescriptor * ptr)
{
    ptr->mRefCount++;
}

void intrusive_ptr_release(const FieldDescriptor * ptr)
{
    EA_ASSERT(ptr->mRefCount > 0);
    ptr->mRefCount--;
    if (ptr->mRefCount == 0)
        ptr->mPropertyDescriptor->mPacker->removePropertyField(ptr->mFieldId);
}

FieldDescriptor::FieldDescriptor()
{
}

FieldDescriptor::~FieldDescriptor()
{
    EA_ASSERT(mRefCount == 0);
}

eastl::string FieldDescriptor::getFullFieldName() const
{
    return mPropertyDescriptor->getFullFieldName(mFieldName);
}

FieldValueTable::~FieldValueTable()
{
    for (auto& columnItr : mFieldValueColumnMap)
    {
        delete columnItr.second;
    }
}

bool FieldValueTable::hasCommonFields(const FieldValueTable & otherTable) const
{
    if (mCommittedTableSize == 0)
    {
        return otherTable.mFieldValueColumnMap.size() > 0;
    }

    auto end = mFieldValueColumnMap.end();
    for (auto& otherBlockItr : otherTable.mFieldValueColumnMap)
    {
        auto columnItr = mFieldValueColumnMap.find(otherBlockItr.first);
        if ((columnItr != end) && (columnItr->second->mCommittedAddTableRefCount > 0))
            return true;
    }
    
    return false;
}

void FieldValueTable::setFieldValue(const FieldDescriptor * field, uint32_t valueIndex, const PropertyValue * value)
{
    EA_ASSERT(mTableType == ALLOW_SET_FIELD);
    EA_ASSERT(valueIndex < FieldValueColumn::MAX_FIELD_COLUMN_CAPACITY);
    auto newSize = valueIndex + 1;
    if (newSize < mTableSize)
        newSize = mTableSize;
    else if (newSize > mTableSize)
    {
        mTableSize = newSize;
        for (auto& columnItr : mFieldValueColumnMap)
        {
            columnItr.second->mPropertyValues.resize(newSize, 0);
        }
    }
    auto ret = mFieldValueColumnMap.insert(field->mFieldId);
    if (ret.second)
    {
        ret.first->second = new FieldValueColumn;
        ret.first->second->mPropertyValues.resize(newSize, 0);
        ret.first->second->mFieldRef = field;
    }

    bool isSet = value != nullptr;
    if (isSet)
    {
        ret.first->second->mPropertyValues[valueIndex] = *value;
    }
    ret.first->second->mSetValueBits.set(valueIndex, isSet);
}

PropertyValue FieldValueTable::getFieldValue(const FieldDescriptor * field, uint32_t valueIndex, bool * hasValue) const
{
    EA_ASSERT(valueIndex < FieldValueColumn::MAX_FIELD_COLUMN_CAPACITY);
    auto columnItr = mFieldValueColumnMap.find(field->mFieldId);
    EA_ASSERT(columnItr != mFieldValueColumnMap.end());
    bool isSet = columnItr->second->mSetValueBits.test(valueIndex);

    if (hasValue)
        *hasValue = isSet;

    if (isSet)
        return columnItr->second->mPropertyValues[valueIndex];

    return 0;
}

bool FieldValueTable::hasField(const FieldDescriptor * field) const
{
    auto columnItr = mFieldValueColumnMap.find(field->mFieldId);
    return (columnItr != mFieldValueColumnMap.end());
}

void FieldValueTable::addTable(const FieldValueTable & otherTable)
{
    EA_ASSERT(mTableType == ALLOW_ADD_REMOVE);
    auto originalSize = mCommittedTableSize;
    auto newSize = mCommittedTableSize + otherTable.mTableSize;
    for (auto& otherBlockItr : otherTable.mFieldValueColumnMap)
    {
        auto ret = mFieldValueColumnMap.insert(otherBlockItr.first);
        auto& column = *(ret.second ? new FieldValueColumn : ret.first->second);
        column.mPropertyValues.reserve(newSize);
        if (ret.second)
        {
            ret.first->second = &column;
            column.mFieldRef = otherBlockItr.second->mFieldRef;
            // inserted new column, need to make it the size of the existing table
            column.mPropertyValues.resize(originalSize, 0);
        }

        column.mAddTableRefCount++;
        // append values to column starting from *last committed* size
        column.mPropertyValues.insert(column.mPropertyValues.begin() + originalSize, 
            otherBlockItr.second->mPropertyValues.begin(), otherBlockItr.second->mPropertyValues.end());
        auto tempBits = otherBlockItr.second->mSetValueBits;
        tempBits <<= originalSize; // shift incoming bits into position to be merged with new column
        column.mSetValueBits |= tempBits;
    }
    mTableSize = newSize;
}

void FieldValueTable::removeTable(const FieldValueTable & otherTable, uint32_t offset)
{
    EA_ASSERT(mTableType == ALLOW_ADD_REMOVE);
    EA_ASSERT(offset + otherTable.mTableSize <= mTableSize);
    auto newSize = mTableSize - otherTable.mTableSize;
    for (auto& otherBlockItr : otherTable.mFieldValueColumnMap)
    {
        auto columnItr = mFieldValueColumnMap.find(otherBlockItr.first);
        EA_ASSERT(columnItr != mFieldValueColumnMap.end());
        auto& column = *columnItr->second;
        if (column.mAddTableRefCount > 1)
        {
            EA_ASSERT(column.mCommittedAddTableRefCount > 0);
            column.mAddTableRefCount--;
            column.mCommittedAddTableRefCount--;

            // erase values from column
            auto from = column.mPropertyValues.begin() + offset;
            auto to = from + otherTable.mTableSize;
            column.mPropertyValues.erase(from, to);
            column.mPropertyValueIndex.clear(); // removing a table always invalidates the index because it cannot remain valid

            auto tempBits = column.mSetValueBits;
            auto clearLowBitsCount = offset + otherTable.mTableSize;
            column.mSetValueBits >>= clearLowBitsCount; // clear the lower part of the bit range keeping the high bits
            column.mSetValueBits <<= offset; // restore the high bits back to their new position (minus otherTable.mTableSize)

            auto clearHighBitsCount = FieldValueColumn::FieldBits::kSize - offset;
            tempBits <<= clearHighBitsCount; // clear the high part of the bit range, keeping the low bits
            tempBits >>= clearHighBitsCount; // restore the low bits back to their original position (high bits are all 0)
            column.mSetValueBits |= tempBits; // merge the low and high bits
        }
        else
        {
            EA_ASSERT(column.mAddTableRefCount == 0);
            EA_ASSERT(column.mCommittedAddTableRefCount == 0);
            // In practice we can't guarantee that no values would remain set since we don't clear values beyond mCommittedSize
            EA_ASSERT(column.mSetValueBits.none()); // no values should be set at this point...
            delete columnItr->second;
            mFieldValueColumnMap.erase(columnItr);
        }
    }
    mTableSize = newSize;
}

void FieldValueTable::commitTable(const FieldValueTable & otherTable)
{
    EA_ASSERT(mTableType == ALLOW_ADD_REMOVE);
    for (auto& otherBlockItr : otherTable.mFieldValueColumnMap)
    {
        auto columnItr = mFieldValueColumnMap.find(otherBlockItr.first);
        EA_ASSERT(columnItr != mFieldValueColumnMap.end());
        auto& column = *columnItr->second;
        if (column.mAddTableRefCount == column.mCommittedAddTableRefCount)
        {
            continue;
        }
        column.mCommittedAddTableRefCount = column.mAddTableRefCount;
    }
    mCommittedTableSize = mTableSize;

    trimToCommitted();
}

void FieldValueTable::trimToCommitted()
{
    // PACKER_TODO: #OPTIMIZATION If the vector_map has a lot of entries it's much more efficient to iterate it in reverse order
    // and erase elements with erase_unordered() and then sort the map by keys at the end, as this does at most O(NLogN) comparisons and swaps.
    // Meanwhile what we do here has the potential to compare/swap elements O(N^2) times. The saving grace of this right now is that
    // trim is only currenlty ever used to remove a handful of values at the moment...
    for (auto columnItr = mFieldValueColumnMap.begin(); columnItr != mFieldValueColumnMap.end();)
    {
        if (columnItr->second->mCommittedAddTableRefCount > 0)
        {
            if (columnItr->second->mAddTableRefCount > columnItr->second->mCommittedAddTableRefCount)
            {
                // Roll back previously committed column to previously committed state
                columnItr->second->mAddTableRefCount = columnItr->second->mCommittedAddTableRefCount;
                columnItr->second->mPropertyValues.resize(mCommittedTableSize);
                columnItr->second->mPropertyValueIndex.resize(mCommittedTableSize);
            }

            ++columnItr;
        }
        else
        {
            // Clean up wholly uncommitted column
            delete columnItr->second;
            columnItr = mFieldValueColumnMap.erase(columnItr);
        }
    }
}

void FieldValueTable::rebuildIndexes()
{
    for (auto& columnItr : mFieldValueColumnMap)
    {
        auto& fieldValueColumn = *columnItr.second;
        fieldValueColumn.mPropertyValueIndex.clear();
        auto indexSize = fieldValueColumn.mSetValueBits.count();
        if (indexSize > mCommittedTableSize)
            indexSize = mCommittedTableSize; // clamp to committed table size
        fieldValueColumn.mPropertyValueIndex.reserve(indexSize);
        for (auto i = fieldValueColumn.mSetValueBits.find_first(), size = (size_t)mCommittedTableSize; i < size; i = fieldValueColumn.mSetValueBits.find_next(i))
        {
            fieldValueColumn.mPropertyValueIndex.push_back((uint16_t)i);
        }

        // sort the index column
        eastl::sort(fieldValueColumn.mPropertyValueIndex.begin(),
            fieldValueColumn.mPropertyValueIndex.end(), [&fieldValueColumn](const uint16_t x, const uint16_t y) {
            return fieldValueColumn.mPropertyValues[x] < fieldValueColumn.mPropertyValues[y];
        });
    }
}

Score FieldValueColumn::getValueParticipationScore() const
{
    if (mPropertyValues.empty())
        return 0;
    return (Score)mSetValueBits.count() / (Score)mPropertyValues.size();
}

} // GamePacker
