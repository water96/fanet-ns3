TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

NS3DIR = "/home/aleksey/work/ns-allinone-3.30/ns-3.30"

SOURCES += \
            fanet.cc

INCLUDEPATH += $$NS3DIR/build

message ($$INCLUDEPATH)

unix:!macx: LIBS += -L$$NS3DIR/build/lib/ \
                    -lns3.30-core-debug \
                    -lns3.30-network-debug \
                    -lns3.30-internet-debug \
                    -lns3.30-point-to-point-debug \
                    -lns3.30-applications-debug \
                    -lns3.30-stats-debug \

INCLUDEPATH += $$NS3DIR/build
DEPENDPATH += $$NS3DIR/build
