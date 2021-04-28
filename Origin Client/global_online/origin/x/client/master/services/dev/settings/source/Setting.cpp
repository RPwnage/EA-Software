//
//  Setting.cpp
//
//  Created by Kiss, Bela on 11-11-17.
//  Copyright 2011-2012 Electronic Arts Inc. All rights reserved.
//

#include "Setting.h"
#include "SettingsManager.h"
#include "services/debug/DebugService.h"

namespace Origin
{
    namespace Services
    {
        // public
        // IMPORTANT: a setting name can only exist once, otherwise our internal settings map gets confused!!!

        Setting::Setting(
            QString const& name,
            Variant::Type type,
            Variant defaultValue,
            Scope scope,
            Mutability mutability,
            Encryption encryption)
            : QObject(NULL)
            , mName(name)
            , mType(type)
            , mDefaultValue(defaultValue)
            , mScope(scope)
            , mEncryption(encryption)
            , mMutability(mutability)
            , mHasDefaultValueUpdated(false)
        {
            // TODO: implement system to track Setting definitions to insure there is no conflict between definition
            if (mScope == Setting::LocalOverrideAllUsers)
            {
                SettingsManager::addToInternalSettingsOverrideMap(mName, this);
            }
            else
            {
                SettingsManager::addToInternalSettingsMap(mName, this);
            }
        }

        Setting::Setting()
            : QObject(NULL)
            , mName()
            , mType(Variant::String)
            , mDefaultValue(Variant(""))
            , mScope(DefaultScope)
            , mEncryption(DefaultEncryption)
            , mMutability(DefaultMutability)
            , mHasDefaultValueUpdated(false)
        {
        }

        Setting::Setting(Setting const& from)
            : QObject(NULL)
        {
            mName = from.mName;
            mType = from.mType;
            mDefaultValue = from.mDefaultValue;
            mScope = from.mScope;
            mMutability = from.mMutability;
            mEncryption = from.mEncryption;
            mHasDefaultValueUpdated = from.mHasDefaultValueUpdated;
        }

        Setting::~Setting()
        {
            if (mScope == Setting::LocalOverrideAllUsers)
                SettingsManager::removeFromInternalSettingsOverrideMap(mName);
            else
                SettingsManager::removeFromInternalSettingsMap(mName);
        }

        void Setting::updateDefaultValue( Variant defaultValue )
        {
            ORIGIN_ASSERT(defaultValue.isNull() || mType == defaultValue.type()); // TODO use our own assert
            if (defaultValue.isNull())
            {
                return;
            }
            mDefaultValue = defaultValue;
            mHasDefaultValueUpdated = true;
        }

    }
} // namespace Origin
