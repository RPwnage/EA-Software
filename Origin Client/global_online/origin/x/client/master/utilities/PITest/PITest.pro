#-------------------------------------------------
#
# Project created by QtCreator 2014-04-26T23:05:38
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PITest
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    OriginSDKMain.cpp \
    AppController.cpp

HEADERS  += mainwindow.h \
    OriginSDK/Origin.h \
    OriginSDK/OriginAchievements.h \
    OriginSDK/OriginBaseTypes.h \
    OriginSDK/OriginBlocked.h \
    OriginSDK/OriginChat.h \
    OriginSDK/OriginCommerce.h \
    OriginSDK/OriginDefines.h \
    OriginSDK/OriginEnumeration.h \
    OriginSDK/OriginEnums.h \
    OriginSDK/OriginError.h \
    OriginSDK/OriginEvents.h \
    OriginSDK/OriginFriends.h \
    OriginSDK/OriginIGO.h \
    OriginSDK/OriginIntTypes.h \
    OriginSDK/OriginInvite.h \
    OriginSDK/OriginLanguage.h \
    OriginSDK/OriginMemory.h \
    OriginSDK/OriginPresence.h \
    OriginSDK/OriginProfile.h \
    OriginSDK/OriginProgressiveInstallation.h \
    OriginSDK/OriginRecentPlayers.h \
    OriginSDK/OriginSDK.h \
    OriginSDK/OriginTypes.h \
    EABase/eabase.h \
    EABase/eahave.h \
    EABase/earesult.h \
    EABase/nullptr.h \
    EABase/config/eacompiler.h \
    EABase/config/eacompilertraits.h \
    EABase/config/eaplatform.h \
    OriginSDK/windows/OriginWinTypes.h \
    OriginSDK/mac/OriginMacTypes.h \
    OriginSDKMain.h \
    AppController.h

FORMS    += mainwindow.ui

win32:LIBS += WS2_32.lib
win32:LIBS += Advapi32.lib

QMAKE_CXXFLAGS += -MTd

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/OriginSDK/ -lOriginSDK
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/OriginSDK/ -lOriginSDKd

INCLUDEPATH += $$PWD/OriginSDK
DEPENDPATH += $$PWD/OriginSDK

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/OriginSDK/libOriginSDK.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/OriginSDK/libOriginSDKd.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/OriginSDK/OriginSDK.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/OriginSDK/OriginSDKd.lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/OriginSDK/ -lEAThread
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/OriginSDK/ -lEAThreadd

INCLUDEPATH += $$PWD/OriginSDK
DEPENDPATH += $$PWD/OriginSDK

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/OriginSDK/libEAThread.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/OriginSDK/libEAThreadd.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/OriginSDK/EAThread.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/OriginSDK/EAThreadd.lib
