#include <iostream>
#include <vector>
#include "func.h"

using namespace std;

int main()
{
    // Carrega tarefas do arquivo configuracao.txt
    vector<Tarefa> tarefas = carregarConfiguracao();

   if(tarefas.empty()) {
        tarefas = {
            {0, 5, 2, 1, {}, "", 0},
            {2, 2, 3, 2, {}, "", 0},
            {1, 4, 1, 3, {}, "", 0}
        };
    }

    fifo(tarefas);
    
    return 0;
}