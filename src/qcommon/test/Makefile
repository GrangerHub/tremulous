all: msg_test CmdParser_test COM_Parse_test q_shared_test

CXXFLAGS=-O0 -ggdb -std=c++14 -Wall -Werror -fsanitize=address -fno-omit-frame-pointer 
INCLUDE= -I ../../../external/catch -I ../../

CmdParser_test: CmdParser.cpp ../CmdParser.h
	c++ ${CXXFLAGS} ${INCLUDE} CmdParser.cpp -o CmdParser_test

msg_test: msg.cpp ../msg.cpp ../msg.h ../huffman.h
	c++ ${CXXFLAGS} ${INCLUDE} ../huffman.cpp ../msg.cpp msg.cpp -o msg_test

COM_Parse_test: COM_Parse.cpp ../q_shared.cpp ../q_shared.h
	c++ -g -O0 -std=c++14 -I ../../../external/catch -I ../.. ../q_shared.cpp COM_Parse.cpp -o COM_Parse_test

q_shared_test: q_shared_test.cpp ../q_shared.cpp ../q_shared.h
	c++ -g -O0 -std=c++14 -I ../../../external/catch -I ../.. ../q_shared.cpp q_shared_test.cpp -o q_shared_test 

check: msg_test COM_Parse_test CmdParser_test q_shared_test
	./msg_test
	./COM_Parse_test
	./CmdParser_test
	./q_shared_test

clean:
	rm -f q_shared_test 
	rm -f COM_Parse_test
	rm -f CmdParser_test
	rm -f msg_test
	rm -rf *.dSYM
