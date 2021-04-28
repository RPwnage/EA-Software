#include "../../../include/Qt/origintabwidget.h"
#include "OriginTabWidgetPlugin.h"

#include <QtCore/QtPlugin>

OriginTabWidgetPlugin::OriginTabWidgetPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginTabWidgetPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginTabWidgetPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginTabWidgetPlugin::createWidget(QWidget *parent)
{
    return new Origin::UIToolkit::OriginTabWidget(parent);
}

QString OriginTabWidgetPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginTabWidget");
}

QString OriginTabWidgetPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginTabWidgetPlugin::icon() const
{
    return QIcon();
}

QString OriginTabWidgetPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginTabWidgetPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginTabWidgetPlugin::isContainer() const
{
    return false;
}

QString OriginTabWidgetPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginTabWidget\" name=\"originTabWidget\">\n</widget>\n");
}

QString OriginTabWidgetPlugin::includeFile() const
{
    return QLatin1String("origintabwidget.h");
}

