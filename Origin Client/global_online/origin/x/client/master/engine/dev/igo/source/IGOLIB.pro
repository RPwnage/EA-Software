#-------------------------------------------------
#
# Project created by QtCreator 2011-02-07T15:39:55
#
#-------------------------------------------------

QT       += core gui

TARGET = IGOLIB
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui \
    Forms/IGOWebBrowser.ui \
    Forms/IGOTopBar.ui \
    Forms/IGOBottomBar.ui \
    Forms/IGOHotkeyNotification.ui \
    Forms/IGONotification.ui \
    Forms/IGOFriendsList.ui \
    Forms/IGOChatWindow.ui \
    Forms/IGOChatConversation.ui \
    Forms/IGOChatMessage.ui \
    Forms/IGOFriendsTree.ui \
    Forms/IGONuxDisplay.ui \
    Forms/IGONuxSmall.ui \
	Forms/IGOWebSslInfo.ui

RESOURCES += \
    ../../ui/ebisu.qrc
