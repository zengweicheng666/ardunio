QT += core network websockets sql
QT -= gui

TARGET = ControlServer
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += "../libraries/shared"

#Include files
include(ControlServer.pri)
include($$PWD/QtSolutionsService/src/qtservice.pri)

#Windows resource file
//win32:RC_FILE = AppServer.rc
