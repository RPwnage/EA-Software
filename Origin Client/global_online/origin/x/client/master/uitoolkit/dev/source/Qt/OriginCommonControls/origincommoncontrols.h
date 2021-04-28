#ifndef ORIGINCOMMONCONTROLS_H
#define ORIGINCOMMONCONTROLS_H

#include <QtDesigner/QtDesigner>
#include <QtCore/qplugin.h>

class OriginCommonControls : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "origincommoncontrolsplugin" FILE "origincommoncontrols.json")
    Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
    explicit OriginCommonControls(QObject *parent = 0);

    virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;

private:
    QList<QDesignerCustomWidgetInterface*> m_widgets;

    void initFonts();
};

#endif
