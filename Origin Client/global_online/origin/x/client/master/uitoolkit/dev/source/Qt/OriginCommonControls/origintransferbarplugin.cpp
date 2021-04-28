#include "../../../include/Qt/origintransferbar.h"
#include "origintransferbarplugin.h"

#include <QtCore/QtPlugin>

OriginTransferBarPlugin::OriginTransferBarPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginTransferBarPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginTransferBarPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginTransferBarPlugin::createWidget(QWidget *parent)
{
    return new Origin::UIToolkit::OriginTransferBar(parent);
}

QString OriginTransferBarPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginTransferBar");
}

QString OriginTransferBarPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginTransferBarPlugin::icon() const
{
    return QIcon();
}

QString OriginTransferBarPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginTransferBarPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginTransferBarPlugin::isContainer() const
{
    return false;
}

QString OriginTransferBarPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginTransferBar\" name=\"originTransferBar\">\n</widget>\n");
}

QString OriginTransferBarPlugin::includeFile() const
{
    return QLatin1String("origintransferbar.h");
}

