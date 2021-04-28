/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_DBRESULT_H
#define BLAZE_DBRESULT_H
/*** Include files *******************************************************************************/
#include "framework/database/dbrow.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "framework/system/frameworkresource.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class DbConnInterface;
class DbConnBase;

/*************************************************************************************************/
/*!
    \class DbResultCommon
    \brief An interface to serve as the base class for all db result types used to fetch the results
            of a database operation.
*/
/*************************************************************************************************/
class DbResultCommon : public FrameworkResource
{
	NON_COPYABLE(DbResultCommon);
    friend class DbConnBase;

public:
    const char8_t* getTypeName() const override { return "DbResultCommon"; }
    virtual const char8_t* getColumnName(uint32_t columnIndex) const { return 0; }
    virtual uint32_t getColumnCount() const { return 0; }
    virtual uint32_t getAffectedRowCount() const = 0;
    virtual uint64_t getLastInsertId() const = 0;
    virtual const char8_t* toString(
        eastl::string& buf, int32_t maxStringLen = 0, int32_t maxBinaryLen = 0) const = 0;

protected:
    DbResultCommon() {}
    ~DbResultCommon() override {}

private:
    void assignInternal() override {}
    void releaseInternal() override
    {
        delete this;
    }
};

/*************************************************************************************************/
/*!
    \class DbResultBase
    \brief Contains the result of a database operation.  The entire result set is available in this object
*/
/*************************************************************************************************/
class DbResultBase : public DbResultCommon
{
    NON_COPYABLE(DbResultBase);

public:
    typedef eastl::vector<DbRow*> RowList;
    typedef RowList::const_iterator const_iterator;

    bool empty() const { return getRowList().empty(); }
    size_t size() const { return getRowList().size(); }   

    const_iterator begin() const { return getRowList().begin(); }
    const_iterator end() const { return getRowList().end(); }

protected:
    DbResultBase() {}
    ~DbResultBase() override {}

private:
    virtual const RowList &getRowList() const = 0;
};

//  DbResult interface definition
typedef FrameworkResourcePtr<DbResultBase> DbResultPtr;

class DbResultPtrs : public eastl::vector<DbResultPtr>
{
public:
    typedef eastl::vector<DbResultPtr> base_type;
    virtual ~DbResultPtrs() {}
};


//  deprecated interface - use DbResultPtr
class DbResult : public DbResultBase
{
    NON_COPYABLE(DbResult);

public:
    DbResult(){}
    ~DbResult() override{
        // cleanup any assignment in case release() wasn't called - 
        //  this can happen if using the old interface where callers explicitly deleted results.
        clearOwningFiber();     
    }
};

class DbResults : public eastl::vector<DbResult *>
{
public:
    typedef eastl::vector<DbResult *> base_type;
    virtual ~DbResults()
    {
        base_type::iterator itr = begin();
        base_type::iterator enditr = end();
        for (; itr != enditr; ++itr)
        {
            delete (*itr);
        }
    }
};

/*************************************************************************************************/
/*!
    \class StreamingDbResult
    \brief Contains the result of a database operation.  The results are fetched in a streaming mannor
            from the database.
*/
/*************************************************************************************************/
class StreamingDbResultBase : public DbResultCommon
{
    NON_COPYABLE(StreamingDbResultBase);

public:
    /*************************************************************************************************/
    /*!
        \brief next

        Fetches the next result in the result set from the database

        \note - the caller is responsible for de-allocating the DbRow object that is returned.
        \return - the next row in the result set, or nullptr if there are none.
    */
    /*************************************************************************************************/
    virtual DbRow* next() = 0;

protected:
    StreamingDbResultBase() {}
    ~StreamingDbResultBase() override {}
};

//  StreamingDbResultPtr interface definition
typedef FrameworkResourcePtr<StreamingDbResultBase> StreamingDbResultPtr;


//  deprecated interface - use StreamingDbResultPtr for new development.
class StreamingDbResult : public StreamingDbResultBase
{
    NON_COPYABLE(StreamingDbResult);

public:
    StreamingDbResult() {}
    ~StreamingDbResult() override {
        // cleanup any assignment in case release() wasn't called - 
        //  this can happen if using the old interface where callers explicitly deleted results.
        clearOwningFiber();     
    }
};


}// Blaze
#endif // BLAZE_DBRESULT_H
