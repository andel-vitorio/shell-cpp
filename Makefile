# Diretórios do projeto
INC_DIR = inc
LIB_DIR = lib
OBJ_DIR = obj
SRC_DIR = src

# Nome do executável
CURRENT_DIR := $(notdir $(shell pwd))
EXECUTABLE := $(CURRENT_DIR)

# Arquivos fonte, cabeçalhos e objetos
SOURCE_FILES := $(wildcard $(SRC_DIR)/*.cpp)
HEADER_FILES := $(wildcard $(INC_DIR)/*.h)
OBJECT_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCE_FILES))

# Compilador e opções
CXX := g++
CXXFLAGS := -std=c++17 -I$(INC_DIR)

# Alvos do Makefile
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECT_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lm

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADER_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: clean $(EXECUTABLE)
	./$(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE) $(OBJECT_FILES)

.PHONY: all run clean
