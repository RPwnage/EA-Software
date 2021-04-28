#include "../../../include/Qt/originradiobutton.h"
#include "originradiobuttonplugin.h"

#include <QtCore/QtPlugin>

OriginRadioButtonPlugin::OriginRadioButtonPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginRadioButtonPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginRadioButtonPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginRadioButtonPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginRadioButton* w = new Origin::UIToolkit::OriginRadioButton(parent);
    w->setText(name());
    return w;
}

QString OriginRadioButtonPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginRadioButton");
}

QString OriginRadioButtonPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginRadioButtonPlugin::icon() const
{
    return QIcon();
}

QString OriginRadioButtonPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginRadioButtonPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginRadioButtonPlugin::isContainer() const
{
    return false;
}

QString OriginRadioButtonPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginRadioButton\" name=\"originRadioButton\">\n</widget>\n");
}

QString OriginRadioButtonPlugin::includeFile() const
{
    return QLatin1String("originradiobutton.h");
}

