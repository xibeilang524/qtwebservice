QT = core network qtestlib
CONFIG += qtestlib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../lib/ -lQWebService
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../lib/ -lQWebServiced
else:symbian: LIBS += -lQWebService
else:unix: LIBS += -L$$PWD/../../lib/ -lQWebService

INCLUDEPATH += $$PWD/../../QWebService/headers
DEPENDPATH += $$PWD/../../QWebService/headers

SOURCES += tst_qwebservicemethod.cpp
