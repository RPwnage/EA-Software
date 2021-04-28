#include "PendingActionGameDownloadViewController.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "ClientFlow.h"

namespace Origin
{
    using namespace Engine::Content;

    namespace Client
    {
        using namespace PendingAction;

        PendingActionGameDownloadViewController::PendingActionGameDownloadViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId)
            :PendingActionBaseViewController(queryParams, launchedFromBrowser, uuid, lookupId)
        {
            mEntitlementLookupId = Engine::Content::ContentController::EntByOfferId;
            mFrontOfQueue = false;
            mRequiresEntitlements = true;
            mRequiresExtraContent = true;
        }

        void PendingActionGameDownloadViewController::performMainAction()
        {
            ensureClientVisible();
            refreshEntitlementsIfRequested();

            EntitlementRef entToShowDetails = findEntitlementToShow(mEntitlementId, mEntitlementLookupId);

            if (entToShowDetails)
            {
                // if we are telling a DLC to move to the front make sure the parent is fully installed first - being in queue is not enough
                if (mFrontOfQueue && entToShowDetails->contentConfiguration()->isPDLC() && !entToShowDetails->localContent()->isAnyParentInstalled(false, false))
                    mFrontOfQueue = false;

                if (entToShowDetails->localContent()->downloadable())
                {
                    Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::instance()->currentUser();
                    QString locale = "";
                    if (currentUser)
                        locale = currentUser->locale();

                    int flags = 0;

                    if (mFrontOfQueue)
                        flags = Origin::Engine::Content::LocalContent::kDownloadFlag_try_to_move_to_top;

                    entToShowDetails->localContent()->download(locale, flags);
                }
                else
                if (mFrontOfQueue)  // can't add a new download - but if we want to make sure it is in the front of the queue - we can try
                {
                    Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
                    if (queueController)
                    {
                        if (queueController->indexInQueue(entToShowDetails) > 0)    // in queue already but not at the head
                        {
                            queueController->pushToTop(entToShowDetails);
                        }
                    }
                }
            }

            mComponentsCompleted |= MainAction;
        }

        bool PendingActionGameDownloadViewController::parseAndValidateParams()
        {
            bool valid = PendingActionBaseViewController::parseAndValidateParams();

            //grab the string thats in the id field, could be a list of ids or a single id
            //its used for showing offer pages and the same field is used for profile
            QString id = mQueryParams.queryItemValue(PendingAction::ParamOfferIdId, QUrl::FullyDecoded);

            //if the id param is empty but we have specified a show game details page action, the request is not valid.
            //otherwise assign showGameDetails function to the function pointer
            if(!id.isEmpty())
            {
                mEntitlementId = id;
            }
            else
                valid = false;

            QString front_of_queue = mQueryParams.queryItemValue(PendingAction::ParamFrontOfQueue, QUrl::FullyDecoded);

            mFrontOfQueue = front_of_queue == QString("true");

            return valid;
        }

        EntitlementRef PendingActionGameDownloadViewController::findEntitlementToShow(QString ids, Engine::Content::ContentController::EntitlementLookupType lookup)
        {
            QStringList idList = ids.split(',', QString::SkipEmptyParts);

            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
            if(contentController)
            {
                return contentController->bestMatchedEntitlementByIds(idList, lookup, false);
            }

            return EntitlementRef();
        }
    }
}
