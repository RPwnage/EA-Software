///////////////////////////////////////////////////////////////////////////////
// OriginClientFactory.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _ORIGINCLIENTFACTORY_H_INCLUDED_
#define _ORIGINCLIENTFACTORY_H_INCLUDED_

#include <QScopedPointer>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        /// \brief Generates the required singletons to access Origin clients.
        template <typename T>
        class ORIGIN_PLUGIN_API OriginClientFactory
        {
        public:
            /// \fn instance
            /// \brief If necessary, creates and then returns a pointer to the required OriginClient.
            static T* instance()
            {
                if(sMinst.isNull()) 
                { 
                    sMinst.reset(new T); 
                } 
                return sMinst.data(); 
            }

        private:
            /// \brief My singleton; private.
            static QScopedPointer<T> sMinst; 
        };

        /// \brief Definition of static member.
        template <typename T>
        QScopedPointer<T> OriginClientFactory<T>::sMinst; 

    }
}


#endif

