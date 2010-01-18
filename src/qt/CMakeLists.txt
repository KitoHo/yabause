project( yabause-qt )

set( QT_DONT_USE_QTCORE TRUE )
set( QT_DONT_USE_QTGUI TRUE )
set( QT_USE_QTOPENGL TRUE )

#QT_DONT_USE_QTCORE
#QT_DONT_USE_QTGUI
#QT_USE_QT3SUPPORT
#QT_USE_QTASSISTANT
#QT_USE_QAXCONTAINER
#QT_USE_QAXSERVER
#QT_USE_QTDESIGNER
#QT_USE_QTMOTIF
#QT_USE_QTMAIN
#QT_USE_QTNETWORK
#QT_USE_QTNSPLUGIN
#QT_USE_QTOPENGL
#QT_USE_QTSQL
#QT_USE_QTXML
#QT_USE_QTSVG
#QT_USE_QTTEST
#QT_USE_QTUITOOLS
#QT_USE_QTDBUS
#QT_USE_QTSCRIPT
#QT_USE_QTASSISTANTCLIENT
#QT_USE_QTHELP
#QT_USE_QTWEBKIT
#QT_USE_QTXMLPATTERNS
#QT_USE_PHONON

include( ${QT_USE_FILE} )

set( yabause_qt_RESOURCES resources/resources.qrc )

set( yabause_qt_FORMS
	ui/UIYabause.ui
	ui/UISettings.ui
	ui/UIAbout.ui
	ui/UICheats.ui
	ui/UICheatAR.ui
	ui/UICheatRaw.ui
	ui/UIWaitInput.ui
	ui/UIBackupRam.ui
	ui/UIPortManager.ui
	ui/UIPadSetting.ui )

set( yabause_qt_HEADERS
	QtYabause.h
	CommonDialogs.h
	PerQt.h )

set( yabause_qt_MOC_HEADERS
	ui/UIYabause.h
	ui/UISettings.h
	ui/UIAbout.h
	ui/UICheats.h
	ui/UICheatAR.h
	ui/UICheatRaw.h
	ui/UIWaitInput.h
	ui/UIBackupRam.h
	ui/UIPortManager.h
	ui/UIPadSetting.h
	YabauseGL.h
	Settings.h
	YabauseThread.h )

set( yabause_qt_SOURCES
	main.cpp
	QtYabause.cpp
	CommonDialogs.cpp
	PerQt.c
	ui/UIYabause.cpp
	ui/UISettings.cpp
	ui/UIAbout.cpp
	ui/UICheats.cpp
	ui/UICheatAR.cpp
	ui/UICheatRaw.cpp
	ui/UIWaitInput.cpp
	ui/UIBackupRam.cpp
	ui/UIPortManager.cpp
	ui/UIPadSetting.cpp
	YabauseGL.cpp
	Settings.cpp
	YabauseThread.cpp )

QT4_ADD_RESOURCES(yabause_qt_RCC_RESOURCES ${yabause_qt_RESOURCES})
QT4_WRAP_UI( yabause_qt_UI_FORMS ${yabause_qt_FORMS} )
QT4_WRAP_CPP( yabause_qt_MOC_SOURCES ${yabause_qt_MOC_HEADERS} )

add_executable( yabause-qt
	${yabause_qt_RCC_RESOURCES}
	${yabause_qt_HEADERS}
	${yabause_qt_MOC_HEADERS}
	${yabause_qt_SOURCES}
	${yabause_qt_UI_FORMS}
	${yabause_qt_MOC_SOURCES} )

include_directories(
	.
	${CMAKE_CURRENT_BINARY_DIR}
	${QT_INCLUDE_DIR}
	${QT_QTCORE_INCLUDE_DIR}
	${QT_QTGUI_INCLUDE_DIR}
	${QT_QTOPENGL_INCLUDE_DIR} )

link_directories(..)

set_target_properties(yabause-qt PROPERTIES OUTPUT_NAME yabause)
target_link_libraries(yabause-qt ${PORT_LIBRARIES})
target_link_libraries(yabause-qt ${QT_LIBRARIES})
target_link_libraries(yabause-qt yabause)
