#include "../../../include/Qt/originprogressbar.h"
#include "originprogressbarplugin.h"

#include <QtCore/QtPlugin>

OriginProgressBarPlugin::OriginProgressBarPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginProgressBarPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginProgressBarPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginProgressBarPlugin::createWidget(QWidget *parent)
{
    return new Origin::UIToolkit::OriginProgressBar(parent);
}

QString OriginProgressBarPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginProgressBar");
}

QString OriginProgressBarPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginProgressBarPlugin::icon() const
{
    return QIcon();
}

QString OriginProgressBarPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginProgressBarPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginProgressBarPlugin::isContainer() const
{
    return false;
}

QString OriginProgressBarPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginProgressBar\" name=\"originProgressBar\">\n</widget>\n");
}

QString OriginProgressBarPlugin::includeFile() const
{
    return QLatin1String("originprogressbar.h");
}

