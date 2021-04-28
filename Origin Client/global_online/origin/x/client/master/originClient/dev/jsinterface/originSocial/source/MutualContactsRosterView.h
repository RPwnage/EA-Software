#ifndef _MUTUALCONTACTSROSTERVIEW_H
#define _MUTUALCONTACTSROSTERVIEW_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include "AbstractRosterView.h"
#include "chat/RemoteUser.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API MutualContactsRosterView : public AbstractRosterView
{
private:
    bool contactIncludedInView(Chat::RemoteUser *f)
    {
        return f->subscriptionState().isContactSubscribedToCurrentUser() &&
               f->subscriptionState().isCurrentUserSubscribedToContact();
    }
};

}
}
}

#endif

