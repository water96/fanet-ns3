TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
TARGET = 7script

SOURCES += \
    7script.cc

unix:!macx: LIBS += -L$$(NS3DIR)/build/lib/ \
                    -l$$(NS3VER)-core-debug \
                    -l$$(NS3VER)-network-debug \
                    -l$$(NS3VER)-internet-debug \
                    -l$$(NS3VER)-point-to-point-debug \
                    -l$$(NS3VER)-applications-debug \
                    -l$$(NS3VER)-stats-debug \

DEPENDPATH += $$(NS3DIR)/build
INCLUDEPATH += $$(NS3DIR)/build
