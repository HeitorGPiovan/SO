#include <iostream>
#include <vector>
#include "func.h"

using namespace std;

int main() {
    string modo;
    //menu
    cout << "Selecione o modo de execucao:\n";
    cout << "1. Passo-a-passo\n";
    cout << "2. Completo\n";
    cout << "Digite 1 ou 2: ";
    getline(cin, modo);

    if (modo != "1" && modo != "2") {
        cerr << "Modo invalido. Usando passo-a-passo por padrao.\n";
        modoExecucao = "passo";
    } else {
        modoExecucao = (modo == "1") ? "passo" : "completo";
    }

    vector<Tarefa> tarefas = carregarConfiguracao();

    //sugestao de parametros padrao
    if (tarefas.empty()) {
        tarefas = {
            {0, 3, 2, 1, {}, "E74C3C", 0},
            {2, 2, 3, 2, {}, "27AE60", 0},
            {5, 4, 1, 3, {}, "F1C40F", 0}
        };
    }

    simulador(tarefas);

    return 0;
}