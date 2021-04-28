#include "../../../include/Qt/originmultilineedit.h"
#include "originmultilineeditplugin.h"

#include <QtCore/QtPlugin>

OriginMultiLineEditPlugin::OriginMultiLineEditPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginMultiLineEditPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginMultiLineEditPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginMultiLineEditPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginMultiLineEdit* w = new Origin::UIToolkit::OriginMultiLineEdit(parent);
    w->setHtml(name());
    return w;
}

QString OriginMultiLineEditPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginMultiLineEdit");
}

QString OriginMultiLineEditPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginMultiLineEditPlugin::icon() const
{
    return QIcon();
}

QString OriginMultiLineEditPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginMultiLineEditPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginMultiLineEditPlugin::isContainer() const
{
    return false;
}

QString OriginMultiLineEditPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginMultiLineEdit\" name=\"originMultiLineEdit\">\n</widget>\n");
}

QString OriginMultiLineEditPlugin::includeFile() const
{
    return QLatin1String("originmultilineedit.h");
}

