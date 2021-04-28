#ifndef OriginBase_H
#define OriginBase_H

#include <QtWidgets/QWidget>
#include <QHash>
#include <QList>

//#include "services/debug/DebugService.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {

struct widgetDescription
{
	QString widgetClass;
    QString fontFamily;
    QString fontWeight;
    QString fontSize;
};

struct langDescription
{
    QString name;
    QHash<QString, widgetDescription*> widgetHash;
};



class UITOOLKIT_PLUGIN_API OriginUIBase
{

public:
    OriginUIBase();
    virtual ~OriginUIBase() {}

    static void changeCurrentLang(QString Lang);
	static QHash<QString,langDescription*> loadLanguages();
    static void initFonts();

protected:
     virtual void paintEvent(QPaintEvent* event);
     virtual bool event(QEvent* event);

     void setOriginElementName(const QString& element, const QString& subElement="");

     virtual QWidget* getSelf() = 0;
     virtual QString& getElementStyleSheet() = 0;

     virtual QString& getCurrentLang();

     virtual void prepForStyleReload() = 0;

     
     static void loadLangIntoStyleSheet(QString& preLangStyleSheet, 
										const QString& name, 
										const QString& subname = "");

     void removeElementFromChildList(OriginUIBase* child);


     static QString loadPrivateStyleSheet(const QString& path, 
										  const QString& name, 
										  const QString& subname = "");
     QString m_styleSheetPath;
     QString m_OriginElementName;
	 QString m_OriginSubElementName;


     static QHash<QString,langDescription*> m_langDescHash;
     static QString m_currentLang;


     //used to keep track of children so signalish can be sent for lang reload
     static QList<OriginUIBase*> m_children;


};

    }
}

#endif

