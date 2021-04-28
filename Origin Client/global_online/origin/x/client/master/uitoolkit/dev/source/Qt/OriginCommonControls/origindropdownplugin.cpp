//#pragma warning(disable:4005) // warning C4005: 'INT8_MIN' : macro redefinition
#include "../../../include/Qt/origindropdown.h"
#include "origindropdownplugin.h"

#include <QtCore/QtPlugin>

OriginDropdownPlugin::OriginDropdownPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginDropdownPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginDropdownPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginDropdownPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginDropdown* w = new Origin::UIToolkit::OriginDropdown(parent);
//    w->setText(name());
    return w;
}

QString OriginDropdownPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginDropdown");
}

QString OriginDropdownPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginDropdownPlugin::icon() const
{
    return QIcon();
}

QString OriginDropdownPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginDropdownPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginDropdownPlugin::isContainer() const
{
    return false;
}

QString OriginDropdownPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginDropdown\" name=\"originDropdown\">\n</widget>\n");
}

QString OriginDropdownPlugin::includeFile() const
{
    return QLatin1String("origindropdown.h");
}

