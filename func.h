#ifndef FUNC_H
#define FUNC_H

#include <vector>
#include <string>

using std::vector;
using std::string;

typedef struct Tarefa {
    int ingresso; //tempo de inicio da tarefa
    int duracao;  //tempo de duracao da tarefa
    int prioridade; //numero de prioridade (quanto menor o valor maior a prioridade)
    int id; //identificador da tarefa
    vector<int> eventos; //lista de eventos (projeto B)
    string cor; //cor das barras utilizadas no grafico de gantt
    int tempoRestante; //tempo restante necessario para a tarefa finalizar
} Tarefa;

extern string modoExecucao;
extern string algoritmo;
extern int quantum;

vector<Tarefa> carregarConfiguracao(); //usada para carregar os parametros escolhidos pelo usuario a partir de um arquivo txt
void simulate(vector<Tarefa>& tarefas);

#endif