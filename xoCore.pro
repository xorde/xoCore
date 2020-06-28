QT += core websockets network widgets script
TARGET = xoCore
TEMPLATE = lib

include("../xoTools/Compilation.pri")
include("../xoTools/xoTools.pri")

DEFINES += XOCORE_LIBRARY
DEFINES += QT_DEPRECATED_WARNINGS

VER_MAJ = 0
VER_MIN = 1
VER_PAT = 0
VERSION = $$VER_MAJ"."$$VER_MIN"."$$VER_PAT

DEFINES += VERSION_NUMBER=$$VERSION

SOURCES += \
    ComponentsConfigParser.cpp \
    Data/ComponentConnection.cpp \
    Data/ComponentInfo.cpp \
    Loader.cpp \
    Module/ObjectProxy.cpp \
    ONBMetaDescriptor.cpp \
    ONBSettings.cpp \
    ConfigManager.cpp \
    ModuleConfig.cpp \
    Scheme.cpp \
    Hub.cpp \
    ModuleList.cpp \
    Core.cpp \
    ScriptEngineWrapper.cpp \
    Server.cpp \
    PluginManager.cpp \
    Module/ComponentProxyONB.cpp \
    Module/ModuleProxyONB.cpp \
    ONBMetaDescription.cpp \
    xoCorePlugin.cpp \
    xoPrimitiveConsole.cpp

HEADERS += \
    Loader.h \
    ScriptEngineWrapper.h \
    xoCore_global.h \
    xoCorePlugin.h \
    ComponentsConfigParser.h \
    Data/ComponentConnection.h \
    Data/ComponentInfo.h \
    Module/ObjectProxy.h \
    ModuleConfig.h \
    ModuleStartType.h \
    ONBMetaDescriptor.h \
    ONBSettings.h \
    Scheme.h \
    Hub.h \
    ModuleList.h \
    Core.h \
    Server.h \
    PluginManager.h \
    Module/ComponentProxyONB.h \
    Module/ModuleProxyONB.h \
    ONBMetaDescription.h \
    ConfigManager.h \
    xoPrimitiveConsole.h


DISTFILES += xoCorePlugin.pri xoCore.pri
