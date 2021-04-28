///////////////////////////////////////////////////////////////////////////////
// ProgressView.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ProgressView_h
#define ProgressView_h

#include "DataCollector.h"
#include <QWidget>

namespace Ui
{
    class ProgressViewWidget;
};

namespace Origin
{
    namespace UIToolkit 
    {
        class OriginWindow;
    };
};

class ProgressViewWidget : public QWidget
{
    Q_OBJECT

public:
    ProgressViewWidget(QWidget* parent = NULL);
    ~ProgressViewWidget();

    void setProgressCancel();

signals:
    void submitFinished();

public slots:
    void onBackendProgress(DataCollectedType completedType, DataCollectedType startingType);

private:
    Ui::ProgressViewWidget* ui;
};

#endif