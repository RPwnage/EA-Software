//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#ifndef NUCLEUSENTITLEMENT_H
#define NUCLEUSENTITLEMENT_H

#include "services/common/Property.h"

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QDataStream>
#include <QJsonObject>
#include <QSharedPointer>

namespace lsx
{
    struct EntitlementT;
}

namespace Origin
{

namespace Services
{

namespace Publishing
{

enum NucleusEntitlementStatus
{
    ACTIVE,
    DELETED,
    DISABLED,
    UNKNOWN
};

enum OriginPermissions
{
    OriginPermissionsNormal,
    OriginPermissions1102,
    OriginPermissions1103
};

class NucleusEntitlement;
typedef QSharedPointer<NucleusEntitlement> NucleusEntitlementRef;
typedef QList<NucleusEntitlementRef> NucleusEntitlementList;

class NucleusEntitlement : public QObject
{
    Q_OBJECT

    DECLARE_PROPERTY_CONTAINER(NucleusEntitlement)

public:
    enum ExternalType
    {
        ExternalTypeNone,
        ExternalTypeSubscription,
    };

    enum StatusReasonCode
    {
        StatusReasonCodeNone,
        StatusReasonCodeSubscriptionExpired,
    };

public:
    static NucleusEntitlementRef INVALID_ENTITLEMENT;

    static void init();
    static void release();

    static bool parseJson(const QByteArray& payload, NucleusEntitlementList& entitlementList);
    static NucleusEntitlementRef parseJsonObject(const QJsonObject& json);

    static bool serialize(QDataStream& outputStream, NucleusEntitlementRef ent);
    static bool deserialize(QDataStream& inputStream, NucleusEntitlementRef &ent);

    static void logEntitlement(NucleusEntitlementRef ent);

    // Compare signature for stable_sort
    static bool CompareEntitlements(const NucleusEntitlementRef& entitlementA, const NucleusEntitlementRef& entitlementB);

    NucleusEntitlement(const NucleusEntitlement& rhs);

    DECLARE_PROPERTY_ULONGLONG(id, 0, setId);
    DECLARE_PROPERTY_STRING(source, "", setSource);
    DECLARE_PROPERTY_STRING(tag, "", setTag);
    DECLARE_PROPERTY_STRING(type, "", setType);
    DECLARE_PROPERTY_INT(version, 0, setVersion);

    DECLARE_PROPERTY_DATETIME(grantDate, QDATESTRING_PAST_DEFAULTVALUE, QDATESTRING_FORMAT, setGrantDate);

    DECLARE_PROPERTY_STRING(groupName, "", setGroupName);
    DECLARE_PROPERTY_STRING(productCatalog, "", setProductCatalog);
    DECLARE_PROPERTY_STRING(productId, "", setProductId);
    DECLARE_PROPERTY_STRING(projectId, "", setProjectId);
    DECLARE_PROPERTY_ENUM(NucleusEntitlementStatus, status, ACTIVE, setStatus);

    DECLARE_PROPERTY_DATETIME(terminationDate, QDATESTRING_FUTURE_DEFAULTVALUE, QDATESTRING_FORMAT, setTerminationDate);

    DECLARE_PROPERTY_INT(useCount, -1, setUseCount);
    DECLARE_PROPERTY_BOOL(isConsumable, false, setConsumable);

    DECLARE_PROPERTY_ENUM(OriginPermissions, originPermissions, OriginPermissionsNormal, setOriginPermissions);

    DECLARE_PROPERTY_DATETIME(updatedDate, QDATESTRING_FUTURE_DEFAULTVALUE, QDATESTRING_FORMAT, setUpdatedDate);

    // ORPUB-139
    // These attributes are only to be used during library initialization as hints.  Assuming A suppresses B, the goal is 
    // to prevent B from being exposed to the other modules of the client before getting immediately suppressed by A.
    // If you need these values, see the CatalogDefinition/ContentConfiguration equivalents.
    DECLARE_PROPERTY_STRINGLIST(suppressedOfferIds, setSuppressedOfferIds);
    DECLARE_PROPERTY_STRINGLIST(suppressedBy, setSuppressedBy);

    DECLARE_PROPERTY_STRING(cdKey, "", setCdKey);

    DECLARE_PROPERTY_ENUM(ExternalType, externalType, ExternalTypeNone, setExternalType);
    DECLARE_PROPERTY_STRING(externalId, "", setExternalId);

    DECLARE_PROPERTY_ENUM(StatusReasonCode, statusReasonCode, StatusReasonCodeNone, setStatusReasonCode);

    bool isValid() const { return id() != 0; } 
    bool isPreorder() const { return (tag().compare("ORIGIN_PREORDER", Qt::CaseInsensitive) == 0); }

    bool operator==(const NucleusEntitlement &rhs) const;
    NucleusEntitlement& operator=(const NucleusEntitlement&);
    void updateEntitlement(const NucleusEntitlementRef& other);

    lsx::EntitlementT toLSX() const;

private:
    static NucleusEntitlementStatus convertEntitlementStatus(const QString &status);
    static ExternalType convertExternalType(const QString &type);
    static StatusReasonCode convertStatusReasonCode(const QString &code);

signals:
    void entitlementChanged();
};

} // namespace Publishing

} // namespace Services

} // namespace Origin

#endif
