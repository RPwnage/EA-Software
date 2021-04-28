#include "../../../include/Qt/originsinglelineedit.h"
#include "originsinglelineeditplugin.h"

#include <QtCore/QtPlugin>

OriginSingleLineEditPlugin::OriginSingleLineEditPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginSingleLineEditPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginSingleLineEditPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginSingleLineEditPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginSingleLineEdit* w = new Origin::UIToolkit::OriginSingleLineEdit(parent);
    w->setText(name());
    return w;
}

QString OriginSingleLineEditPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginSingleLineEdit");
}

QString OriginSingleLineEditPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginSingleLineEditPlugin::icon() const
{
    return QIcon();
}

QString OriginSingleLineEditPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginSingleLineEditPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginSingleLineEditPlugin::isContainer() const
{
    return false;
}

QString OriginSingleLineEditPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginSingleLineEdit\" name=\"originSingleLineEdit\">\n</widget>\n");
}

QString OriginSingleLineEditPlugin::includeFile() const
{
    return QLatin1String("originsinglelineedit.h");
}

