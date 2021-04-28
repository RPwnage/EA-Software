/////////////////////////////////////////////////////////////////////////////
// CreateRoomView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CREATE_ROOM_VIEW_H
#define CREATE_ROOM_VIEW_H

#include <QWidget>

class QFrame;
class QLayout;
class QEvent;

namespace Ui
{
    class CreateRoomView;
}

namespace Origin
{
	namespace Client
	{
		class CreateRoomView : public QWidget
		{
			Q_OBJECT

		public:

			CreateRoomView(QWidget *parent);
			~CreateRoomView();

			void init();

            QString getRoomName();
            QString getPassword();
            bool isRoomLocked();

        signals:
            void acceptButtonEnabled(bool isEnabled);

        protected slots:
            void onPasswordChecked(int state);
            void onPasswordTextChanged(const QString& text);
            void onRoomNameTextChanged(const QString& text);

		private:
			Ui::CreateRoomView* ui;
		};
	}
}

#endif //CREATE_ROOM_VIEW_H
