///////////////////////////////////////////////////////////////////////////////
// CloudSaveDebugActions.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _CLOUDSAVEDEBUGACTIONS_H
#define _CLOUDSAVEDEBUGACTIONS_H

#include <QObject>
#include <QList>

#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

class QAction;

namespace Origin
{
namespace Client
{

/// \brief Utility class to help debug cloud saves.
class ORIGIN_PLUGIN_API CloudSaveDebugActions : public QObject
{
    Q_OBJECT
public: 
    /// \brief The CloudSaveDebugActions constructor.
    /// \param entitlement The entitlement to debug.
    /// \param parent This object's parent object.
    CloudSaveDebugActions(Engine::Content::EntitlementRef entitlement, QObject *parent = NULL);

    /// \brief Gets a list of actions.
    /// \return A list of actions.
    QList<QAction*> actions() const;

    /// \brief Gets a list of eligible save files.
    /// \return A list of eligible save files.
    QStringList eligibleSaveFiles() const;

public slots:
	void clearRemoteArea(); ///< Clears the remote save.

private slots:
	void queryUsage(); ///< Queries the remote usage.
	void showEligibleSaveFiles(); ///< Shows all files eligible for cloud save.

private:
    Engine::Content::EntitlementRef mEntitlement; ///< The entitlement to debug.
    QList<QAction*> mActions; ///< The list of applicable actions.
};

}
}

#endif