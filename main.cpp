#include <iostream>
#include <vector>
#include "func.h"

using namespace std;

int main()
{

    vector<Tarefa> tarefas = {
        {0, 5, 2, 1, {}, "", 0},
        {2, 3, 1, 2, {}, "", 0},
        {4, 1, 3, 3, {}, "", 0}
    };

    fifo(tarefas);
    
    return 0;
}