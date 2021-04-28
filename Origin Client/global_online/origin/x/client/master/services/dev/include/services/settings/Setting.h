//  Setting.h
//  Copyright 2011-2012 Electronic Arts Inc. All rights reserved.

#ifndef _EBISUCOMMON_SETTING_H
#define _EBISUCOMMON_SETTING_H

#include <QtCore>
#include "services/settings/Variant.h"
#include "services/plugin/PluginAPI.h"
#include <assert.h>

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API Setting;

        /// \brief Defines an Origin setting.
        /// 
        /// The Setting class serves to define a setting. Typically, settings are identified by a unique name
        /// and has a number of properties, such as type, default value, etc.
        class ORIGIN_PLUGIN_API Setting : public QObject
        {
            friend void *QtMetaTypePrivate::QMetaTypeFunctionHelper<Origin::Services::Setting, 1>::Create(const void *);
            friend void *QtMetaTypePrivate::QMetaTypeFunctionHelper<Origin::Services::Setting, 1>::Construct(void *where, const void *t);

            Q_OBJECT

        public:
            
            /// \typedef Setting::Encryption
            /// \brief Defines whether a setting is stored in an encrypted form.
            typedef enum
            {
                Unencrypted,
                Encrypted,
                DefaultEncryption = Unencrypted
            }
            Encryption;
        
            /// \typedef Setting::Scope
            /// \brief Defines which users a setting applies to.
            typedef enum
            {
                LocalAllUsers,              // Applies to all Origin Users on this machine (Located in c:\ProgramData\Origin\local.xml)
                LocalPerAccount,            // Applies to all Origin Users on this machine for a particular Windows User (Located in c:\Users\<username>\AppData\Roaming\Origin\local.xml)
                LocalPerUser,               // Applies to specific Origin User on this machine for a particular Windows User (Located in c:\Users\<username>\AppData\Roaming\Origin\local_<hash>.xml)
                GlobalPerUser,              // Applies to specific Origin User on any machine (Located on Atom server)
                GlobalOverrideAllUsers,     // Unused
                LocalOverrideAllUsers,      // Applies to EACore.ini overrides
                DefaultScope = LocalAllUsers
            }
            Scope;

            /// \typedef Setting::Mutability
            /// \brief 
            typedef enum
            {
                ReadOnly,
                ReadWrite,
                DefaultMutability = ReadWrite
            }
            Mutability;
        
            /// \fn Setting
            /// \brief Constructor for a setting
            explicit Setting(QString const& name,
                    Variant::Type type = Variant::String,
                    Variant defaultValue = Variant(""),
                    Scope scope = DefaultScope,
                    Mutability mutability = DefaultMutability,
                    Encryption encryption = DefaultEncryption); 

            /// \fn Setting
            /// \brief Default constructor. DO NOT USE. Only provided for QMetaObject use. 
            Setting();

            /// \fn Setting
            /// \brief Copy constructor. DO NOT USE. Only provided for QMetaObject use. 
            Setting(Setting const& from);

            /// \fn Setting
            /// \brief Destructor for a setting
            virtual ~Setting();
            
            /// \fn updateDefaultValue
            /// \brief the default value of a read-only setting can be changed
            void updateDefaultValue(Variant defaultValue);

            /// \fn hasDefaultValueUpdated()
            /// \brief Returns true if the default value has been changed
            bool hasDefaultValueUpdated() const;

            /// \fn name
            /// \brief Returns the name of the setting.
            QString const& name() const;

            /// \fn type
            /// \brief Returns the type of the setting in terms of the Qt typing system.
            Variant::Type type() const;

            /// \fn defaultValue
            /// \brief Returns the default value of the setting.
            Variant const& defaultValue() const;

            /// \fn scope
            /// \brief Returns the scope of the setting.
            Scope scope() const;

            /// \fn encryption
            /// \brief Returns how the setting is encrypted.
            Encryption encryption() const;

            /// \fn mutability
            /// \brief Returns whether the setting can be changed or not.
            Mutability mutability() const;

        private:

            QString mName;
            Variant::Type mType;
            Variant mDefaultValue;
            Scope mScope;
            Encryption mEncryption;
            Mutability mMutability;
            bool mHasDefaultValueUpdated;

            /// \fn Setting      
            /// \brief Copy operator. DO NOT USE!
            Setting& operator = (const Setting&);

        };
   
        // inline definitions

            
        inline QString const& Setting::name() const
        {
            return mName;
        }

        inline Variant::Type Setting::type() const
        {
            return mType;
        }

        inline Variant const& Setting::defaultValue() const
        {
            return mDefaultValue;
        }

        inline Setting::Scope Setting::scope() const
        {
            return mScope;
        }

        inline Setting::Encryption Setting::encryption() const
        {
            return mEncryption;
        }

        inline Setting::Mutability Setting::mutability() const
        {
            return mMutability;
        }

        inline bool Setting::hasDefaultValueUpdated() const {
            return mHasDefaultValueUpdated;
        }
    }
}

#endif // _EBISUCOMMON_SETTING_H
