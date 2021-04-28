#include "../../../include/Qt/originlabel.h"
#include "originlabelplugin.h"

#include <QtCore/QtPlugin>

OriginLabelPlugin::OriginLabelPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginLabelPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginLabelPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginLabelPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginLabel* w = new Origin::UIToolkit::OriginLabel(parent);
    w->setText(name());
    return w;
}

QString OriginLabelPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginLabel");
}

QString OriginLabelPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginLabelPlugin::icon() const
{
    return QIcon();
}

QString OriginLabelPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginLabelPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginLabelPlugin::isContainer() const
{
    return false;
}

QString OriginLabelPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginLabel\" name=\"originLabel\">\n</widget>\n");
}

QString OriginLabelPlugin::includeFile() const
{
    return QLatin1String("originlabel.h");
}

