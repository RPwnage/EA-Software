#include "origindialog.h"
#include "ui_origindialogtitlebar.h"
#include "ui_originchromebase.h"
#include <QMouseEvent>

namespace Origin {
    namespace UIToolkit {

QString OriginDialog::mPrivateStyleSheet = QString("");


OriginDialog::OriginDialog(QWidget* content, QWidget* parent) 
: OriginChromeBase(parent)
, ui(new Ui::OriginDialogTitlebar)
{
	uiBase->setupUi(this);
	pluginCustomUi();

    setOriginElementName("OriginDialog");
    m_children.append(this);
	setContent(content);

	connect(ui->btnClose, SIGNAL(clicked()), this, SLOT(close()));
}


OriginDialog::~OriginDialog() 
{
	removeElementFromChildList(this);
    delete ui;
    ui = NULL;

}


void OriginDialog::pluginCustomUi()
{
	ui->setupUi(uiBase->titlebar);
}


QString& OriginDialog::getElementStyleSheet()
{
	if (mPrivateStyleSheet.length() == 0)
	{
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
	}
	return mPrivateStyleSheet;
}


void OriginDialog::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}



void OriginDialog::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    if(mouseEvent && mouseEvent->button() == Qt::LeftButton)
    {
        emit clicked();
    }
}


}
}