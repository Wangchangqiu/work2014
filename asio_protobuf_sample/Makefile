# Makefile for asio_protobuf_samples
# Expects protobuf and boost to be installed 
#
PROTOBUF_CFLAGS = `pkg-config --cflags protobuf`
PROTOBUF_LFLAGS = `pkg-config --libs protobuf`
BOOST_LFLAGS = -lboost_system
OBJ_PATH = /home/rtu/Desktop/obj/


server_main: server_main.o stringdb.pb.o db_server.o 
	g++ -o $(OBJ_PATH)server_main server_main.o db_server.o stringdb.pb.o \
		$(PROTOBUF_LFLAGS) $(BOOST_LFLAGS) 

stringdb.pb.o: stringdb.proto
	protoc --cpp_out=. --python_out=. stringdb.proto
	g++ -c stringdb.pb.cc $(PROTOBUF_CFLAGS)

HEADERS = $(wildcard *.h)

%.o: %.cpp stringdb.pb.o ${HEADERS}
	g++ -c $< -I$(BOOST_INCLUDE) -fpermissive 

clean:
	rm -f *.pb.*
	rm -f *_pb2.*
	rm -f *.o 
	rm -f $(OBJ_PATH)server_main 
