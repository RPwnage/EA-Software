//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.


#ifndef ENGINE_CONTENT_TYPES_H
#define ENGINE_CONTENT_TYPES_H

#include <QSharedPointer>

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            class Entitlement;
            class ContentConfiguration;

            /// \brief TBD.
            typedef QSharedPointer<Entitlement> EntitlementRef;
            typedef QList<EntitlementRef> EntitlementRefList;
            typedef QSet<EntitlementRef> EntitlementRefSet;

            /// \brief TBD.
            typedef QWeakPointer<Entitlement> EntitlementWRef;
            typedef QList<EntitlementWRef> EntilementWRefList;

            typedef QSharedPointer<ContentConfiguration> ContentConfigurationRef;

            typedef QWeakPointer<ContentConfiguration> ContentConfigurationWRef;

        } //Content
    } //Engine
} //Origin



#endif // ENGINE_CONTENT_TYPES_H
