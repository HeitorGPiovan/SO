#ifndef FUNC_H
#define FUNC_H

#include <vector>
#include <string>
#include <map>

using std::vector;
using std::string;

// Estrutura usada no gráfico de Gantt
struct FatiaTarefa {
    int id;
    int inicio;
    int fim;
    int duracao;
};

typedef struct Tarefa {
    int ingresso;
    int duracao;
    int prioridade;
    double prioridadeDinamica;
    int id;
    vector<int> eventos;                   // não usado (mantido por compatibilidade)
    string cor;
    int tempoRestante;
    std::vector<std::pair<int, std::pair<char, int>>> eventosMutex; // {tempo_rel, {'L'/'U', mutex_id}}
    int tempoExecutado = 0;
    bool bloqueada = false;
} Tarefa;

extern string modoExecucao;
extern string algoritmo;
extern int quantum;
extern double alpha;

struct Mutex {
    int dono = -1;
    std::vector<int> filaEspera;
};

extern std::map<int, Mutex> mutexes;

// Funções
vector<Tarefa> carregarConfiguracao();
int escalonador(vector<Tarefa>& prontos);
void simulador(vector<Tarefa>& tarefasOriginais);

#endif