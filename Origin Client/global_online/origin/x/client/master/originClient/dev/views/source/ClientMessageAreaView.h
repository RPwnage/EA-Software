/////////////////////////////////////////////////////////////////////////////
// ClientMessageAreaView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTMESSAGEAREAVIEW_H
#define CLIENTMESSAGEAREAVIEW_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

class QPushButton;

namespace Ui
{
    class ClientMessageAreaView;
}

namespace Origin
{
	namespace Client
	{
		class ORIGIN_PLUGIN_API ClientMessageAreaView : public QWidget
		{
			Q_OBJECT
            Q_PROPERTY(QString title READ title WRITE setTitle)
            Q_PROPERTY(QString text READ text WRITE setText)

		public:
			ClientMessageAreaView(QWidget* parent = 0);
			~ClientMessageAreaView();

            void setTitle(const QString& title);
            QString title() const;

			void setText(const QString& text);
			QString text() const;

            QPushButton* firstButton();

            QPushButton* secondButton();


		private:
			Ui::ClientMessageAreaView* ui;
		};
	}
}

#endif