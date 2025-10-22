#ifndef TAREFA_H
#define TAREFA_H

#include <iostream>
#include <vector>
#include <string>

using namespace std;

typedef struct Tarefa {
    int ingresso;
    int duracao;
    int prioridade;
    int id;
    vector<int> eventos;
    string cor;
    int tempoRestante; //SRTF
} Tarefa;

#endif