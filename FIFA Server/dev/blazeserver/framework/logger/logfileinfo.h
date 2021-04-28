/*************************************************************************************************/
/*!
\file logfileinfo.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#ifndef BLAZE_LOGFILEINFO_H
#define BLAZE_LOGFILEINFO_H

#include "framework/blazedefines.h"

#include "EASTL/intrusive_list.h"
#include "EATDF/time.h"

namespace Blaze
{

class LogFileInfo : public eastl::intrusive_list_node
{
public:
    LogFileInfo();
    virtual ~LogFileInfo() {}

    void initialize(const char8_t* subName, const char8_t* subFolderName = "");

    void outputData(const char8_t* buffer, uint32_t count, eastl::intrusive_list<LogFileInfo>& flushList, bool allowRotate);

    bool isEnabled() const { return mEnabled && !mFileName.empty(); }

    void rotate(TimeValue now = TimeValue::getTimeOfDay());

    void open();
    void close();
    void flush();

    void setState(bool state) { mEnabled = state; }

    void setRotationPeriod(TimeValue rotationPeriod) { mRotationPeriod = rotationPeriod; }
    TimeValue getRotationPeriod() const { return mRotationPeriod; }
    TimeValue getLastRotation() const { return mLastRotation; }

    uint32_t getSeqCounter() const { return mSequenceCounter; }

    uint32_t incSeqCounter() { return ++mSequenceCounter; }

    int getFd() { return mFp ? fileno(mFp) : -1; }

private:
    bool mInitialized;
    bool mEnabled;
    bool mIsBinary;
    FILE* mFp;
    uint32_t mWrittenSize;
    uint32_t mFlushedSize;
    TimeValue mLastRotation;
    uint32_t mSequenceCounter;
    TimeValue mOpenFailureTime;
    TimeValue mRotationPeriod;

    eastl::string mFileDirectory;
    eastl::string mFileName;
    //eastl::string mFileExt;
};

}
#endif