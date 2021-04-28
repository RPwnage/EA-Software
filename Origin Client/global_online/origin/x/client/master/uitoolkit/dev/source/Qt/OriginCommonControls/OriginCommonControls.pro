CONFIG      += plugin debug_and_release
TARGET      = $$qtLibraryTarget(origincommoncontrolsplugin)
TEMPLATE    = lib

INCLUDEPATH = ../../../../../include/Qt ../../../include/Qt
HEADERS     = originpushbuttonplugin.h   origincheckbuttonplugin.h   originradiobuttonplugin.h   originlabelplugin.h   originsinglelineeditplugin.h    origincommoncontrols.h   origindropdownplugin.h   origintooltipplugin.h   originmultilineeditplugin.h   originspinnerplugin.h   originclosebuttonplugin.h   originwebviewplugin.h	OriginCommonUIUtils.h	originprogressbarplugin.h    originbannerplugin.h		originvalidationcontainerplugin.h origincommandlinkplugin.h originiconbuttonplugin.h origintransferbarplugin.h \
    originsurveywidgetplugin.h   originsliderplugin.h   origindividerplugin.h
SOURCES     = originpushbuttonplugin.cpp origincheckbuttonplugin.cpp originradiobuttonplugin.cpp originlabelplugin.cpp originsinglelineeditplugin.cpp  origincommoncontrols.cpp origindropdownplugin.cpp origintooltipplugin.cpp originmultilineeditplugin.cpp originspinnerplugin.cpp originclosebuttonplugin.cpp originwebviewplugin.cpp	OriginCommonUIUtils.cpp	originprogressbarplugin.cpp	originbannerplugin.cpp	originvalidationcontainerplugin.cpp origincommandlinkplugin.cpp originiconbuttonplugin.cpp origintransferbarplugin.cpp \
    originsurveywidgetplugin.cpp   originsliderplugin.cpp   origindividerplugin.cpp
RESOURCES   = Resources/origincommoncontrols.qrc
LIBS        += -L. 

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS    += target
QT += designer webkitwidgets

include(originmultilineedit.pri)
include(originmultilineeditcontrol.pri)
include(originpushbutton.pri)
include(originlabel.pri)
include(originradiobutton.pri)
include(originsinglelineedit.pri)
include(origincheckbutton.pri)
include(origindropdown.pri)
include(originuibase.pri)
include(originspinner.pri)
include(originclosebutton.pri)
include(originprogressbar.pri)
include(originwebview.pri)
include(originvalidationcontainer.pri)
include(originbanner.pri)
include(origincommandlink.pri)
include(originiconbutton.pri)
include(origintransferbar.pri)
include(origintooltip.pri)
include(originsurveywidget.pri)
include(originslider.pri)
include(origindivider.pri)
FORMS += 
