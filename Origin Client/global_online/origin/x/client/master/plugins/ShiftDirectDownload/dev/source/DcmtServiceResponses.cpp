#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "ShiftDirectDownload/DcmtServiceResponses.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

DcmtServiceResponse::DcmtServiceResponse(QNetworkReply *reply)
    : Services::HttpServiceResponse(reply)
    , mErrorCode(DCMT_OK)
{
    ORIGIN_VERIFY_CONNECT(reply, SIGNAL(sslErrors(const QList<QSslError> &)),
        this, SLOT(ignoreSslErrors(const QList<QSslError> &)));
}


DcmtServiceResponse::~DcmtServiceResponse()
{
    ORIGIN_VERIFY_DISCONNECT(reply(), SIGNAL(sslErrors(const QList<QSslError> &)),
        this, SLOT(ignoreSslErrors(const QList<QSslError> &)));
}


QDomElement DcmtServiceResponse::getResponseRoot(const QString &tag)
{
    QDomDocument doc;
    doc.setContent(reply()->readAll());

    const QDomElement &rootNode = doc.documentElement();
    if (rootNode.isNull())
    {
        mErrorCode = DCMT_ERR_NOT_XML;
        mErrorCause = "Invalid response";
        mErrorDescription = "The response did not contain a valid xml document";

        return QDomElement();
    }

    QNetworkReply::NetworkError networkError = reply()->error();
    if (QNetworkReply::NoError != networkError || "error" == rootNode.tagName())
    {
        int dcmtCode = rootNode.attribute("code").toInt();

        switch (dcmtCode)
        {
        case 1:   mErrorCode = DCMT_ERR_DUPLICATE_DELIVERY; break;
        case 404: mErrorCode = DCMT_ERR_NOT_FOUND; break;
        case 550: mErrorCode = DCMT_ERR_PERMISSION_DENIED; break;
        default:  mErrorCode = DCMT_ERR_UNKNOWN; break;
        }

        const QDomNodeList &children = rootNode.childNodes();
        for (int i = 0; i < children.size(); ++i)
        {
            const QDomElement &child = children.at(i).toElement();
            if (child.isNull())
            {
                continue;
            }

            if (0 == child.tagName().compare("cause"))
            {
                mErrorCause = child.text();
            }
            else if (0 == child.tagName().compare("description"))
            {
                mErrorDescription = child.text();
            }
        }

        return QDomElement();
    }

    if (rootNode.tagName() != tag)
    {
        mErrorCode = DCMT_ERR_WRONG_ROOT;
        mErrorCause = "Incorrect root node";
        mErrorDescription = "The root node of the response did not match what was expected";

        return QDomElement();
    }

    return rootNode;
}


bool DcmtServiceResponse::parseDeliveryStatus(DeliveryStatusInfo &deliveryStatus, const QDomElement &node)
{
    if (node.isNull())
    {
        return false;
    }

    const QDomNodeList &children = node.childNodes();
    for (int i = 0; i < children.size(); ++i)
    {
        const QDomElement &child = children.at(i).toElement();
        if (child.isNull())
        {
            continue;
        }

        if (0 == child.tagName().compare("buildId"))
        {
            deliveryStatus.buildId = child.text();
        }
        else if (0 == child.tagName().compare("downloadUrl"))
        {
            deliveryStatus.downloadUrl = child.text();
        }
        else if (0 == child.tagName().compare("status"))
        {
            deliveryStatus.status = child.text();
        }
        else
        {
            ORIGIN_LOG_WARNING << "Unknown field " << child.tagName();
        }
    }

    return true;
}


const QDateTime &DcmtServiceResponse::getServerTime()
{
    if (!mServerTime.isValid() && reply()->hasRawHeader("Date"))
    {
        // Sometimes the server sends multiple Date headers, and QNetworkReply
        // "helpfully" merges them, so we have to split out the first value
        const QByteArray &rawDateHeader = reply()->rawHeader("Date");
        const QList<QByteArray> &rawDateHeaderPieces = rawDateHeader.split(',');
        const QByteArray &dateHeader = rawDateHeaderPieces[0] + ',' + rawDateHeaderPieces[1];

        mServerTime = QLocale(QLocale::English, QLocale::UnitedStates).toDateTime(
            dateHeader, "ddd', 'dd' 'MMM' 'yyyy' 'HH':'mm':'ss' GMT'");
    }

    return mServerTime;
}


void DcmtServiceResponse::ignoreSslErrors(const QList<QSslError> &errors)
{
    reply()->ignoreSslErrors(errors);
}


void AuthenticateResponse::processReply()
{
    const QDomElement &rootNode = getResponseRoot("userAuthResponse");
    if (!rootNode.isNull())
    {
        const QDomNodeList &nodes = rootNode.childNodes();
        for (int i = 0; i < nodes.size(); ++i)
        {
            const QDomElement &node = nodes.at(i).toElement();
            if (node.isNull())
            {
                continue;
            }

            if (0 == node.tagName().compare("token"))
            {
                mAuthToken = node.text();
            }
            else if (0 == node.tagName().compare("expiresOn"))
            {
                mTokenExpiration = QLocale(QLocale::English, QLocale::UnitedStates).toDateTime(
                    node.text(), "yyyy'-'MM'-'dd'T'HH':'mm':'ss'.'zzz'Z'");
            }
            else
            {
                ORIGIN_LOG_WARNING << "Unknown field " << node.tagName();
            }
        }
    }

    emit complete(reply()->error());
    deleteLater();
}


QVariant AuthenticateResponse::toVariant() const
{
    QVariantMap ret;

    ret["token"] = mAuthToken;
    ret["expiresOn"] = mTokenExpiration;

    return ret;
}


