#include "../../../include/Qt/originsurveywidget.h"
#include "originsurveywidgetplugin.h"

#include <QtCore/QtPlugin>

OriginSurveyWidgetPlugin::OriginSurveyWidgetPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void OriginSurveyWidgetPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginSurveyWidgetPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget* OriginSurveyWidgetPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginSurveyWidget* w = new Origin::UIToolkit::OriginSurveyWidget(parent);
    return w;
}

QString OriginSurveyWidgetPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginSurveyWidget");
}

QString OriginSurveyWidgetPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginSurveyWidgetPlugin::icon() const
{
    return QIcon();
}

QString OriginSurveyWidgetPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginSurveyWidgetPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginSurveyWidgetPlugin::isContainer() const
{
    return false;
}

QString OriginSurveyWidgetPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginSurveyWidget\" name=\"originSurveyWidget\">\n</widget>\n");
}

QString OriginSurveyWidgetPlugin::includeFile() const
{
    return QLatin1String("originsurveywidget.h");
}

