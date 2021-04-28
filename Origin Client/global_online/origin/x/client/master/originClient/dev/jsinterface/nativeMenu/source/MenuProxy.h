#ifndef _MENUPROXY_H
#define _MENUPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QVariant>
#include <qpointer.h>

#include "engine/igo/IIGOWindowManager.h"
#include "services/plugin/PluginAPI.h"


class QMenu;
class QAction;
class QWebPage;

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API MenuProxy : public QObject, public Origin::Engine::IIGOWindowManager::IContextMenuListener
{
	Q_OBJECT
public:
	MenuProxy(QWebPage *page, QObject *parent = NULL);
	~MenuProxy();

	Q_INVOKABLE QObject *addAction(const QString &text);
	Q_INVOKABLE QObject *addMenu(const QString &text);
	Q_INVOKABLE QObject *addSeparator();

    Q_INVOKABLE void     addObjectName(const QString &text);

    // Operate on QAction*
    Q_INVOKABLE void addAction(QObject *);
    Q_INVOKABLE void removeAction(QObject *);

	Q_INVOKABLE void popup();
	Q_INVOKABLE void popup(QVariantMap pos);
	Q_INVOKABLE void hide();

	Q_INVOKABLE void clear();
    Q_INVOKABLE void setMinWidth(int w);
    Q_INVOKABLE void setVisible(bool visible);

	Q_PROPERTY(QObject* menuAction READ menuAction);
	QObject *menuAction();

signals:
	void aboutToHide();
	void aboutToShow();

private slots:
    // IContextMenuListener impl
    virtual void onFocusChanged(QWidget* window, bool hasFocus);
    virtual void onDeactivate();

    void hiding();
    void onShowSubmenu();
    void onHideSubmenu();

private:
    QWebPage *mPage;
	QPointer<QMenu> mNativeMenu;
    QPointer<QMenu> mSubmenu;
};

}
}
}

#endif
