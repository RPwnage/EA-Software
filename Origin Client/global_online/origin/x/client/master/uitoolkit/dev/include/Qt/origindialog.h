#ifndef ORIGINDIALOG_H_
#define ORIGINDIALOG_H_

#include "originchromebase.h"
#include "uitoolkitpluginapi.h"

namespace Ui
{
    class OriginDialogTitlebar;
}

namespace Origin
{
namespace UIToolkit
{

class UITOOLKIT_PLUGIN_API OriginDialog : public Origin::UIToolkit::OriginChromeBase
{
	Q_OBJECT

public:
	OriginDialog(QWidget* content = 0, QWidget* parent = 0);
	~OriginDialog();
	const QMargins borderWidths() const {return QMargins(9,6,8,9);}
    void setBannerVisible(const bool&) {}
    void setMsgBannerVisible(const bool&) {}


signals:
    void clicked();

protected:
	void pluginCustomUi();
	QString& getElementStyleSheet();
	void prepForStyleReload();
    void ensureCreatedMsgBanner() {}
    void mouseReleaseEvent(QMouseEvent*);


private slots:

private:
	Ui::OriginDialogTitlebar *ui;
	static QString mPrivateStyleSheet;
};

}
}
#endif