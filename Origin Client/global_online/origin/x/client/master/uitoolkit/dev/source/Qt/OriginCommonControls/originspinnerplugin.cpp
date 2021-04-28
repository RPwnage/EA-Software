#include "../../../include/Qt/originspinner.h"
#include "originspinnerplugin.h"

#include <QtCore/QtPlugin>

OriginSpinnerPlugin::OriginSpinnerPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginSpinnerPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginSpinnerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginSpinnerPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginSpinner* w = new Origin::UIToolkit::OriginSpinner(parent);
    // w->setText(name());
    return w;
}

QString OriginSpinnerPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginSpinner");
}

QString OriginSpinnerPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginSpinnerPlugin::icon() const
{
    return QIcon();
}

QString OriginSpinnerPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginSpinnerPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginSpinnerPlugin::isContainer() const
{
    return false;
}

QString OriginSpinnerPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginSpinner\" name=\"originSpinner\">\n</widget>\n");
}

QString OriginSpinnerPlugin::includeFile() const
{
    return QLatin1String("originspinner.h");
}

