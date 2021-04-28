#ifndef _PLAYINGROSTERVIEW_H
#define _PLAYINGROSTERVIEW_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include "AbstractRosterView.h"

#include <QStringList>
#include "chat/RemoteUser.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API PlayingRosterView : public AbstractRosterView
{
public:
    PlayingRosterView(QStringList productIds = QStringList()) : m_productIds(productIds)
    {
    }

private:
    bool contactIncludedInView(Chat::RemoteUser *f)
    {
        if (f->presence().gameActivity().isNull())
        {
            return false;
        }

        // Is there a content ID filter?
        if (!m_productIds.isEmpty())
        {
            return m_productIds.contains(f->presence().gameActivity().productId());
        }
        
        return true;
    }

    QStringList m_productIds;
};

}
}
}

#endif
