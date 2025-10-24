#include <iostream>
#include <vector>
#include "func.h"

using namespace std;

int main() {
    string modo;
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

    if (tarefas.empty()) {
        tarefas = {
            {0, 5, 2, 1, {}, "vermelho", 0},
            {2, 2, 3, 2, {}, "verde", 0},
            {5, 4, 1, 3, {}, "amarelo", 0}
        };
    }

    if (algoritmo == "FIFO") {
        fifo(tarefas);
    } else if (algoritmo == "SRTF") {
        srtf(tarefas);
    } else if (algoritmo == "PRIOP") {
        priop(tarefas);
    } else {
        cerr << "Algoritmo desconhecido: " << algoritmo << ". Usando SRTF.\n";
        srtf(tarefas);
    }

    return 0;
}