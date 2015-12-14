containerize: main.cpp
	g++ -std=c++11 main.cpp -o containerize -lboost_system -lboost_program_options -lboost_filesystem
install:
	sudo cp ./containerize /usr/bin
