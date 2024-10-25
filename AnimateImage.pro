QT       += core gui openglwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += create_prl
CONFIG += link_prl

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    cframe.cpp \
    cframeimage.cpp \
    cframeobject.cpp \
    cframetext.cpp \
    cimagealigner.cpp \
    main.cpp \
    animateimage.cpp

HEADERS += \
    animateimage.h \
    cframe.h \
    cframeimage.h \
    cframeobject.h \
    cframetext.h \
    cimagealigner.h

FORMS += \
    animateimage.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    AnimateImage.qrc

include(gifImage/src/gifimage/qtgifimage.pri)
