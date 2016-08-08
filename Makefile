CC=g++
OBJ= bin/
TARGET=segnet_optimisation

CFLAGS= `pkg-config --cflags opencv` -O2 -Wall -std=c++11 $(BOOST)
LINKFLAGS = `pkg-config --libs opencv`
CUDA = -I/usr/local/cuda/include -L/usr/local/cuda/lib
BOOST = -lboost_system -lboost_filesystem -lboost_regex
VIVDIR= -L/usr/local/lib/ -I/usr/local/include/
VIVLIB = -lvivacity
SQL = -lmysqlcppconn
CAFFE = -lcaffe
NCURSES = -lncurses

OBJECTS= segnet_optimisation.o \
	 json11.o

all:$(TARGET)

$(TARGET): $(addprefix $(OBJ),$(OBJECTS)) 
	$(CC) -o $(OBJ)$(TARGET) $(addprefix $(OBJ),$(OBJECTS)) $(VIVDIR) $(VIVLIB) $(BOOST) $(NCURSES) $(LINKFLAGS) $(SQL) $(CAFFE) -lglog -lgflags -lcpr -lcurl

$(OBJ)%.o: $(SRC)%.cpp | MKDIR
	$(CC) $< -g -o $@ -c $(CFLAGS) -w $(CUDA) $(CAFFE) -I./include

clean:
	rm -rf $(OBJ)*.o 

MKDIR:
	mkdir -p $(OBJ)

print-%: ; @echo $*=$($*)
