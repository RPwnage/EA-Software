#include "../../../include/Qt/origindivider.h"
#include "origindividerplugin.h"

#include <QtCore/QtPlugin>

OriginDividerPlugin::OriginDividerPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginDividerPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginDividerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginDividerPlugin::createWidget(QWidget *parent)
{
    return new Origin::UIToolkit::OriginDivider(parent);
}

QString OriginDividerPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginDivider");
}

QString OriginDividerPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginDividerPlugin::icon() const
{
    return QIcon();
}

QString OriginDividerPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginDividerPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginDividerPlugin::isContainer() const
{
    return false;
}

QString OriginDividerPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginDivider\" name=\"originDivider\">\n</widget>\n");
}

QString OriginDividerPlugin::includeFile() const
{
    return QLatin1String("origindivider.h");
}

