CXXFLAGS = -std=c++0x
CXX = g++

all: urecv usend

urecv: urecv.o common.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

usend: usend.o common.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

urecv.o: urecv.cc common.hh
usend.o: usend.cc common.hh
common.o: common.cc common.hh
