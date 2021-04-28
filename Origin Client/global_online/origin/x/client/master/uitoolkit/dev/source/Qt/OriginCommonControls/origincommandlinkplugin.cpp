#include "../../../include/Qt/origincommandlink.h"
#include "origincommandlinkplugin.h"

#include <QtCore/QtPlugin>

OriginCommandLinkPlugin::OriginCommandLinkPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginCommandLinkPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginCommandLinkPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginCommandLinkPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginCommandLink* w = new Origin::UIToolkit::OriginCommandLink(parent);
    w->setText(name());
    w->setDescription(name());
    return w;
}

QString OriginCommandLinkPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginCommandLink");
}

QString OriginCommandLinkPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginCommandLinkPlugin::icon() const
{
    return QIcon();
}

QString OriginCommandLinkPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginCommandLinkPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginCommandLinkPlugin::isContainer() const
{
    return false;
}

QString OriginCommandLinkPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginCommandLink\" name=\"originCommandLink\">\n</widget>\n");
}

QString OriginCommandLinkPlugin::includeFile() const
{
    return QLatin1String("origincommandlink.h");
}

