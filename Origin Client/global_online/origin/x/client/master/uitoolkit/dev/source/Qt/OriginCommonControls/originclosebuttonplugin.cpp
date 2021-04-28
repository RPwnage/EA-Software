#include "../../../include/Qt/originclosebutton.h"
#include "originclosebuttonplugin.h"

#include <QtCore/QtPlugin>

OriginCloseButtonPlugin::OriginCloseButtonPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginCloseButtonPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginCloseButtonPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginCloseButtonPlugin::createWidget(QWidget *parent)
{
    return new Origin::UIToolkit::OriginCloseButton(parent);
}

QString OriginCloseButtonPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginCloseButton");
}

QString OriginCloseButtonPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginCloseButtonPlugin::icon() const
{
    return QIcon();
}

QString OriginCloseButtonPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginCloseButtonPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginCloseButtonPlugin::isContainer() const
{
    return false;
}

QString OriginCloseButtonPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginCloseButton\" name=\"originCloseButton\">\n</widget>\n");
}

QString OriginCloseButtonPlugin::includeFile() const
{
    return QLatin1String("originclosebutton.h");
}

