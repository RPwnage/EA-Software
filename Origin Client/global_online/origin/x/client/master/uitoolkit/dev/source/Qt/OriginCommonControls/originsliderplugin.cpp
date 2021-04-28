#include "../../../include/Qt/originslider.h"
#include "originsliderplugin.h"

#include <QtCore/QtPlugin>

OriginSliderPlugin::OriginSliderPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginSliderPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginSliderPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginSliderPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginSlider* w = new Origin::UIToolkit::OriginSlider(parent);
    // w->setText(name());
    return w;
}

QString OriginSliderPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginSlider");
}

QString OriginSliderPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginSliderPlugin::icon() const
{
    return QIcon();
}

QString OriginSliderPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginSliderPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginSliderPlugin::isContainer() const
{
    return false;
}

QString OriginSliderPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginSlider\" name=\"originSlider\">\n</widget>\n");
}

QString OriginSliderPlugin::includeFile() const
{
    return QLatin1String("originslider.h");
}

