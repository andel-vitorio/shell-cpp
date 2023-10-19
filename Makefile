# Diretórios do template
INC_DIR = inc
LIB_DIR = lib
OBJ_DIR = obj
SRC_DIR = src

# Obter o nome da pasta
CURRENT_DIR = $(notdir $(shell pwd))
EXECUTABLE = $(CURRENT_DIR)

SOURCE_FILES = $(wildcard $(SRC_DIR)/*.cpp)  # Alterado para .cpp
HEADER_FILES = $(wildcard $(INC_DIR)/*.h)
OBJECT_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCE_FILES))  # Alterado para .cpp

# Compilador e opções
CXX = g++
CXXFLAGS = -std=c++11 -I$(INC_DIR)  # Usando g++ e opções C++11

build: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECT_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lm

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADER_FILES)  # Alterado para .cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: clean

clean:
	rm -f $(EXECUTABLE) $(OBJECT_FILES)
