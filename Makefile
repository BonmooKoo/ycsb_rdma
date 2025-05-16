#make CXXFLAGS="-std=gnu++11 -O3 -I./"
CXX=g++
LDFLAGS=-libverbs -lmemcached -lpthread
CXXFLAGS= -std=gnu++11 -O3  
INCLUDES = -I./

SRCS_COMMON = keeper.cpp rdma_verb.cpp rdma_common.cpp zipf.cpp
OBJS_COMMON = $(SRCS_COMMON:.cpp=.o)

all: client server

client: $(OBJS_COMMON) rdma_client.o
	$(CXX) -g $(CXXFLAGS) -o client $(OBJS_COMMON) rdma_client.o $(LDFLAGS)

server: $(OBJS_COMMON) rdma_server.o
	$(CXX) $(CXXFLAGS) -o server rdma_common.o rdma_server.o keeper.o $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o client server

