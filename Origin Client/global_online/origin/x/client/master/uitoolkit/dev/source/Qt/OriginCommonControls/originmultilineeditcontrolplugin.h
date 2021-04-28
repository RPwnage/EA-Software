#ifndef ORIGINMULTILINEEDITCONTROLPLUGIN_H

#define ORIGINMULTILINEEDITCONTROLPLUGIN_H



#include <QtDesigner/QDesignerCustomWidgetInterface>



class OriginMultiLineEditControlPlugin : public QObject, public QDesignerCustomWidgetInterface

{

    Q_OBJECT

    Q_PLUGIN_METADATA(IID QDesignerCustomWidgetInterface_iid FILE "originmultilineeditcontrolplugin.json")

    Q_INTERFACES(QDesignerCustomWidgetInterface)



public:

    OriginMultiLineEditControlPlugin(QObject *parent = 0);



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



private:

    bool m_initialized;

};



#endif

