include(../ns3-first.pri)

TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
TARGET = 7script

SOURCES += \
        fanet.cc

unix:!macx: LIBS += -L$$(NS3DIR)/build/lib/ \
                    -lns3-dev-core-debug \
                    -lns3-dev-network-debug \
                    -lns3-dev-internet-debug \
                    -lns3-dev-point-to-point-debug \
                    -lns3-dev-applications-debug \
                    -lns3-dev-stats-debug \

DEPENDPATH += $$(NS3DIR)/build
