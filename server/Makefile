
#C compiler
CC = g++

SRC = server.cpp Chrono.cpp ChronoCpu.cpp

CC_FLAGS = -pthread -lz
#CC_FLAGS += -D COMPRESSION

EXE = server

release:$(SRC)
	$(CC) $(SRC) $(CC_FLAGS) -o $(EXE) 

clean: $(SRC)
	rm -f $(EXE) $(EXE_X) $(EXE).linkinfo 
