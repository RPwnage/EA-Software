#include "../../../include/Qt/origincheckbutton.h"
#include "origincheckbuttonplugin.h"

#include <QtCore/QtPlugin>

OriginCheckButtonPlugin::OriginCheckButtonPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginCheckButtonPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginCheckButtonPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginCheckButtonPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginCheckButton* w = new Origin::UIToolkit::OriginCheckButton(parent);
    w->setText(name());
    return dynamic_cast<QWidget*> (w);
}

QString OriginCheckButtonPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginCheckButton");
}

QString OriginCheckButtonPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginCheckButtonPlugin::icon() const
{
    return QIcon();
}

QString OriginCheckButtonPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginCheckButtonPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginCheckButtonPlugin::isContainer() const
{
    return false;
}

QString OriginCheckButtonPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginCheckButton\" name=\"originCheckButton\">\n</widget>\n");
}

QString OriginCheckButtonPlugin::includeFile() const
{
    return QLatin1String("origincheckbutton.h");
}

