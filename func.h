#ifndef FUNC_H
#define FUNC_H

#include <iostream>
#include <vector>
#include <string>
#include "tarefa.h"

using namespace std;

std::string get_color_code(const std::string& cor);
void print_gantt(const std::vector<Tarefa>& tarefas, const std::vector<int>& running_task, int current_time);
void fifo(vector<Tarefa>& tarefas);
void srtf(vector<Tarefa>& tarefas);
void priop(vector<Tarefa>& tarefas);
std::vector<Tarefa> carregarConfiguracao();

#endif