#include "EulaPrintViewController.h"
#ifdef ORIGIN_MAC
#include "Services/debug/DebugService.h"
#include "Services/platform/PlatformService.h"
#include <QtPrintSupport/QPrinter>

namespace Origin
{
namespace Client
{
    
EulaPrintDialog::EulaPrintDialog(const QString& html, QPrinter* printer, QDialog* parent)
: QPrintDialog(printer, parent)
{
    mDoc.setHtml(html);
    
    // Display as sheet attached to parent window
    setWindowModality(Qt::WindowModal);
}

EulaPrintDialog::~EulaPrintDialog() 
{
    delete printer();
}

void EulaPrintDialog::done(int result)
{
    if (result == QDialog::Accepted)
    {
        // Print!
        mDoc.print(printer());
    }
    
    deleteLater();

    // Stop the hidden dialog parent event loop
    QDialog* parent = static_cast<QDialog*>(parentWidget());
    parent->done(QDialog::Accepted);
}


EulaWrapperDialog::EulaWrapperDialog(const QString& html, QDialog* parent)
: QDialog(parent, Qt::FramelessWindowHint)
, mHtml(html)
, mPrintDialog(NULL)
{
}


void EulaWrapperDialog::paintEvent(QPaintEvent* event)
{
    QDialog::paintEvent(event);

    if (!mPrintDialog)
    {
        // We use the paint event to make sure the new dialog is initialized before changing its visibility/attaching the printer dialog sheet to it
        Origin::Services::PlatformService::MakeWidgetInvisible(winId(), true);
        
        QPrinter *printer = new QPrinter;
        printer->setOutputFormat(QPrinter::NativeFormat);

        mPrintDialog = new EulaPrintDialog(mHtml, printer, this);
        mPrintDialog->show(); // need to use show for Qt to use the dialog modality/create a window sheet instead of a separate window
    }
}


void EulaWrapperDialog::run()
{
    // Locate the new dialog on top of its parent, but down a bit so that the printer dialog sheet will appear under the title bar.
    QDialog* parent = static_cast<QDialog*>(parentWidget());
    
    const int ParentTitleBarHeight = 36;
    resize(parent->size().width(), parent->size().height());
    move(parent->pos().x(), parent->pos().y() + ParentTitleBarHeight);

    // Disable the parent dialog buttons - just for the visual aspect since we're running a separate event loop anyways
    emit changeButtonEnabledStatus(false);
    
    // Start the dialog loop so that we keep input focus on the printer sheet
    exec();
    
    // Re-enable the parent dialog buttons
    emit changeButtonEnabledStatus(true);
}

}
}
#endif