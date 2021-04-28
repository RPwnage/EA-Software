/////////////////////////////////////////////////////////////////////////////
// HotkeyToastView.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "HotkeyToastView.h"
#include "ToastView.h"
#include "ui_HotkeyToastView.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/Setting.h"
#include "services/settings/SettingsManager.h"
#include "services/settings/InGameHotKey.h"
#include "originlabel.h"
#include <QNetworkReply>

namespace Origin
{
    namespace Client
    {

        HotkeyToastView::HotkeyToastView(HotkeyContext context, QWidget *parent)
        : QWidget(parent)
        , mContext(context)
        , ui(new Ui::HotkeyToastView)
        {
	        ui->setupUi(this);
	        setupHotkeyLabels();
        }


        HotkeyToastView::~HotkeyToastView()
        {
            if(ui)
            {
                delete ui;
                ui = NULL;
            }
        }


        void HotkeyToastView::setupHotkeyLabels()
        {
            ui->lblPress->setText(tr("ebisu_igo_notification_hotkey_press"));

            switch(mContext)
            {
            case Open:
                ui->lblToDoSomething->setText(tr("ebisu_igo_notification_hotkey_to_open"));
                break;
            case View:
                ui->lblToDoSomething->setText(tr("ebisu_igo_notification_hotkey_to_view"));
                break;
            case Chat:
                ui->lblToDoSomething->setText(tr("ebisu_igo_notification_hotkey_to_chat"));
                break;
            case Reply:
                ui->lblToDoSomething->setText(tr("ebisu_igo_notification_hotkey_to_reply"));
                break;
            case Unpin:
                ui->lblToDoSomething->setText(tr("ebisu_igo_notification_hotkey_to_unpin"));
                break;
            case Repin:
                ui->lblToDoSomething->setText(tr("ebisu_igo_notification_hotkey_to_repin"));
                break;

            }
    
            // If certain locals only have the first string we need hide the second label
            if(ui->lblToDoSomething->text().isEmpty())
            {
                ui->lblToDoSomething->setVisible(false);
            }
            Services::Variant hotkeyInput = NULL;
            if (mContext == Unpin || mContext == Repin)
                hotkeyInput = Services::readSetting(Services::SETTING_PIN_HOTKEY);
            else
                hotkeyInput = Services::readSetting(Services::SETTING_INGAME_HOTKEY);
	        Services::InGameHotKey hotkey = Services::InGameHotKey::fromULongLong(hotkeyInput);

	        //Set all labels to disabled just in case they are not used
	        // We use this value later on to determine if we should show
	        ui->lblModifierKey->setEnabled(false);
	        ui->plus->setEnabled(false);
	        ui->lblUnmodifiedKey->setEnabled(false);

            int32_t modifierKey = hotkey.QtModifierKey();
            modifierKey = (modifierKey & ~Qt::KeypadModifier);
            int32_t unmodifiedKey = hotkey.QtUnmodifiedKey();

	        // Do we have a modifier key?
	        if (modifierKey > 0)
	        {
	            QString modifier;
	            if (modifierKey & Qt::ShiftModifier)
	            {
		            modifier += QObject::tr("ebisu_client_shortcut_shift");
	            }
	            if (modifierKey & Qt::ControlModifier)
	            {
		            modifier += QObject::tr("ebisu_client_shortcut_ctrl");
	            }
	            if (modifierKey & Qt::AltModifier)
	            {
        #if defined(ORIGIN_MAC)
		            modifier += QObject::tr("ebisu_client_shortcut_option");
        #else
		           modifier += QObject::tr("ebisu_client_shortcut_alt");
        #endif
	            }
        #if defined(ORIGIN_MAC)
                if (modifierKey & Qt::MetaModifier)
                {
                    modifier += QObject::tr("ebisu_client_shortcut_command");
                }
        #endif
		        ui->lblModifierKey->setEnabled(true);
		        ui->lblModifierKey->setText(modifier);

		        // Since we have a modifier we need the plus label
		        ui->plus->setEnabled(true);
		        ui->plus->setText("<b>+</b>");
	        }
	        else
	        {
		        ui->lblModifierKey->setVisible(false);
		        ui->plus->setVisible(false);
	        }
	
	        // Do we have an unmodified key?
	        if (unmodifiedKey > 0)
	        {
		        QString unmodified = hotkey.keyDisplayName(unmodifiedKey);
		        ui->lblUnmodifiedKey->setEnabled(true);
		        ui->lblUnmodifiedKey->setText(unmodified);
	        }
        }

        void HotkeyToastView::handleHotkeyLables()
        {
	        this->show();
	        if(ui->lblModifierKey->isEnabled())
		        ui->lblModifierKey->show();
	        if(ui->plus->isEnabled())
		        ui->plus->show();
	        if(ui->lblUnmodifiedKey->isEnabled())
		        ui->lblUnmodifiedKey->show();
        }

        void HotkeyToastView::setOIGHotkeyStyle(bool isHotkey)
        {
	        QString value = (isHotkey) ? "true" : "false";
            setProperty("hotkey", value);
            setStyle(QApplication::style());
        }
    }// namespace Client
}//namespace Origin
