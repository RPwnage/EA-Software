#include "OfferProgressService.h"
#include "engine/content/ContentController.h"
#include "engine/login//LoginController.h"
#include "engine/downloader/ContentServices.h"
#include "engine/content/LocalInstallProperties.h"

using namespace Origin::Engine::Content;

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            const char paramMasterTitleId[] = "masterTitleId";
            const char paramOfferIds[] = "offerIds";

            QString stateToStringLookup(LocalContent::States state, bool repairing, bool updating, bool enqueued)
            {
                QString stateString;

                switch(state)
                {
                case LocalContent::Downloading:
                case LocalContent::FinalizingDownload:
                case LocalContent::Paused:
                    stateString = "DOWNLOADING";
                    break;
                case LocalContent::Installing:
                    stateString = "INSTALLING";
                    break;
                case LocalContent::Installed:
                case LocalContent::ReadyToPlay:
                case LocalContent::Playing:
                    stateString = "INSTALLED";
                    break; 

                case LocalContent::ReadyToDownload:
                case LocalContent::NotPurchased:
                case LocalContent::ReadyToInstall:
                    stateString = "NOT_INSTALLED";
                    break;
                case LocalContent::ReadyToUnpack:
                case LocalContent::Unpacking:
                case LocalContent::ReadyToDecrypt:
                case LocalContent::Decrypting:
                case LocalContent::Preparing:
                case LocalContent::Unreleased:
                case LocalContent::Busy:
                    stateString = "BUSY";
                    break; 
                default:
                    stateString = "UNKNOWN";
                    break;
                }

                //check against the repairing/updating flags and update the state string as those states are not represented in LocalContent::States
                if (enqueued)
                {
                    stateString = "QUEUED";
                }
                else
                if(repairing)
                {
                    stateString = "BUSY";
                }
                else
                if(updating)
                {
                    stateString = "UPDATING";
                }

                return stateString;
            }

            ResponseInfo OfferProgressService::processRequest(QUrl requestUrl)
            {
                ResponseInfo responseInfo;
                Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();

                QUrlQuery queryParams(requestUrl.query());

                QString offerIds = queryParams.queryItemValue(paramOfferIds, QUrl::FullyDecoded);
                QString masterTitleId = queryParams.queryItemValue(paramMasterTitleId, QUrl::FullyDecoded);

                QString offerString;

                if(!offerIds.isEmpty() || !masterTitleId.isEmpty())
                {
                    if(Origin::Engine::LoginController::isUserLoggedIn())
                    {
                        if(contentController)
                        {
                            QList<Origin::Engine::Content::EntitlementRef> entitlementList;
                            QStringList notFoundList;
                            if(!masterTitleId.isEmpty())
                            {
                                entitlementList = contentController->entitlementsByMasterTitleId(masterTitleId);
                                if(entitlementList.isEmpty())
                                {
                                    responseInfo.errorCode = HttpStatus::NotFound;
                                    responseInfo.response += "{";
                                    responseInfo.response += QString("\"masterTitleId\":\"%1\"").arg(masterTitleId);
                                    responseInfo.response +=",";
                                    responseInfo.response += QString("\"error\":\"%1\"").arg("OFFERS_NOT_FOUND");
                                    responseInfo.response += "}";
                                    return responseInfo;
                                }
                            }
                            else
                            {
                                QStringList offerIdList = offerIds.split(",");
                                for ( QStringList::ConstIterator it = offerIdList.begin(); it != offerIdList.end(); it++ )
                                {
                                    Origin::Engine::Content::EntitlementRef ent = contentController->entitlementByProductId(*it);
                                    if(ent)
                                    {
                                        entitlementList.push_back(ent);
                                    }
                                    else
                                    {
                                        notFoundList.push_back(*it);
                                    }

                                }
                            }

                            //first process the list with all the entitlements
                            offerString = "\"offers\":[";
                            for ( QList<Origin::Engine::Content::EntitlementRef>::ConstIterator it = entitlementList.begin(); it != entitlementList.end(); it++ )
                            {
                                if(it != entitlementList.begin())
                                    offerString += ",";

                                offerString += "{";
                                Origin::Engine::Content::EntitlementRef ent = *it;

                                if(ent)
                                {
                                    QString errorState;

                                    if(ent->localContent()->installFlow() && (ent->localContent()->installFlow()->getFlowState() == Origin::Downloader::ContentInstallFlowState::kError))
                                    {
                                        errorState = "ERROR";
                                    }
                                    else
                                    {
                                        bool enqueued = (ent->localContent()->installFlow() && (ent->localContent()->installFlow()->getFlowState() == Origin::Downloader::ContentInstallFlowState::kEnqueued));
                                        errorState = stateToStringLookup(ent->localContent()->state(), ent->localContent()->repairing(), ent->localContent()->updating(), enqueued);
                                    }
                                    //look for non-ascii chars and convert them to their hex code representation
                                    QString displayName8Bit = ent->contentConfiguration()->displayName();
                                    for(int i = 0; i < displayName8Bit.length(); i++)
                                    {
                                        QChar textChar = displayName8Bit.at(i);

                                        if(textChar.toLatin1() == 0)
                                        {
                                            QString unicodeStr;
                                            unicodeStr.sprintf("\\u%04x", textChar.unicode());
                                            displayName8Bit.replace(i, 1, unicodeStr.toLocal8Bit());
                                        }
                                        else
                                        if (textChar == '\"')   // look for double-quotes as these will break the json formatting for the displayname in the response
                                        {
                                            QString escapeStr("\\u0022");   // double-quote in unicode
                                            displayName8Bit.replace(i, 1, escapeStr.toLocal8Bit());
                                        }
                                    }

                                    QString contentType = QString("UNDEFINED");
                                    if (ent->contentConfiguration()->isBaseGame())
                                        contentType = QString("BASE_GAME");
                                    else if (ent->contentConfiguration()->isPDLC())
                                        contentType = QString("PDLC");
                                    else if (ent->contentConfiguration()->isPULC())
                                        contentType = QString("UNLOCKABLE");

                                    offerString += QString("\"offerId\":\"%1\"").arg(ent->contentConfiguration()->productId());
                                    offerString +=",";
                                    offerString += QString("\"contentId\":\"%1\"").arg(ent->contentConfiguration()->contentId());
                                    offerString +=",";
                                    offerString += QString("\"softwareId\":\"%1\"").arg(ent->contentConfiguration()->softwareId());
                                    offerString +=",";
                                    offerString += QString("\"displayName\":\"%1\"").arg(QString(displayName8Bit));
                                    offerString +=",";
                                    offerString += QString("\"contentType\":\"%1\"").arg(contentType);
                                    offerString +=",";
                                    offerString += QString("\"progress\":%1").arg(QString::number(ent->localContent()->installFlow() ? ent->localContent()->installFlow()->progressDetail().mProgress : 0.0f, 'f',4));
                                    offerString += ",";
                                    offerString += QString("\"state\":\"%1\"").arg(errorState);
                                    offerString += ",";


                                    // figure out play state
                                    QString playState;

                                    switch(ent->localContent()->playableStatus())
                                    {
                                    case LocalContent::PS_Playable:
                                        playState = "READY_TO_PLAY";
                                        break;
                                    case LocalContent::PS_Playing:
                                        playState = "PLAYING";
                                        break;
                                    case LocalContent::PS_Busy:
                                        playState = "BUSY";
                                        break;
                                    default:
                                        playState = "NOT_PLAYABLE";
                                        break;
                                    }

                                    offerString += QString("\"playState\":\"%1\"").arg(playState);
                                    offerString += ",";

                                    bool update_available = ent->localContent()->updateAvailable();
                                    Origin::Downloader::LocalInstallProperties* localInstallProperties = ent->localContent()->localInstallProperties();
                                    bool update_mandatory = localInstallProperties ? localInstallProperties->GetTreatUpdatesAsMandatory() : false;

                                    if (ent->localContent()->installFlow() && ent->localContent()->installFlow()->isActive())
                                    {
                                        QString currentAvailableVersion = ent->contentConfiguration()->version();
                                        QString currentActiveVersion;

                                        Origin::Downloader::Parameterizer installManifest;
                                        if (Origin::Downloader::ContentServices::LoadInstallManifest(ent, installManifest))
                                        {
                                            installManifest.GetString("contentversion", currentActiveVersion);
                                            if (currentActiveVersion == currentAvailableVersion)
                                            {
                                                update_available = false;   // if an update is available but a download operation is active, then we assume it will update the content
                                            }
                                        }
                                    }

                                    offerString += QString("\"updateAvailable\":\"%1\"").arg(update_available ? (update_mandatory ? QString("mandatory") : QString("true")) : QString("false"));
                                    offerString += ",";
                                    offerString += QString("\"localVersion\":\"%1\"").arg(ent->localContent()->installedVersion());
                                }
                                offerString += "}";
                            }

                            for ( QStringList::ConstIterator it = notFoundList.begin(); it != notFoundList.end(); it++ )
                            {
                                if(it != notFoundList.begin() || !entitlementList.isEmpty())
                                    offerString += ",";

                                offerString += "{";
                                offerString += QString("\"offerId\":\"%1\"").arg(*it);
                                offerString +=",";
                                offerString += QString("\"error\":\"%1\"").arg("OFFER_NOT_FOUND");
                                offerString += "}";
                            }
                            offerString += "]";

                            QString responseString;
                            responseString += "{";
                            responseString += offerString;
                            responseString += "}";
                            responseInfo.response = responseString;
                            responseInfo.errorCode = HttpStatus::OK;
                        }
                        else
                        {
                            responseInfo.errorCode = HttpStatus::NotFound;
                            responseInfo.response = "{\"error\":\"ENTITLEMENTS_NOT_AVAILABLE\"}";
                        }

                    }
                    else
                    {
                        responseInfo.errorCode = HttpStatus::Forbidden;
                        responseInfo.response = "{\"error\":\"NOT_LOGGED_IN\"}";
                    }
                }
                else
                {
                    responseInfo.errorCode = HttpStatus::BadRequest;
                    responseInfo.response = "{\"error\":\"MISSING_ID\"}";
                }
                return responseInfo;
            }
        }
    }
}