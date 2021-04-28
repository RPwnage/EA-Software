#ifndef TESTBEDDIALOG_H
#define TESTBEDDIALOG_H

#include "ui_testbed.h"
#include "originwindow.h"
#include "uitoolkitpluginapi.h"

class UITOOLKIT_PLUGIN_API TestBedDialog : public QWidget
{
    Q_OBJECT

public:
    TestBedDialog(QWidget* w = NULL);

signals:
    void closeDialog();

    public slots:
        void normalClicked();
        void validClicked();
        void validatingClicked();
        void invalidClicked();
        void windowMsgBoxClicked();
        void windowScrollClicked();
        void windowScrollMsgBoxClicked();
        void windowWebViewClicked();
        void searchBoxClicked();
        void onShowHideErrorBanner();
        void onShowHideMsgBanner();


private:
    void setupValidationContainers();

    Ui::TestBedDialog ui;
    Origin::UIToolkit::OriginWindow* mWindow;
};
#endif