void ExpireTokenResponse::processReply()
{
    emit complete(reply()->error());
    deleteLater();
}


QVariant ExpireTokenResponse::toVariant() const
{
    return QVariant();
}


void AvailableBuildsResponse::processReply()
{
    const QDomElement &rootNode = getResponseRoot("gameBuilds");
    if (!rootNode.isNull())
    {
        const QRegExp listSplitter("[\\[,\\] ]");
        const QDomNodeList &nodes = rootNode.childNodes();
        for (int i = 0; i < nodes.size(); ++i)
        {
            const QDomElement &gameBuildNode = nodes.at(i).toElement();
            if (gameBuildNode.isNull() || gameBuildNode.tagName().compare("gameBuild"))
            {
                continue;
            }

            GameBuildInfo buildInfo;
            const QDomNodeList &propertyNodes = gameBuildNode.childNodes();
            for (int j = 0; j < propertyNodes.size(); ++j)
            {
                const QDomElement &propertyNode = propertyNodes.at(j).toElement();
                if (propertyNode.isNull())
                {
                    continue;
                }

                if (0 == propertyNode.tagName().compare("buildId"))
                {
                    buildInfo.buildId = propertyNode.text();
                }
                else if (0 == propertyNode.tagName().compare("version"))
                {
                    buildInfo.version = propertyNode.text();
                }
                else if (0 == propertyNode.tagName().compare("milestone"))
                {
                    buildInfo.milestone = propertyNode.text();
                }
                else if (0 == propertyNode.tagName().compare("buildTypes"))
                {
                    buildInfo.buildTypes.append(propertyNode.text().split(listSplitter, QString::SkipEmptyParts));
                }
                else if (0 == propertyNode.tagName().compare("lifeCycleStatus"))
                {
                    buildInfo.lifeCycleStatus = propertyNode.text();
                }
                else if (0 == propertyNode.tagName().compare("status"))
                {
                    buildInfo.status = propertyNode.text();
                }
                else if (0 == propertyNode.tagName().compare("approvalStates"))
                {
                    buildInfo.approvalStates.append(propertyNode.text().split(listSplitter, QString::SkipEmptyParts));
                }
                else
                {
                    ORIGIN_LOG_WARNING << "Unknown field " << propertyNode.tagName();
                }
            }

            mGameBuilds.prepend(buildInfo);
        }
    }

    emit complete(reply()->error());
    deleteLater();
}


QVariant AvailableBuildsResponse::toVariant() const
{
    QList<QVariant> varGameBuilds;

    foreach (const GameBuildInfo &gameBuild, mGameBuilds)
    {
        QVariantMap varGameBuild;

        varGameBuild["buildId"] = gameBuild.buildId;
        varGameBuild["version"] = gameBuild.version;
        varGameBuild["milestone"] = gameBuild.milestone;
        varGameBuild["buildTypes"] = gameBuild.buildTypes;
        varGameBuild["lifeCycleStatus"] = gameBuild.lifeCycleStatus;
        varGameBuild["status"] = gameBuild.status;
        varGameBuild["approvalStates"] = gameBuild.approvalStates;

        varGameBuilds.push_back(varGameBuild);
    }

    return varGameBuilds;
}


void DeliveryRequestResponse::processReply()
{
    parseDeliveryStatus(mDeliveryStatusInfo, getResponseRoot("deliveryStatusResponse"));
    emit complete(reply()->error());
    deleteLater();
}


QVariant DeliveryRequestResponse::toVariant() const
{
    QVariantMap ret;

    ret["buildId"] = mDeliveryStatusInfo.buildId;
    ret["downloadUrl"] = mDeliveryStatusInfo.downloadUrl;
    ret["status"] = mDeliveryStatusInfo.status;

    return ret;
}


void DeliveryStatusResponse::processReply()
{
    const QDomElement &rootNode = getResponseRoot("deliveryStatusResponses");
    if (!rootNode.isNull())
    {
        const QDomNodeList &nodes = rootNode.childNodes();
        for (int i = 0; i < nodes.size(); ++i)
        {
            const QDomElement &deliveryStatusNode = nodes.at(i).toElement();
            if (deliveryStatusNode.isNull() || deliveryStatusNode.tagName().compare("deliveryStatusResponse"))
            {
                continue;
            }

            DeliveryStatusInfo deliveryStatusInfo;
            if (parseDeliveryStatus(deliveryStatusInfo, deliveryStatusNode))
            {
                mDeliveryStatusList.append(deliveryStatusInfo);
            }
        }
    }

    emit complete(reply()->error());
    deleteLater();
}


QVariant DeliveryStatusResponse::toVariant() const
{
    QList<QVariant> varDeliveryStatusList;

    foreach (const DeliveryStatusInfo &deliveryStatus, mDeliveryStatusList)
    {
        QVariantMap varDeliveryStatus;

        varDeliveryStatus["buildId"] = deliveryStatus.buildId;
        varDeliveryStatus["downloadUrl"] = deliveryStatus.downloadUrl;
        varDeliveryStatus["status"] = deliveryStatus.status;

        varDeliveryStatusList.push_back(varDeliveryStatus);
    }

    return varDeliveryStatusList;
}


void GenerateUrlResponse::processReply()
{
    parseDeliveryStatus(mDeliveryStatusInfo, getResponseRoot("deliveryStatusResponse"));
    emit complete(reply()->error());
    deleteLater();
}


QVariant GenerateUrlResponse::toVariant() const
{
    QVariantMap ret;

    ret["buildId"] = mDeliveryStatusInfo.buildId;
    ret["downloadUrl"] = mDeliveryStatusInfo.downloadUrl;
    ret["status"] = mDeliveryStatusInfo.status;

    return ret;
}

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
