#include <QUrl>

#include "NativeInterfaceRegistry.h"
#include "NativeInterfaceFactory.h"

namespace
{
    using namespace WebWidget;

    // Keyed by feature URI
    typedef QHash<QString, NativeInterface> SharedInterfaceHash;
    // Keyed by feature URI
    typedef QHash<QString, NativeInterfaceFactory*> InterfaceFactoryHash;

    SharedInterfaceHash SharedInterfaces;
    InterfaceFactoryHash InterfaceFactories;
}

namespace WebWidget
{
    QString NativeInterfaceRegistry::defaultFeatureUri(const QByteArray &name)
    {
        // They shouldn't use names that need to be encoded but be robust if you do
        QByteArray encodedName(QUrl::toPercentEncoding(name));

        return "http://widgets.dm.origin.com/features/interfaces/" + encodedName;
    }

    QString NativeInterfaceRegistry::registerSharedInterface(const NativeInterface &inter, const QString &featureUri)
    {
        QString interfaceFeatureUri(featureUri);

        if (interfaceFeatureUri.isNull())
        {
            // Build a default
            interfaceFeatureUri = NativeInterfaceRegistry::defaultFeatureUri(inter.name());
        }

        SharedInterfaces.insert(interfaceFeatureUri, inter);

        return interfaceFeatureUri;
    }

    QString NativeInterfaceRegistry::registerInterfaceFactory(const QString &featureUri, NativeInterfaceFactory *factory)
    {
        InterfaceFactories.insert(featureUri, factory);
        return featureUri;
    }

    QList<NativeInterface> NativeInterfaceRegistry::interfacesForFeatureRequests(QWebFrame *frame, QList<FeatureRequest> &featureRequests)
    {
        QList<NativeInterface> nativeInterfaces;
        QMutableListIterator<FeatureRequest> it(featureRequests);

        while(it.hasNext())
        {
            const FeatureRequest &request = it.next();

            if (SharedInterfaces.contains(request.uri()))
            {
                // Clear the hasOwnership() flag using clone() so the caller won't delete our shared interface
                nativeInterfaces.append(SharedInterfaces[request.uri()].clone());
                it.remove();
            }
            else if (InterfaceFactories.contains(request.uri()))
            {
                // Build the interface using the interface factory
                NativeInterfaceFactory *factory = InterfaceFactories[request.uri()];
                NativeInterface interface(factory->createNativeInterface(frame, request.parameters()));

                if (!interface.isNull())
                {
                    // Preserve the hasOwnership() flag as this interface isn't going to be reused by us
                    nativeInterfaces.append(interface);
                    it.remove();
                }
            }
        }

        return nativeInterfaces;
    }

    void NativeInterfaceRegistry::unregisterInterface(const QString &featureUri)
    {
        // Remove the shared interface
        NativeInterface sharedInterface = SharedInterfaces.take(featureUri);

        if (!sharedInterface.isNull() && sharedInterface.hasOwnership())
        {
            // Delete the bridge object as we have ownership
            delete sharedInterface.bridgeObject();
        }

        // Remove the interface factory
        InterfaceFactories.remove(featureUri);
    }
}
