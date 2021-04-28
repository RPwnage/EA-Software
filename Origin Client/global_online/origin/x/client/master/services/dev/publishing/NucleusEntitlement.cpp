//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#include "services/publishing/NucleusEntitlement.h"

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include "lsx.h"
#include "ReaderCommon.h"

#include <QStringList>
#include <QJsonDocument>

namespace Origin
{

namespace Services
{

namespace Publishing
{

NucleusEntitlementRef NucleusEntitlement::INVALID_ENTITLEMENT;

IMPLEMENT_PROPERTY_CONTAINER(NucleusEntitlement)
{
    setId(0);
}

NucleusEntitlement& NucleusEntitlement::operator=(const NucleusEntitlement& rhs)
{
    if(this != &rhs)
    {
        // ORPUB-1389: Don't overwrite a valid cdKey or updatedDate with a blank value.
        const QString& oldCdKey = cdKey();
        const QDateTime& oldUpdatedDate = updatedDate();

        setproperties(rhs);

        if (cdKey().isEmpty())
        {
            setCdKey(oldCdKey);
        }

        if (!updatedDate().isValid())
        {
            setUpdatedDate(oldUpdatedDate);
        }
    }

    return *this;
}

bool NucleusEntitlement::operator==(const NucleusEntitlement &rhs) const
{
    return (this == &rhs || this->properties() == rhs.properties());
}

void NucleusEntitlement::updateEntitlement(const NucleusEntitlementRef& other)
{
    // id's have to match for update
    if (other->id() == id())
    {
        const bool entitlementsMatch = (*this == *other);
        if (!entitlementsMatch)
        {
            *this = *other;
            emit entitlementChanged();
        }
    }
    else
    {
        ORIGIN_LOG_WARNING << "Ignoring entitlement update for entitlement [" << id() << "] as other entitlement has different id [" << other->id() << "]";
    }
}

bool NucleusEntitlement::parseJson(const QByteArray& payload, NucleusEntitlementList& entitlementList)
{
    // LARS3_TODO: Revisit error handling
    QJsonParseError jsonResult;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &jsonResult);
    if (jsonResult.error != QJsonParseError::NoError)
    {
        return false;
    }

    const QJsonObject &doc = jsonDoc.object();
    foreach (const QJsonValue &entVal, doc["entitlements"].toArray())
    {
        NucleusEntitlementRef ent = NucleusEntitlement::parseJsonObject(entVal.toObject());
        if (ent != NucleusEntitlement::INVALID_ENTITLEMENT)
        {
            entitlementList.push_back(ent);
        }
        else
        {
            ORIGIN_LOG_ERROR << "Failed to parse entitlement.";
        }
    }

