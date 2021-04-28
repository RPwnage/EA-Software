#ifndef _OWNEDROSTERVIEW_H
#define _OWNEDROSTERVIEW_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QList>
#include <QSharedPointer>

#include "AbstractRosterView.h"
#include "engine/content/ContentTypes.h"
#include "engine/social/SocialController.h"
#include "engine/social/CommonTitlesController.h"
#include "services/plugin/PluginAPI.h"
#include "chat/RemoteUser.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API OwnedRosterView : public AbstractRosterView
{
public:
    OwnedRosterView(Engine::Social::SocialController *socialController, QList<Engine::Content::EntitlementRef> &entRefs) : 
        m_socialController(socialController),
        m_entRefs(entRefs)
    {
    }

private:
    bool contactIncludedInView(Chat::RemoteUser *f)
    {
        const Engine::Social::CommonTitlesController *commonTitlesController = m_socialController->commonTitles();
        const quint64 nucleusId = f->nucleusId();

        for(auto it = m_entRefs.constBegin(); it != m_entRefs.constEnd(); it++)
        {
            if (commonTitlesController->nucleusIdHasSimilarEntitlement(nucleusId, *it))
            {
                return true;
            }
        }

        return false;
    }

    Engine::Social::SocialController *m_socialController;
    QList<Engine::Content::EntitlementRef> m_entRefs;
};

}
}
}

#endif
