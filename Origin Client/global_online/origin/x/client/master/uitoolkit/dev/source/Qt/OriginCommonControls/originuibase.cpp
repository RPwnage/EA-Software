#include <QtWidgets>
#include <QCheckBox>
#include "originuiBase.h"
#include <typeinfo>
#include <QtXml>
#include <QXmlStreamReader>
#include <QList>

namespace Origin {
    namespace UIToolkit {
QHash<QString,langDescription*> OriginUIBase::m_langDescHash;

static const QString defaultLang = "en_US";
QString OriginUIBase::m_currentLang = defaultLang;

QList<OriginUIBase*> OriginUIBase::m_children;


OriginUIBase::OriginUIBase()
{
}

//static QHash<QString,langDescription*> m_langDescHash;
QHash<QString,langDescription*> OriginUIBase::loadLanguages()
{
    static QHash<QString,langDescription*> langDescHash;
    if (langDescHash.size() ==0 )
    {
        //load lang file and open for reading in xmlReader
        QFile langFile(":/OriginCommonControls/LangDescription/langdescription.xml");
        Q_ASSERT (langFile.open(QFile::ReadOnly | QFile::Text));
        QDomDocument langXMLReader;
        langXMLReader.setContent(&langFile);
        langFile.close();

        //initial setup inside XML file
        QDomElement root = langXMLReader.documentElement();
        Q_ASSERT (root.tagName() == "langDesc");


        QDomNode langNode = root.firstChild();
        while (!langNode.isNull())
        {
            QDomElement langElement = langNode.toElement();
            if (!langElement.isNull() && langElement.tagName() == "lang")
            {
                langDescription* newLangElement = new langDescription;
                newLangElement->name = langElement.attribute("name");

                QDomNode widgetNode = langElement.firstChild();
                while (!widgetNode.isNull())
                {
                    QDomElement widgetElement = widgetNode.toElement();
                    if (!widgetElement.isNull() && widgetElement.tagName() == "widget")
                    {
                        widgetDescription* newWidgetElement = new widgetDescription;
                        newWidgetElement->widgetClass = widgetElement.attribute("class");
                        newWidgetElement->fontFamily = widgetElement.attribute("font-family");
                        newWidgetElement->fontSize = widgetElement.attribute("font-size");
                        newWidgetElement->fontWeight = widgetElement.attribute("font-weight");
                        newLangElement->widgetHash.insert(newWidgetElement->widgetClass, newWidgetElement);

                    }
                    widgetNode = widgetNode.nextSibling();
                }

                langDescHash.insert(newLangElement->name, newLangElement);

            }
            langNode = langNode.nextSibling();
        }

    }
    return langDescHash;
}


void OriginUIBase::loadLangIntoStyleSheet(QString& preLangStyleSheet, const QString& name, const QString& subname)
{
    const QString widgetClass = subname.length() ? (QString("%1_%2").arg(name).arg(subname)) : name;
    langDescription* curLang = m_langDescHash[m_currentLang];
    langDescription* enus_Lang = m_langDescHash[defaultLang];

    // If the current language doesn't have a widget style override, use English - if English has one.
    widgetDescription* desc = NULL;
    if(curLang && curLang->widgetHash[widgetClass])
        desc = curLang->widgetHash[widgetClass];
    else if(enus_Lang && enus_Lang->widgetHash[widgetClass])
        desc = enus_Lang->widgetHash[widgetClass];

    if (desc)
    {
        preLangStyleSheet = preLangStyleSheet.replace("__FONT_FAMILY__", desc->fontFamily);
        preLangStyleSheet = preLangStyleSheet.replace("__FONT_SIZE__", desc->fontSize);
        preLangStyleSheet = preLangStyleSheet.replace("__FONT_WEIGHT__", desc->fontWeight);
    }
}



QString OriginUIBase::loadPrivateStyleSheet(const QString& path, const QString& name, const QString& subname)
{
	QString stylesheet = "";
	QFile inputFile(path);
	if(inputFile.open(QIODevice::ReadOnly))
	{
		QTextStream in(&inputFile);
		stylesheet = (in.readAll());
		inputFile.close();
	}
	loadLangIntoStyleSheet(stylesheet, name, subname);
	return stylesheet;
}


bool OriginUIBase::event(QEvent* event)
{
    if ((event->type() == QEvent::Polish) || (event->type() == QEvent::LanguageChange))
    {
        getSelf()->setStyleSheet((this->getElementStyleSheet()));
    }
    return false;


}

void OriginUIBase::paintEvent(QPaintEvent * /*event*/ )
{
    QStyleOption styleOption;
    styleOption.init(getSelf());
    QPainter painter(getSelf());
    if(getSelf() && getSelf()->style())
        getSelf()->style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, getSelf());
}

void OriginUIBase::setOriginElementName(const QString& element, const QString& subElement/*=""*/)
{
    m_OriginElementName = element;
    m_styleSheetPath = QString(":/OriginCommonControls/%1/style.qss").arg(m_OriginElementName);

    m_OriginSubElementName = subElement;
}


QString& OriginUIBase::getCurrentLang()
{
    return m_currentLang;
}


void OriginUIBase::changeCurrentLang(QString newLang)
{
	m_langDescHash = loadLanguages();
    if (!m_langDescHash.contains(newLang))
        newLang = defaultLang; // default to en_US

    if ( m_langDescHash.contains(newLang) )
    {
        m_currentLang = newLang;
        for ( int loop = 0; loop < m_children.size(); loop++)
        {

            if(  m_children[loop])
            {
                m_children[loop]->prepForStyleReload();
            }
        }
    }
}


void OriginUIBase::removeElementFromChildList(OriginUIBase* child)
{
    m_children.removeAll(child);
}


static const char* fontResourcePaths[] = 
{
    ":/OriginCommonControls/Fonts/OpenSans-Bold.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-BoldItalic.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-ExtraBold.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-ExtraBoldItalic.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-Italic.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-Light.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-LightItalic.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-Regular.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-Semibold.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-SemiboldItalic.ttf",
    ":/OriginCommonControls/Fonts/ApexNew-Bold.otf",                    // "Apex New Bold"
    ":/OriginCommonControls/Fonts/ApexNew-BoldItalic.otf",              // "Apex New Bold Italic"
    ":/OriginCommonControls/Fonts/ApexNew-Medium.otf",                  // "Apex New Medium"
    ":/OriginCommonControls/Fonts/ApexNew-MediumItalic.otf",            // "Apex New Medium Italic"
    ":/OriginCommonControls/Fonts/DFG-HeiSeiGothic-W5.ttf",             // "DFGHSGothic-W5" (Japanese)
    ":/OriginCommonControls/Fonts/Kdnrm.ttf",                           // "AsiaDNR-M"      (Korean)
    ":/OriginCommonControls/Fonts/KlavikaBasic-Bold.otf",               // "Klavika Basic Bold"
    ":/OriginCommonControls/Fonts/KlavikaBasic-BoldItalic.otf",         // "Klavika Basic Bold Italic"
    ":/OriginCommonControls/Fonts/KlavikaBasic-Light.otf",              // "Klavika Basic Light"
    ":/OriginCommonControls/Fonts/KlavikaBasic-LightItalic.otf",        // "Klavika Basic Light Italic"
    ":/OriginCommonControls/Fonts/KlavikaBasic-Medium.otf",             // "Klavika Basic Medium"
    ":/OriginCommonControls/Fonts/KlavikaBasic-MediumItalic.otf",       // "Klavika Basic Medium Italic"
    ":/OriginCommonControls/Fonts/KlavikaBasic-Regular.otf",            // "Klavika Basic Regular"
    ":/OriginCommonControls/Fonts/KlavikaBasic-RegularItalic.otf"       // "Klavika Basic Regular Italic"
};

const int kFontResourceCount = sizeof(fontResourcePaths)/sizeof(fontResourcePaths[0]);


void OriginUIBase::initFonts()
{
    for(int i = 0; i < kFontResourceCount; i++)
    {
        int result = QFontDatabase::addApplicationFont(fontResourcePaths[i]);
        Q_UNUSED(result);
    }
}
}
}
