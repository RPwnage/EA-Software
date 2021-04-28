///////////////////////////////////////////////////////////////////////////////
// ReportErrorView.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ReportErrorView_h
#define ReportErrorView_h

#include "DataCollector.h"
#include <QMap>
#include <QMetaType>
#include <QWidget>

#include "OriginCommonUI.h"

namespace Ui
{
    class ReportErrorView;
};

class QStringList;

namespace Origin 
{
    namespace UIToolkit
    {
        class OriginWindow;
    };
};

/*
TODO:
	Try to run the binary directly from the web site
	Sign the binary
	Able to copy and paste tracking number, with mouse click-drag, not just double-click
	Use the oring client scroll bar.
*/
class ReportErrorView : public QWidget
{
	Q_OBJECT
public:
	ReportErrorView(QString const& locale, QString const& originId);
	~ReportErrorView();

	bool eventFilter(QObject *object, QEvent *event);

	// list contains the elements to show in their proper order, with "select one please""
	// at the top, "other" at the bottom, and the remaining entries sorted. map contains
	// a map from localized to non-localized strings, so the app sends the same strings
	// back to the server regardless of localization.
    void setProblemAreas(QStringList const& problemList, QList<QPair<QString,int> > categoryOrder);
    void setCurrentCategory(const int& category);
	void setDescription(const QString& description);

	QString getUserId();
	QString getMessage();
	QString getProblemArea();
    QString getCeNumber();

    void addProblemArea(const QString& newArea);

	bool isIdFieldBlank();
	bool isNotesFieldBlank();
	bool isIssueFieldBlank();

    void setSending(bool sending);
    void setSubmitButton(Origin::UIToolkit::OriginPushButton *submitButton);
    void onHideProgress();

	void updateProgressWindow(DataCollectedType completedType, DataCollectedType startingType);

signals:
	void submitReport();
    void cancelSubmit();
    void enableSubmit(bool);

public slots:
    void onSubmitClicked();
	void setProgressCancelButton(bool status);

private slots:
	void updateCharacterCount();
	void onCurrentIssueChanged(QString const&);
	void onConfirmStateChanged(int);
	void validateSubmitReport();
    void onClickPrivacyQuestion(const QString&);
    void onHelpClicked(const QString&);
    void onProgressWindowCanceled();


private:
    Ui::ReportErrorView* ui;
	QString m_locale;
    QString m_cmdline_originId;
    QStringList mProblemList;
    QList<QPair<QString,int> > mCategoryOrder;
	bool m_propagate;
    Origin::UIToolkit::OriginWindow* m_progressWindow;
    Origin::UIToolkit::OriginPushButton *mSubmitButton;
};

#endif
