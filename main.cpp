#include <iostream>
#include <vector>
#include "func.h"

using namespace std;

int main() {
    string modo;
    cout << "Selecione o modo de execucao:\n1. Passo-a-passo\n2. Completo\nDigite 1 ou 2: ";
    getline(cin, modo);

    modoExecucao = (modo == "2") ? "completo" : "passo";

    vector<Tarefa> tarefas = carregarConfiguracao();
    if (tarefas.empty()) {
        tarefas = {
            {0, 8, 10, 10.0, 1, {}, "E74C3C", 8},
            {1, 6,  5,  5.0, 2, {}, "27AE60", 6},
            {3, 4,  8,  8.0, 3, {}, "F1C40F", 4}
        };
    }

    simulador(tarefas);
    return 0;
}