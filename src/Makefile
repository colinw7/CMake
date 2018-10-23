CC = g++
AR = ar
RM = rm

CDEBUG = -g
LDEBUG = -g

INC_DIR = ../include
OBJ_DIR = ../obj
LIB_DIR = ../lib
BIN_DIR = ../bin

all: $(BIN_DIR)/CMake

SRC = \
CMake.cpp \

OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

CPPFLAGS = \
-std=c++14 \
-I. \
-I$(INC_DIR) \
-I../../CFile/include \
-I../../CStrUtil/include \
-I../../COS/include \

LFLAGS = \
-L$(LIB_DIR) \
-L../../CFile/lib \
-L../../CStrUtil/lib \
-L../../COS/lib \

LIBS = \
-lCFile \
-lCStrUtil \
-lCOS \

clean:
	$(RM) -f $(OBJ_DIR)/*.o
	$(RM) -f $(BIN_DIR)/CMake

$(OBJS): $(OBJ_DIR)/%.o: %.cpp
	$(CC) $(CDEBUG) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

.SUFFIXES: .cpp

$(BIN_DIR)/CMake: $(OBJ_DIR)/CMake.o
	$(CC) $(LDEBUG) -o $(BIN_DIR)/CMake $(OBJ_DIR)/CMake.o $(LFLAGS) $(LIBS)
