#ifndef ORIGINCHECKBUTTONGROUPPLUGIN_H
#define ORIGINCHECKBUTTONGROUPPLUGIN_H

#include <QtDesigner/QDesignerCustomWidgetInterface>
#include "../../../include/Qt/originvalidationcontainer.h"

class OriginValidationContainerPlugin : public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
    OriginValidationContainerPlugin(QObject *parent = 0);

    bool isContainer() const;
    bool isInitialized() const;
    QIcon icon() const;
    QString domXml() const;
    QString group() const;
    QString includeFile() const;
    QString name() const;
    QString toolTip() const;
    QString whatsThis() const;
    QWidget *createWidget(QWidget *parent);
    void initialize(QDesignerFormEditorInterface *core);

protected:
    bool event(QEvent *);

private:
    bool m_initialized;
    Origin::UIToolkit::OriginValidationContainer* m_widget;
};
#endif // ORIGINCHECKBUTTONGROUPPLUGIN_H
