test:	main.cpp
	c++ -std=c++11 main.cpp messages1.pb.cc -o test `pkg-config --cflags --libs protobuf`