#include <QMap>
#include "PendingActionFactory.h"
#include "PendingActionBaseViewController.h"
#include "PendingActionGameLaunchViewController.h"
#include "PendingActionStoreOpenViewController.h"
#include "PendingActionLibraryOpenViewController.h"
#include "PendingActionCurrentTabViewController.h"
#include "PendingActionGameDownloadViewController.h"

namespace Origin
{
    namespace Client
    {
        namespace PendingActionFactory
        {
            typedef PendingActionBaseViewController * (*PendingActionFactory)(const QUrlQuery &urlQuery, bool launchedFromBrowser, QString uuid);
            
            QMap<QString, PendingActionFactory>  sPendingActionMap;

            //the /store/open action
            PendingActionBaseViewController *createStoreOpenAction(const QUrlQuery &urlQuery, bool launchedFromBrowser, QString uuid)
            {
                return (new PendingActionStoreOpenViewController(urlQuery, launchedFromBrowser, uuid, PendingAction::StoreOpenLookupId));
            }

            //the /game/launch action
            PendingActionBaseViewController *createGameLaunchAction(const QUrlQuery &urlQuery, bool launchedFromBrowser, QString uuid)
            {
                return (new PendingActionGameLaunchViewController(urlQuery, launchedFromBrowser, uuid, PendingAction::GameLaunchLookupId));
            }

            //the /library/open action
            PendingActionBaseViewController *createLibraryOpenAction(const QUrlQuery &urlQuery, bool launchedFromBrowser, QString uuid)
            {
                return (new PendingActionLibraryOpenViewController(urlQuery, launchedFromBrowser, uuid, PendingAction::LibraryOpenLookupId));
            }

            //the currenttab action
            PendingActionBaseViewController *createCurrentTabAction(const QUrlQuery &urlQuery, bool launchedFromBrowser, QString uuid)
            {
                return (new PendingActionCurrentTabViewController(urlQuery, launchedFromBrowser, uuid, PendingAction::CurrentTabLookupId));
            }

            //the /game/download action
            PendingActionBaseViewController *createGameDownloadAction(const QUrlQuery &urlQuery, bool launchedFromBrowser, QString uuid)
            {
                return (new PendingActionGameDownloadViewController(urlQuery, launchedFromBrowser, uuid, PendingAction::GameDownloadLookupId));
            }

            void registerPendingActions()
            {
                sPendingActionMap[PendingAction::StoreOpenLookupId] = &createStoreOpenAction;
                sPendingActionMap[PendingAction::LibraryOpenLookupId] = &createLibraryOpenAction;
                sPendingActionMap[PendingAction::GameLaunchLookupId] = &createGameLaunchAction;
                sPendingActionMap[PendingAction::CurrentTabLookupId] = &createCurrentTabAction;
                sPendingActionMap[PendingAction::GameDownloadLookupId] = &createGameDownloadAction;
            }

            QString lookupIdFromUrl(const QUrl &url)
            {
                QString lookup = url.host() + url.path();
                if(!lookup.isEmpty())
                {
                    //if we have an extra slash, chop it off
                    if(lookup.at(lookup.length()-1) == '/')
                        lookup.chop(1);

                    //remove any slashes at beginning
                    if(lookup.at(0) == '/')
                    {
                        lookup.remove(0, 1);
                    }
                }

                return lookup;
            }

            PendingActionBaseViewController *createPendingActionViewController(const QUrl &url, bool launchedFromBrowser, QString uuid)
            {
                //parse the id from the url e.g /store/open, /library/open
                QString lookup = lookupIdFromUrl(url);

                PendingActionBaseViewController *action = NULL;

                PendingActionFactory actionFactory = NULL;
                if(sPendingActionMap.contains(lookup))
                {
                    actionFactory = sPendingActionMap[lookup];

                    if(actionFactory)
                    {
                        QUrlQuery urlQuery(url.query());

                        //if we found a factory, create the actioni
                        action = actionFactory(urlQuery, launchedFromBrowser, uuid);
                    }
                }
                
                return action;
            }
        }
    }
}