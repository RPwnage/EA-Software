#include "SearchFriendsOperationProxy.h"

#include <QMetaObject>

#include "services/rest/SearchFriendServiceClient.h"
#include "services/rest/SearchFriendServiceResponses.h"
#include "services/debug/DebugService.h"
#include "engine/login/LoginController.h"
#include "OriginSocialProxy.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            SearchFriendsOperationProxy::SearchFriendsOperationProxy(QObject *parent, const QString &searchKeyword, const QString& pageNumber) :
                QObject(parent)
                ,mSearchKeyword(searchKeyword)
                ,mPageNumber(pageNumber)
            {   
            }

            void SearchFriendsOperationProxy::execute()
            {
                //  Only replace the '+' with percent encoded form
                QString searchKeywordEncoded(mSearchKeyword);
                searchKeywordEncoded.replace(QString("+"), QString("%2B"));

                Services::SearchFriendServiceResponse* resp = Services::SearchFriendServiceClient::searchFriend(Services::Session::SessionService::accessToken(Engine::LoginController::currentUser()->getSession()).toLatin1(),searchKeywordEncoded, mPageNumber);
                ORIGIN_ASSERT(resp);

                if(NULL!=resp)
                {
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(searchSucceeded()));
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(searchFailed()));
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::restError)), this, SLOT(searchFailed()));

                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), resp, SLOT(deleteLater()));
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(deleteLater()));
                }
            }

            void SearchFriendsOperationProxy::searchFailed()
            {
                emit failed();
            }

            void SearchFriendsOperationProxy::searchSucceeded()
            {
#if 0 //disable for Origin X
                Services::SearchFriendServiceResponse *resp = dynamic_cast<Services::SearchFriendServiceResponse*>(sender());

                if (!resp)
                {
                    ORIGIN_ASSERT(0);
                    return;
                }

                if(resp && resp->error() == Origin::Services::restErrorSuccess)
                {
                    ORIGIN_ASSERT(resp->list().size() > 0);
                    ORIGIN_ASSERT(resp->totalCount() > 0);
                    const QList<Services::SearchInfo> list  = resp->list();

                    QVariantList searchInformation;

                    for(QList<Services::SearchInfo>::ConstIterator it = list.constBegin();
                        it != list.constEnd();
                        it++)
                    {
                        /*
                        struct SearchInfo
                        {
                            QString mAvatarLink;
                            QString mEaid;
                            QString mFriendUserId;
                            QString mFullName;
                        };
                        */
                        const quint64 nucleusId = (*it).mFriendUserId.toULongLong();

                        QVariantMap searchInfo;
                        searchInfo["avatarUrl"] = (*it).mAvatarLink;
                        searchInfo["nickname"] = (*it).mEaid;
                        OriginSocialProxy* proxy = OriginSocialProxy::instance();
                        if (proxy)
                            searchInfo["userId"] = proxy->userIdByNucleusId(nucleusId);
                        else
                            searchInfo["userId"] = "";
                        searchInfo["fullName"] = (*it).mFullName;

                        searchInformation.append(searchInfo);
                    }
                    QVariantMap totalcount;
                    totalcount["totalCount"] = QString::number(resp->totalCount());
                    searchInformation.append(totalcount);

                    emit succeeded(searchInformation);

                }
#endif
            }
        }
    }
}
