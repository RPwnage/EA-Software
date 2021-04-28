#ifndef _ENTITLEMENTDIALOGPROXY_H
#define _ENTITLEMENTDIALOGPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API EntitlementDialogProxy : public QObject
{
    Q_OBJECT
public:
    // Shows a dialog with the entitlement's properties.
    Q_INVOKABLE void showProperties(const QString& offerId);

    // Shows a dialog that allows the user to modify the entitlement's box art if it's a Non-Origin game.
    Q_INVOKABLE void customizeBoxart(const QString& offerId);

    /// \brief Tells the user to install base game before installing addon content.
    /// \param entitlement The add-on content offerid.
    Q_INVOKABLE void promptForParentInstallation(const QString& offerId);

    /// \brief Show download debug information on a specific entitlement
    Q_INVOKABLE void showDownloadDebug(const QString& offerId);

    /// \brief Show warning window about removing entitlement from user catalog
    Q_INVOKABLE void showRemoveEntitlementPrompt(const QString& offerId);
};

}
}
}

#endif
