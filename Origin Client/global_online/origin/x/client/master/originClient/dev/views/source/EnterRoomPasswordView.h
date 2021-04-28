/////////////////////////////////////////////////////////////////////////////
// EnterRoomPasswordView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef ENTER_ROOM_PASSWORD_VIEW_H
#define ENTER_ROOM_PASSWORD_VIEW_H

#include <QWidget>

class QFrame;
class QLayout;
class QEvent;

namespace Ui
{
    class EnterRoomPasswordView;
}

namespace Origin
{
	namespace Client
	{
		class EnterRoomPasswordView : public QWidget
		{
			Q_OBJECT

		public:

			EnterRoomPasswordView(QWidget *parent);
			~EnterRoomPasswordView();

            QString getPassword();

            void setNormal();
            void setValidating();
            void setError();

		private:
			Ui::EnterRoomPasswordView* ui;
		};
	}
}

#endif //ENTER_ROOM_PASSWORD_VIEW_H
