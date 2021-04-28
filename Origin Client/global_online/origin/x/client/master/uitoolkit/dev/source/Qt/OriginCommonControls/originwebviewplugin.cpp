#include "../../../include/Qt/originwebview.h"
#include "originwebviewplugin.h"

#include <QtCore/QtPlugin>

OriginWebViewPlugin::OriginWebViewPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginWebViewPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginWebViewPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginWebViewPlugin::createWidget(QWidget *parent)
{
    return new Origin::UIToolkit::OriginWebView(parent);
}

QString OriginWebViewPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginWebView");
}

QString OriginWebViewPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginWebViewPlugin::icon() const
{
    return QIcon();
}

QString OriginWebViewPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginWebViewPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginWebViewPlugin::isContainer() const
{
    return false;
}

QString OriginWebViewPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginWebView\" name=\"originWebView\">\n</widget>\n");
}

QString OriginWebViewPlugin::includeFile() const
{
    return QLatin1String("originwebview.h");
}

