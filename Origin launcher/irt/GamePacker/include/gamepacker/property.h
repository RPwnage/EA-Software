/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GAME_PACKER_PROPERTY_H
#define GAME_PACKER_PROPERTY_H

#include <EASTL/intrusive_ptr.h>
#include <EASTL/hash_map.h>
#include "gamepacker/common.h"

namespace Packer
{

typedef eastl::vector<PropertyValue> PropertyValueList;
typedef eastl::vector<uint16_t> PropertyValueIndex;

struct PropertyDescriptor;

/**
 * This is a player property field that tracks a named column of values that is comparable across different players (sub-field under a descriptor)
 */
struct FieldDescriptor
{
    eastl::string mFieldName;
    PropertyFieldId mFieldId = 0;
    PropertyDescriptor* mPropertyDescriptor = nullptr;
    mutable int32_t mRefCount = 0;
    FieldDescriptor();
    ~FieldDescriptor();
    eastl::string getFullFieldName() const;
};

/**
 * This is a player property descriptor, it aggregates multiple fields that must be uniquely named under the descriptor
 */
struct PropertyDescriptor
{
    eastl::string mPropertyName; // This is a 'leaf/suffix' property name pertaining to the *players*, for example if a full 'derived' property name is game.teams.players.uedValues then the descriptor name is uedValues 
    PropertyDescriptorIndex mDescriptorIndex = 0;
    eastl::hash_map<eastl::string, FieldDescriptor, caseless_hash, caseless_eq> mFieldDescriptorByName;
    eastl::hash_set<struct GameQualityFactor*> mReferencingFactors;
    struct PackerSilo* mPacker = nullptr;
    PropertyDescriptor();
    eastl::string getFullFieldName(const eastl::string& fieldName) const;
};

void intrusive_ptr_add_ref(const FieldDescriptor* ptr);
void intrusive_ptr_release(const FieldDescriptor* ptr);

typedef eastl::intrusive_ptr<const FieldDescriptor> FieldDescriptorRef;


typedef eastl::vector<PropertyDescriptor> PropertyDescriptorList; // (Indexed by PropertyDescriptorIndex)
typedef eastl::vector<const FieldDescriptor*> FieldDescriptorList;

struct FieldValueColumn {
    static const uint32_t MAX_FIELD_COLUMN_CAPACITY = MAX_PLAYER_COUNT * 2; // max capacity is 2 times max number of players in a game to ensure that we can provisionally over-allocate a single party
    typedef eastl::bitset<MAX_FIELD_COLUMN_CAPACITY> FieldBits;

    PropertyValueList mPropertyValues;
    PropertyValueIndex mPropertyValueIndex; // indices of elements(player/party) sorted by property value (only set values are indexed)
    FieldBits mSetValueBits; // tracks which property values are 'set' in the column
    uint32_t mAddTableRefCount = 0; // number of times this column was added as part of a table addition (includes uncommitted additions)
    uint32_t mCommittedAddTableRefCount = 0; // number of times this column was added as part of a committed table addition
    FieldDescriptorRef mFieldRef;

    Score getValueParticipationScore() const;
};

typedef eastl::vector_map<PropertyFieldId, FieldValueColumn*> FieldValueColumnMap;


/**
 * This is a column-oriented table, for more efficient addition/removal and indexing operations.
 * The table is 'shrinkable' to support the use case whereby we append a set of fields, 
 * then efficiently revert the changes in case the change is not desired.
 */
struct FieldValueTable
{
    // PACKER_TODO: #OPTIMIZATION FieldValue table should be able to group fieldValueColumnMaps by the property 
    // that the fields belong to. This would allow us to quickly eliminate all fields that the property 
    // does not pertain to when evaluating. 
    // The mapping could be a simple array because # of properties are immutable in the silo.
    FieldValueColumnMap mFieldValueColumnMap;

    uint32_t mTableSize = 0; // number of rows in a table
    uint32_t mCommittedTableSize = 0; //  number of committed rows in a table

    enum TableType {
        ALLOW_SET_FIELD,
        ALLOW_ADD_REMOVE
    } mTableType = ALLOW_SET_FIELD;

    ~FieldValueTable();

    bool hasCommonFields(const FieldValueTable& otherTable) const;

    void setFieldValue(const FieldDescriptor* field, uint32_t valueIndex, const PropertyValue* value);

    PropertyValue getFieldValue(const FieldDescriptor* field, uint32_t valueIndex, bool* hasValue = nullptr) const;

    bool hasField(const FieldDescriptor* field) const;

    void addTable(const FieldValueTable& otherTable);

    void removeTable(const FieldValueTable& otherTable, uint32_t offset);

    void commitTable(const FieldValueTable& otherTable);

    void trimToCommitted();

    void rebuildIndexes();
};



}

#endif