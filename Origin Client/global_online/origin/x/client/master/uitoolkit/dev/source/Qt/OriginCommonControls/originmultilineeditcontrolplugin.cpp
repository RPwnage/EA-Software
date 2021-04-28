#include "originmultilineeditcontrol.h"
#include "originmultilineeditcontrolplugin.h"

#include <QtCore/QtPlugin>

OriginMultiLineEditControlPlugin::OriginMultiLineEditControlPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginMultiLineEditControlPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginMultiLineEditControlPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginMultiLineEditControlPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginMultiLineEditControl* w = new Origin::UIToolkit::OriginMultiLineEditControl(parent);
    w->setHtml(name());
    return w;
}

QString OriginMultiLineEditControlPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginMultiLineEditControl");
}

QString OriginMultiLineEditControlPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginMultiLineEditControlPlugin::icon() const
{
    return QIcon();
}

QString OriginMultiLineEditControlPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginMultiLineEditControlPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginMultiLineEditControlPlugin::isContainer() const
{
    return false;
}

QString OriginMultiLineEditControlPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginMultiLineEditControl\" name=\"originMultiLineEditControl\">\n</widget>\n");
}

QString OriginMultiLineEditControlPlugin::includeFile() const
{
    return QLatin1String("originmultilineeditcontrol.h");
}

