TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cc \
        utopiamac1.cc

INCLUDEPATH += $$(NS3DIR)/build
INCLUDEPATH += $$PWD/../


#Other
HEADERS += \
    $$PWD/../utils/tracers.h

SOURCES += \
    $$PWD/../utils/tracers.cc

message ($$INCLUDEPATH)

unix:!macx: LIBS += -L$$(NS3DIR)/build/lib/ \
                    -l$$(NS3VER)-core-debug \
                    -l$$(NS3VER)-network-debug \
                    -l$$(NS3VER)-internet-debug \
                    -l$$(NS3VER)-point-to-point-debug \
                    -l$$(NS3VER)-applications-debug \
                    -l$$(NS3VER)-stats-debug \
                    -l$$(NS3VER)-csma-debug \
                    -l$$(NS3VER)-bridge-debug \
                    -l$$(NS3VER)-internet-apps-debug \
                    -l$$(NS3VER)-wifi-debug \
                    -l$$(NS3VER)-mobility-debug \
                    -l$$(NS3VER)-netanim-debug

message ($$LIBS)

INCLUDEPATH += $$(NS3DIR)/build
DEPENDPATH += $$(NS3DIR)/build

HEADERS += \
    utopiamac1.h