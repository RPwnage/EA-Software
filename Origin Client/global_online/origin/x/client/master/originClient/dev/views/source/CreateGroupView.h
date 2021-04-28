/////////////////////////////////////////////////////////////////////////////
// CreateGroupView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CREATE_GROUP_VIEW_H
#define CREATE_GROUP_VIEW_H

#include <QWidget>

class QFrame;
class QLayout;
class QEvent;

namespace Ui
{
    class CreateGroupView;
}

namespace Origin
{
	namespace Client
	{
		class CreateGroupView : public QWidget
		{
			Q_OBJECT

		public:

			CreateGroupView(QWidget *parent);
			~CreateGroupView();

            void init();

            QString getGroupName();
            void setGroupName(const QString& groupName);

            void setFocus();

        signals:
            void acceptButtonEnabled(bool isEnabled);

        protected slots:
            void onGroupNameTextChanged(const QString& text);

		private:
			Ui::CreateGroupView* ui;
		};
	}
}

#endif //CREATE_GROUP_VIEW_H
