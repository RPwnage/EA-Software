#include "../../../include/Qt/origintooltip.h"
#include "origintooltipplugin.h"

#include <QtCore/QtPlugin>

OriginTooltipPlugin::OriginTooltipPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginTooltipPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginTooltipPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget* OriginTooltipPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginTooltip* w = new Origin::UIToolkit::OriginTooltip(parent);
    return w;
}

QString OriginTooltipPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginTooltip");
}

QString OriginTooltipPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginTooltipPlugin::icon() const
{
    return QIcon();
}

QString OriginTooltipPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginTooltipPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginTooltipPlugin::isContainer() const
{
    return false;
}

QString OriginTooltipPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginTooltip\" name=\"originTooltip\">\n</widget>\n");
}

QString OriginTooltipPlugin::includeFile() const
{
    return QLatin1String("origintooltip.h");
}

