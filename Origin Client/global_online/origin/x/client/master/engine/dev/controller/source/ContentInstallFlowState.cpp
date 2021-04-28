/////////////////////////////////////////////////////////////////////////////
// ContentInstallFlowState.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "engine/downloader/ContentInstallFlowState.h"

#include <qmetaobject.h>

namespace Origin
{

namespace Downloader
{

    ContentInstallFlowState::ContentInstallFlowState() :
        mValue(kInvalid)
    {

    }

    ContentInstallFlowState::ContentInstallFlowState(enum value v) :
        mValue(v)
    {

    }

    ContentInstallFlowState::ContentInstallFlowState(qint64 v)
    {
        if (v <= kCompleted && v > kInvalid)
        {
            mValue = static_cast<enum value>(v);
        }
        else
        {
            mValue = kInvalid;
        }
    }

    ContentInstallFlowState::ContentInstallFlowState(const QString& name)
    {
        mValue = static_cast<enum value>(ContentInstallFlowState::staticMetaObject.enumerator(0).keyToValue(name.toLatin1()));
    }

    ContentInstallFlowState::ContentInstallFlowState(const ContentInstallFlowState& other)
    {
        if (this != &other)
        {
            mValue = other.mValue;
        }
    }

    ContentInstallFlowState::operator int() const
    {
        return mValue;
    }

    ContentInstallFlowState::operator const char*() const
    {
        return ContentInstallFlowState::staticMetaObject.enumerator(0).valueToKey(mValue);
    }

    ContentInstallFlowState::operator QString() const
    {
        return toString();
    }

    const ContentInstallFlowState& ContentInstallFlowState::operator=(const ContentInstallFlowState& other)
    {
        if (this != &other)
        {
            mValue = other.mValue;
        }

        return *this;
    }

    bool ContentInstallFlowState::operator==(enum value v) const
    {
        return mValue == v;
    }

    bool ContentInstallFlowState::operator==(const ContentInstallFlowState& other) const
    {
        return mValue == other.mValue;
    }

    bool ContentInstallFlowState::operator!=(enum value v) const
    {
        return mValue != v;
    }

    bool ContentInstallFlowState::operator!=(const ContentInstallFlowState& other) const
    {
        return mValue != other.mValue;
    }

    bool ContentInstallFlowState::operator<(enum value v) const
    {
        return mValue < v;
    }

    bool ContentInstallFlowState::operator<(const ContentInstallFlowState& other) const
    {
        return mValue < other.mValue;
    }

    enum ContentInstallFlowState::value ContentInstallFlowState::getValue() const
    {
        return mValue;
    }

    QString ContentInstallFlowState::toString() const
    {
        QString name(ContentInstallFlowState::staticMetaObject.enumerator(0).valueToKey(mValue));
        return name;
    }

} // namespace Downloader
} // namespace Origin

