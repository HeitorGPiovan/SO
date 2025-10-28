# Compilador e flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g

# Nome do executável
TARGET = escalonador

# Busca automaticamente todos os .cpp no diretório
SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

# Linkagem final
$(TARGET): $(OBJ)
	@echo "Ligando objetos..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)
	@echo "Compilação concluída com sucesso!"

# Compilação dos .cpp em .o
%.o: %.cpp
	@echo "Compilando $< ..."
	$(CXX) $(CXXFLAGS) -c $< -o $@


run: all
	@echo "Executando o simulador..."
	./$(TARGET)


.PHONY: all clean run
