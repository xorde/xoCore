TEMPLATE = lib
CONFIG  += plugin
QT      += core

include("../xoTools/Compilation.pri")
include("../xoCore/xoCore.pri")

DESTDIR = $$join(DESTDIR,,,/xoCorePlugins/)

QMAKE_POST_LINK += mkdir $$shell_path($${DESTDIR}) &
