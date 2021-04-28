#include "../../../include/Qt/originiconbutton.h"
#include "originiconbuttonplugin.h"

#include <QtCore/QtPlugin>

OriginIconButtonPlugin::OriginIconButtonPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginIconButtonPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginIconButtonPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginIconButtonPlugin::createWidget(QWidget *parent)
{
    return new Origin::UIToolkit::OriginIconButton(parent);
}

QString OriginIconButtonPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginIconButton");
}

QString OriginIconButtonPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginIconButtonPlugin::icon() const
{
    return QIcon();
}

QString OriginIconButtonPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginIconButtonPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginIconButtonPlugin::isContainer() const
{
    return false;
}

QString OriginIconButtonPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginIconButton\" name=\"originIconButton\">\n</widget>\n");
}

QString OriginIconButtonPlugin::includeFile() const
{
    return QLatin1String("originiconbutton.h");
}

