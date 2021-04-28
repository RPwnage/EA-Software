#include "../../../include/Qt/originpushbutton.h"
#include "originpushbuttonplugin.h"

#include <QtCore/QtPlugin>

OriginPushButtonPlugin::OriginPushButtonPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginPushButtonPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginPushButtonPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginPushButtonPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginPushButton* w = new Origin::UIToolkit::OriginPushButton(parent);
    w->setText(name());
    return w;
}

QString OriginPushButtonPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginPushButton");
}

QString OriginPushButtonPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginPushButtonPlugin::icon() const
{
    return QIcon();
}

QString OriginPushButtonPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginPushButtonPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginPushButtonPlugin::isContainer() const
{
    return false;
}

QString OriginPushButtonPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginPushButton\" name=\"originPushButton\">\n</widget>\n");
}

QString OriginPushButtonPlugin::includeFile() const
{
    return QLatin1String("originpushbutton.h");
}

