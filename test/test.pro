TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

message ('$$CONFIG')

CONFIG(release, debug|release){
    NS3_LIB_POSTFIX = 'optimized'
    QMAKE_CXXFLAGS_RELEASE -= -O1
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE *= -O3
} else {
    NS3_LIB_POSTFIX = 'debug'
}

INCLUDEPATH += $$PWD/../

HEADERS += \
    $$PWD/../utils/tracers.h

SOURCES += \
    $$PWD/../utils/tracers.cc


SOURCES += \
    dynamic-global-routing.cc

unix:!macx: LIBS += -L$$(NS3DIR)/build/lib/ \
                    -l$$(NS3VER)-core-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-network-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-internet-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-point-to-point-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-applications-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-stats-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-csma-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-bridge-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-internet-apps-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-wifi-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-mobility-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-netanim-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-wave-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-aodv-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-olsr-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-propagation-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-traffic-control-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-gpsr-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-pagpsr-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-mmgpsr-$$NS3_LIB_POSTFIX \
                    -l$$(NS3VER)-flow-monitor-$$NS3_LIB_POSTFIX

DEPENDPATH += $$(NS3DIR)/build
INCLUDEPATH += $$(NS3DIR)/build