    return true;
}

NucleusEntitlementRef NucleusEntitlement::parseJsonObject(const QJsonObject& json)
{
    QString productId;

    if (json.contains("productId"))
    {
        productId = json["productId"].toString();
    }
    else if (json.contains("offerId"))
    {
        productId = json["offerId"].toString();
    }

    if (json.contains("entitlementId") && !productId.isEmpty())
    {
        NucleusEntitlementRef newEntitlement(new NucleusEntitlement());

        newEntitlement->setId(static_cast<quint64>(json["entitlementId"].toDouble()));
        newEntitlement->setProductId(productId);
        newEntitlement->setTag(json["entitlementTag"].toString());
        newEntitlement->setType(json["entitlementType"].toString());
        newEntitlement->setGrantDate(QDateTime::fromString(json["grantDate"].toString(), Qt::ISODate));
        newEntitlement->setOriginPermissions(static_cast<OriginPermissions>(json["originPermissions"].toString().toUShort()));
        newEntitlement->setCdKey(json["cdKey"].toString());
        newEntitlement->setStatus(convertEntitlementStatus(json["status"].toString()));
        newEntitlement->setSource(json["entitlementSource"].toString());
        newEntitlement->setGroupName(json["groupName"].toString());
        newEntitlement->setProductCatalog(json["productCatalog"].toString());
        newEntitlement->setProjectId(json["projectId"].toString());
        newEntitlement->setTerminationDate(QDateTime::fromString(json["terminationDate"].toString(), Qt::ISODate));
        newEntitlement->setUseCount(json["useCount"].toInt());
        newEntitlement->setConsumable(json["isConsumable"].toBool());
        newEntitlement->setVersion(json["version"].toInt());
        newEntitlement->setUpdatedDate(QDateTime::fromString(json["updatedDate"].toString(), Qt::ISODate));
        newEntitlement->setSuppressedOfferIds(json["suppressedOfferIds"].toVariant().toStringList());
        newEntitlement->setSuppressedBy(json["suppressedBy"].toVariant().toStringList());
        newEntitlement->setExternalType(convertExternalType(json["externalType"].toString()));
        newEntitlement->setExternalId(json["externalId"].toString());
        newEntitlement->setStatusReasonCode(convertStatusReasonCode(json["statusReasonCode"].toString()));

        return newEntitlement;
    }

    return NucleusEntitlement::INVALID_ENTITLEMENT;
}

NucleusEntitlementStatus NucleusEntitlement::convertEntitlementStatus(const QString &status)
{
    if ("ACTIVE" == status)
    {
        return ACTIVE;
    }
    else if ("DELETED" == status)
    {
        return DELETED;
    }
    else if ("DISABLED" == status)
    {
        return DISABLED;
    }
    else
    {
        ORIGIN_LOG_DEBUG << "Unknown nucleus entitlement status [" << status << "]";
        return UNKNOWN;
    }
}

NucleusEntitlement::ExternalType NucleusEntitlement::convertExternalType(const QString &type)
{
    const QString &lowerType = type.toLower();

    if ("subscription" == lowerType)
    {
        return ExternalTypeSubscription;
    }
    else
    {
        return ExternalTypeNone;
    }
}

NucleusEntitlement::StatusReasonCode NucleusEntitlement::convertStatusReasonCode(const QString &code)
{
    const QString &lowerCode = code.toLower();

    if ("subscription_expired" == lowerCode)
    {
        return StatusReasonCodeSubscriptionExpired;
    }
    else
    {
        return StatusReasonCodeNone;
    }
}

void NucleusEntitlement::logEntitlement(NucleusEntitlementRef ent)
{
    if(ent.isNull() || ent == NucleusEntitlement::INVALID_ENTITLEMENT)
    {
        ORIGIN_LOG_DEBUG << "INVALID Nucleus Entitlement";
    }
    else
    {
        ORIGIN_LOG_DEBUG << "Entitlement [" << ent->id() << "]";
        for(auto iter = ent->properties().constBegin(); iter != ent->properties().constEnd(); ++iter) 
        {
            ORIGIN_LOG_DEBUG << "    " << iter.key() << ":" << iter.value().toString();
        }
    }
}

bool NucleusEntitlement::serialize(QDataStream& outputStream, NucleusEntitlementRef ent)
{
    if (ent && ent != NucleusEntitlement::INVALID_ENTITLEMENT)
    {
        outputStream << ent->properties();
        return outputStream.status() == QDataStream::Ok;
    }
    else
    {
        return false;
    }
}

bool NucleusEntitlement::deserialize(QDataStream& inputStream, NucleusEntitlementRef &ent)
{
    if (!inputStream.atEnd())
    {
        NucleusEntitlementRef ref(new NucleusEntitlement());

        inputStream >> ref->properties();
        if (inputStream.status() == QDataStream::Ok)
        {
            ent = ref;
            return true;
        }
    }

    return false;
}

lsx::EntitlementT NucleusEntitlement::toLSX() const
{
    lsx::EntitlementT entitlement;

    QDateTime t = terminationDate();
    t.setTimeSpec(Qt::UTC);
    QString terminationDate = t.toString(Qt::ISODate);

    t = grantDate();
    t.setTimeSpec(Qt::UTC);
    QString grantDate = t.toString(Qt::ISODate);

    entitlement.EntitlementId = QString::number(id());
    entitlement.EntitlementTag = tag();
    Read(terminationDate.toLatin1().constData(), entitlement.Expiration);
    Read(grantDate.toLatin1().constData(), entitlement.GrantDate);
    entitlement.Group = groupName();
    entitlement.ItemId = productId(); 
    entitlement.ResourceId = "";
    entitlement.Type = entitlement.EntitlementTag == "ORIGIN_DOWNLOAD" ? "ONLINE_ACCESS" : "DEFAULT";
    entitlement.UseCount = useCount();

    return entitlement;
}

void NucleusEntitlement::init()
{
    NucleusEntitlement::INVALID_ENTITLEMENT.reset(new NucleusEntitlement());
}

void NucleusEntitlement::release()
{
    NucleusEntitlement::INVALID_ENTITLEMENT.clear();
}

#ifdef _DEBUG
//#define WRITE_ENTITLEMENT_COMPARE_DEBUG_INFO 1
#endif

bool NucleusEntitlement::CompareEntitlements(const NucleusEntitlementRef& entitlementA, const NucleusEntitlementRef& entitlementB)
{
#ifdef WRITE_ENTITLEMENT_COMPARE_DEBUG_INFO
    QString suppressionReason;
    #define SET_SUPPRESSION_REASON(reason) suppressionReason = (reason)
#else
    #define SET_SUPPRESSION_REASON(reason)
#endif

    bool entAWins = true;

    const QDateTime &entAGrantDate = entitlementA->grantDate();
    const QDateTime &entBGrantDate = entitlementB->grantDate();

    bool entAIsOwned = (entitlementA->id() != 0);
    bool entBIsOwned = (entitlementB->id() != 0);

    bool entAIsSubscription = ExternalTypeSubscription == entitlementA->externalType() ||
        (ACTIVE == entitlementA->status() && StatusReasonCodeSubscriptionExpired == entitlementA->statusReasonCode());
    bool entBIsSubscription = ExternalTypeSubscription == entitlementB->externalType() ||
        (ACTIVE == entitlementB->status() && StatusReasonCodeSubscriptionExpired == entitlementB->statusReasonCode());

    bool entAIsDownload = entitlementA->tag().contains("ORIGIN_DOWNLOAD", Qt::CaseInsensitive);
    bool entBIsDownload = entitlementB->tag().contains("ORIGIN_DOWNLOAD", Qt::CaseInsensitive);

    bool entAIsPreorder = entitlementA->tag().contains("ORIGIN_PREORDER", Qt::CaseInsensitive);
    bool entBIsPreorder = entitlementB->tag().contains("ORIGIN_PREORDER", Qt::CaseInsensitive);

    OriginPermissions entAPermissions = entitlementA->originPermissions();
    OriginPermissions entBPermissions = entitlementB->originPermissions();

    if (entAIsSubscription != entBIsSubscription)
    {
        entAWins = entBIsSubscription;
        SET_SUPPRESSION_REASON("subscription precedence");
    }
    else if (entAGrantDate != entBGrantDate)
    {
        entAWins = entAGrantDate > entBGrantDate;
        SET_SUPPRESSION_REASON(QString("grant date precedence (%1 %2 %3)").arg(
            entAGrantDate.toString(), entAWins ? ">" : "<", entBGrantDate.toString()));
    }
    else if (entAIsOwned != entBIsOwned)
    {
        entAWins = entAIsOwned;
        SET_SUPPRESSION_REASON("ownership precedence");
    }
    else if (entAIsDownload != entBIsDownload)
    {
        // ORIGIN_DOWNLOAD entitlements are preferred above all others, because they are the only entitlements that will contain cdkey
        entAWins = entAIsDownload;
        SET_SUPPRESSION_REASON("origin_download precedence");
    }
    else if (entAIsPreorder != entBIsPreorder)
    {
        // Add the new entitlement only if it is not the pre-order entitlement
        entAWins = entBIsPreorder;
        SET_SUPPRESSION_REASON("non preorder precedence");
    }
    else if (entAPermissions != entBPermissions)
    {
        // Add the new entitlement only if it has the better Origin permissions.
        entAWins = (entAPermissions > entBPermissions);
        SET_SUPPRESSION_REASON(QString("originPermissions precedence (%1 %2 %3)").arg(
            QString::number(entAPermissions), entAWins ? ">" : "<", QString::number(entBPermissions)));
    }
    else if (entitlementA->cdKey().isEmpty() != entitlementB->cdKey().isEmpty())
    {
        // We want to preserve a cdkey if one is available
        entAWins = entitlementB->cdKey().isEmpty();
        SET_SUPPRESSION_REASON("cdkey precedence");
    }
    else if (entitlementA->id() != entitlementB->id())
    {
        // NOTE: This check needs to always be last.  We only want to use IDs to sort if the entitlements
        // are the same in every way that matters, and we just need a way to deterministically sort so
        // the same entitlement is always chosen as "best."

        // Consistently choose the higher entitlement id when everything else is equal
        entAWins = (entitlementA->id() > entitlementB->id());
        SET_SUPPRESSION_REASON(QString("entitlement id sort precedence (%1 %2 %3)").arg(
            QString::number(entitlementA->id()), entAWins ? ">" : "<", QString::number(entitlementB->id())));
    }
    else
    {
        // When entitlements are equal, both comparisons return false
        return false;
    }

#ifdef WRITE_ENTITLEMENT_COMPARE_DEBUG_INFO
    if (entAWins)
    {
        ORIGIN_LOG_DEBUG << entitlementA->productId() << " entitlement [" << entitlementB->id() << "] was suppressed by [" << entitlementA->id() << "] due to " << suppressionReason;
    }
    else
    {
        ORIGIN_LOG_DEBUG << entitlementA->productId() << " entitlement [" << entitlementA->id() << "] was suppressed by [" << entitlementB->id() << "] due to " << suppressionReason;
    }
#endif

    return entAWins;
}

} // namespace Publishing

} // namespace Services

} // namespace Origin
