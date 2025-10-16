# ================================
# 🧠 Makefile - Simulador de Escalonador
# ================================

# Compilador e flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g

# Nome do executável
TARGET = escalonador

# Busca automaticamente todos os .cpp no diretório
SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)

# ================================
# 🏗️ Regras de compilação
# ================================

all: $(TARGET)

# Linkagem final
$(TARGET): $(OBJ)
	@echo "🔗 Ligando objetos..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)
	@echo "✅ Compilação concluída com sucesso!"

# Compilação dos .cpp em .o
%.o: %.cpp
	@echo "🧩 Compilando $< ..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ================================
# 🧹 Limpeza
# ================================
clean:
	@echo "🧼 Limpando arquivos temporários..."
	rm -f $(OBJ) $(TARGET)

# ================================
# 🚀 Executar o programa
# ================================
run: all
	@echo "🚀 Executando o simulador..."
	./$(TARGET)

# ================================
# 📦 Arquivos auxiliares
# ================================
# Evita que o make confunda arquivos com regras
.PHONY: all clean run
