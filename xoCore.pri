XOCOREDIR  = $${DESTDIR}
XOCOREDIRH = "$$PWD/../xoCore/"
XOCORELIB  = "xoCore"

CONFIG(debug, debug|release): XOCORELIB = $$join(XOCORELIB,,,d)
LIBS += -L$${XOCOREDIR} -l$${XOCORELIB}

INCLUDEPATH *= $${XOCOREDIRH}
INCLUDEPATH *= $$PWD

include("$$PWD/../xoTools/xoTools.pri")
