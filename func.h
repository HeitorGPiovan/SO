#ifndef FUNC_H
#define FUNC_H

#include <vector>
#include <string>

using std::vector;
using std::string;

struct Tarefa {
    int ingresso;
    int duracao;
    int prioridade;
    int id;
    vector<int> eventos;
    string cor;
    int tempoRestante;
};

extern string modoExecucao;
extern string algoritmo;
extern int quantum;

vector<Tarefa> carregarConfiguracao();
void fifo(vector<Tarefa>& tarefas);
void srtf(vector<Tarefa>& tarefas);
void priop(vector<Tarefa>& tarefas);

#endif