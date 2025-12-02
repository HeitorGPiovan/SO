#ifndef FUNC_H
#define FUNC_H

#include <vector>
#include <string>

using std::vector;
using std::string;

typedef struct Tarefa {
    int ingresso;
    int duracao;
    int prioridade;
    double prioridadeDinamica;
    int id;
    vector<int> eventos;
    string cor;
    int tempoRestante;
} Tarefa;

extern string modoExecucao;
extern string algoritmo;
extern int quantum;
extern double alpha;

vector<Tarefa> carregarConfiguracao();
int escalonador(vector<Tarefa>& prontos);   // AQUI: SEM const!!!
void simulador(vector<Tarefa>& tarefasOriginais);

#endif