#include "../../../include/Qt/originbanner.h"
#include "OriginBannerPlugin.h"

#include <QtCore/QtPlugin>

OriginBannerPlugin::OriginBannerPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginBannerPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginBannerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget* OriginBannerPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginBanner* w = new Origin::UIToolkit::OriginBanner(parent);
    return w;
}

QString OriginBannerPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginBanner");
}

QString OriginBannerPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginBannerPlugin::icon() const
{
    return QIcon();
}

QString OriginBannerPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginBannerPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginBannerPlugin::isContainer() const
{
    return false;
}

QString OriginBannerPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginBanner\" name=\"originBanner\">\n</widget>\n");
}

QString OriginBannerPlugin::includeFile() const
{
    return QLatin1String("originbanner.h");
}

