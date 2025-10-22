#ifndef FUNC_H
#define FUNC_H

#include <iostream>
#include <vector>
#include <string>
#include "tarefa.h"
#include <climits>  // Para INT_MAX

using namespace std;

struct FatiaTarefa {
    int id;
    int inicio;
    int fim;
    int duracao;
};

std::string get_color_code(const std::string& cor);
std::string get_hex_color(const std::string& cor);
void print_gantt(const std::vector<Tarefa>& tarefas, const std::vector<int>& running_task, int current_time);
void exportarGanttSVG(const std::vector<FatiaTarefa>& fatias, const std::vector<Tarefa>& tarefas, const std::string& nomeAlgoritmo);
void fifo(vector<Tarefa>& tarefas);
void srtf(vector<Tarefa>& tarefas);
void priop(vector<Tarefa>& tarefas);
std::vector<Tarefa> carregarConfiguracao();

#endif