#include "originvalidationcontainerplugin.h"
#include "../../../include/Qt/origincheckbutton.h"

#include <QtCore/QtPlugin>
#include <QtCore/QEvent>

OriginValidationContainerPlugin::OriginValidationContainerPlugin(QObject *parent)
: QObject(parent)
, m_initialized(false)
, m_widget(NULL)
{
}

void OriginValidationContainerPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool OriginValidationContainerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *OriginValidationContainerPlugin::createWidget(QWidget *parent)
{
    Origin::UIToolkit::OriginValidationContainer* w = new Origin::UIToolkit::OriginValidationContainer(parent);
 //   w->addButton("btn1");
    m_widget = w;
    return w;
//    return reinterpret_cast<QWidget*> (w);
}

QString OriginValidationContainerPlugin::name() const
{
    return QLatin1String("Origin::UIToolkit::OriginValidationContainer");
}

QString OriginValidationContainerPlugin::group() const
{
    return QLatin1String("");
}

QIcon OriginValidationContainerPlugin::icon() const
{
    return QIcon();
}

QString OriginValidationContainerPlugin::toolTip() const
{
    return QLatin1String("");
}

QString OriginValidationContainerPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool OriginValidationContainerPlugin::isContainer() const
{
    return true;
}

QString OriginValidationContainerPlugin::domXml() const
{
    return QLatin1String("<widget class=\"Origin::UIToolkit::OriginValidationContainer\" name=\"originValidationContainer\">\n</widget>\n");
}

QString OriginValidationContainerPlugin::includeFile() const
{
    return QLatin1String("originvalidationcontainer.h");
}

bool OriginValidationContainerPlugin::event(QEvent* e)
{

    if (e->type() == QEvent::ChildAdded)
    {
        //QChildEvent* ce = (QChildEvent *)e;
        // todo....

    }

    return QObject::event(e);
}

