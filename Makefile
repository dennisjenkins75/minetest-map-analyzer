#CXXFLAGS+=	--std=c++17 -Wall -O2 -ggdb -pg
CXXFLAGS+=	--std=c++17 -Wall -O3 -ggdb
LDFLAGS+=	$(shell pkg-config sqlite3 --libs)
LDFLAGS+=	$(shell pkg-config zlib --libs)

all:	mt-map-search

clean:
	rm -f mt-map-search
	rm -f *.o

run:	mt-map-search
	./mt-map-search

SRCS:=	$(sort $(basename blob_reader.cc inventory.cc mapblock.cc \
		mt-map-search.cc node.ccv pos.cc utils.cc))

OBJS:=	$(SRCS:=.o)

$(OBJS):	*.h

mt-map-search:	$(OBJS)
	$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LDFLAGS)
