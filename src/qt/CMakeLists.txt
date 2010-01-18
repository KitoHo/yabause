project( yabause-qt )

find_package(Qt4)

if (NOT QT4_FOUND)
	return()
endif (NOT QT4_FOUND)

set( QT_USE_QTCORE TRUE )
set( QT_USE_QTGUI TRUE )
set( QT_USE_QTOPENGL TRUE )

# dunno what it does exactly ... but seem required
include( ${QT_USE_FILE} )

# qt resources file
set( yabause_qt_RESOURCES resources/resources.qrc )

# qt forms
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

# pure C headers
set( yabause_qt_HEADERS
	QtYabause.h
	CommonDialogs.h
	PerQt.h )

# C headers needing moc
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

# C sources
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

QT4_ADD_RESOURCES( yabause_qt_RCC_RESOURCES ${yabause_qt_RESOURCES} )
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

set_target_properties( yabause-qt PROPERTIES OUTPUT_NAME yabause-qt )
link_directories( .. )
target_link_libraries( yabause-qt ${PORT_LIBRARIES} )
target_link_libraries( yabause-qt ${QT_LIBRARIES} )
target_link_libraries( yabause-qt yabause )
