#ifndef FUNC_H
#define FUNC_H

#include <vector>
#include <string>
#include <map>

struct FatiaTarefa {
    int id, inicio, fim, duracao;
};

struct Tarefa {
    int id;
    std::string cor;
    int ingresso;
    int duracao;
    int prioridade;
    double prioridadeDinamica;
    int tempoRestante;
    int tempoExecutado = 0;
    bool bloqueada = false;
    std::vector<std::pair<int, std::pair<char, int>>> eventosMutex; // {tempo_rel, {'L','U'}, mutex_id}
};

struct Mutex {
    int dono = -1;
    std::vector<int> filaEspera;
};

extern std::string modoExecucao;
extern std::string algoritmo;
extern int quantum;
extern double alpha;
extern std::map<int, Mutex> mutexes;

void print_gantt(const std::vector<Tarefa>& tarefas, const std::vector<int>& running_task, int current_time);
void print_estado_sistema(const std::vector<Tarefa>& prontos, const std::vector<Tarefa>& pendentes,
                          const std::vector<Tarefa>& bloqueadas, int tempoAtual, int currentTaskId);
void exportarGanttSVG(const std::vector<FatiaTarefa>& fatias, const std::vector<Tarefa>& tarefas, const std::string& nome);
std::vector<Tarefa> carregarConfiguracao();
int escalonador(std::vector<Tarefa>& prontos);
void simulador(std::vector<Tarefa>& tarefasOriginais);

#endif