.PHOYN: clean all

CC = g++
CFLAGS = -I ./inc -std=c++0x -g -L /usr/local/lib64 -lyaml-cpp -lpthread -ldl 
CFALG += -U__FILE__ -D__FILE__='"$(subst $(dir $<),,$<)"'

all_target = test_log test_config test_thread test_util test_scheduler test_fiber test_iomanager

all: $(all_target) 

SRC_FILE = $(wildcard ./src/*.cpp)
OBJ_FILE = $(patsubst ./src/%.cpp, ./obj/%.o, $(SRC_FILE))
TEST_FILE = $(wildcard ./tests/*.cpp)
TEST_OBJ_FILE = $(patsubst ./tests/%.cpp, ./obj/%.o, $(TEST_FILE))

#test_util: $(OBJ_FILE) $(TEST_OBJ_FILE) 
#	$(CC) ./tests/$@.cpp $(OBJ_FILE) -o ./bin/$@ $(CFLAGS)
#
#test_log: $(OBJ_FILE) $(TEST_OBJ_FILE)
#	$(CC) ./tests/test_log.cpp $(OBJ_FILE) -o ./bin/$@ $(CFLAGS)
#
#test_config: $(OBJ_FILE) $(TEST_OBJ_FILE)  
#	$(CC) ./tests/test_config.cpp $(OBJ_FILE) -o ./bin/$@ $(CFLAGS)
#	
#test_thread: $(OBJ_FILE) $(TEST_OBJ_FILE) 
#	$(CC) ./tests/$@.cpp $(OBJ_FILE) -o ./bin/$@ $(CFLAGS)

$(all_target): %: $(OBJ_FILE) $(TEST_OBJ_FILE)
	$(CC) $(patsubst %, ./obj/%.o, $@) $(OBJ_FILE) -o $(patsubst %, ./bin/%, $@) $(CFLAGS)

$(TEST_OBJ_FILE): ./obj/%.o: ./tests/%.cpp
	$(CC) -c $< -o $@ $(CFLAGS)
	
./obj/%.o: ./src/%.cpp
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm $(OBJ_FILE) $(patsubst %, ./bin/%, $(all_target)) $(TEST_OBJ_FILE)

