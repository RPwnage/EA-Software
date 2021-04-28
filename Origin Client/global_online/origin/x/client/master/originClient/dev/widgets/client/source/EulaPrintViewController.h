#ifndef EULAPRINTVIEWCONTROLLER_H
#define EULAPRINTVIEWCONTROLLER_H

#ifdef ORIGIN_MAC
#include <QtPrintSupport/QPrintDialog>
#include <QTextDocument>
#include "services/plugin/PluginAPI.h"

class QPrinter;
class QDialog;


namespace Origin 
{
namespace Client
{


// Display print dialog as sheet attached to parent window to avoid problems when getting notificiations with the
// relative positions of windows (because Qt internally resets window level+boost them depending on the QWidget window flags
// but doesn't change the core NSPrintPanel dialog - which doesn't give direct access to the window it creates... except if it's a sheet)
class ORIGIN_PLUGIN_API EulaPrintDialog : public QPrintDialog
{
    Q_OBJECT
public:
    EulaPrintDialog(const QString& html, QPrinter* printer, QDialog* parent);
    ~EulaPrintDialog();
    virtual void done(int result);

private:
    QTextDocument mDoc;
};

// Instead of directly attaching the printer dialog sheet to the End User License Agreement dialog, we attach it to a hidden dialog so that we can offset the
// printer sheet down under the original dialog title bar as it should be for Mac.
class EulaWrapperDialog : public QDialog
{
    Q_OBJECT
public:
    EulaWrapperDialog(const QString& html, QDialog* parent);
    virtual void paintEvent(QPaintEvent* event);
    void run();

signals:
    void changeButtonEnabledStatus(const bool& enabled);

private:
    QString mHtml;
    EulaPrintDialog* mPrintDialog;
};

}
}
#endif // ORIGIN_MAC
#endif // EULAPRINTVIEWCONTROLLER_H