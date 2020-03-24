include(../ns3-first.pri)

TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        fanet.cc

unix:!macx: LIBS += -L$$(NS3DIR)/build/lib/ \
                    -lns3.30-core-debug \
                    -lns3.30-network-debug \
                    -lns3.30-internet-debug \
                    -lns3.30-point-to-point-debug \
                    -lns3.30-applications-debug \
                    -lns3.30-stats-debug \

DEPENDPATH += $$(NS3DIR)/build
