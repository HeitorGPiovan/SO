#include "func.h"   
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> 
#include <iomanip>   

int quantum;
std::string algoritmo;

std::string get_color_code(const std::string& cor) {
    if (cor == "vermelho") return "\033[41m";
    if (cor == "verde") return "\033[42m";
    if (cor == "amarelo") return "\033[43m";
    if (cor == "azul") return "\033[44m";
    if (cor == "magenta") return "\033[45m";
    if (cor == "ciano") return "\033[46m";
    if (cor == "branco") return "\033[47m";
    return "\033[40m"; // Default to black
}

void print_gantt(const std::vector<Tarefa>& tarefas, const std::vector<int>& running_task, int current_time) {
    // Sort tasks by ID for consistent row order
    std::vector<Tarefa> sorted_tasks = tarefas;
    std::sort(sorted_tasks.begin(), sorted_tasks.end(), [](const Tarefa& a, const Tarefa& b) {
        return a.id < b.id;
    });

    std::cout << "\nGrafico de Gantt (Tempo atual: " << current_time << "):\n";
    std::cout << "Tempo|";
    for (size_t t = 0; t < running_task.size() && t <= static_cast<size_t>(current_time); ++t) {
        std::cout << (t % 10);
    }
    std::cout << std::endl;

    for (const auto& task : sorted_tasks) {
        std::cout << "T" << std::setw(2) << task.id << "  |";
        for (size_t t = 0; t < running_task.size() && t <= static_cast<size_t>(current_time); ++t) {
            if (running_task[t] == task.id) {
                std::string color = get_color_code(task.cor);
                std::cout << color << "█" << "\033[0m";
            } else {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
}

void fifo(vector<Tarefa>& tarefas) {
    cout << "Executando FIFO..." << endl;

    // Ordena as tarefas pelo tempo de ingresso
    sort(tarefas.begin(), tarefas.end(), [](const Tarefa& a, const Tarefa& b) {
        return a.ingresso < b.ingresso;
    });

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;
    vector<Tarefa> tarefasPendentes = tarefas;
    vector<int> running_task;

    for (auto& t : tarefasPendentes) {
        t.tempoRestante = t.duracao;
    }

    cout << "\nOrdem de execucao (FIFO):\n";

    size_t current_task_index = 0;
    int current_fatia = 0;

    while (current_task_index < tarefasPendentes.size() || !running_task.empty()) {
        // If no tasks are ready, advance to the next task's ingresso
        if (current_task_index < tarefasPendentes.size() && tempoAtual < tarefasPendentes[current_task_index].ingresso) {
            running_task.push_back(-1); // CPU idle
            print_gantt(tarefas, running_task, tempoAtual);
            cout << "Pressione Enter para avancar o tempo..." << endl;
            cin.get();
            tempoAtual++;
            continue;
        }

        if (current_task_index >= tarefasPendentes.size()) break;

        auto& t = tarefasPendentes[current_task_index];
        int inicio = tempoAtual;
        int duracaoFatia = min(t.tempoRestante, quantum);
        if (current_fatia < duracaoFatia) {
            running_task.push_back(t.id);
            t.tempoRestante--;
            current_fatia++;

            int fim = tempoAtual + 1;
            int espera = inicio - t.ingresso;
            int retorno = fim - t.ingresso;

            if (t.tempoRestante == t.duracao - 1) {
                esperaTotal += espera;
            }

            cout << "Tarefa " << setw(2) << t.id
                 << " | Ingresso: " << setw(2) << t.ingresso
                 << " | Duracao fatia: " << setw(2) << 1
                 << " | Inicio: " << setw(2) << inicio
                 << " | Fim: " << setw(2) << fim
                 << " | Espera: " << setw(2) << espera
                 << " | Retorno: " << setw(2) << retorno
                 << endl;

            print_gantt(tarefas, running_task, tempoAtual);
            cout << "Pressione Enter para avancar o tempo..." << endl;
            cin.get();
            tempoAtual++;
        }

        if (current_fatia >= duracaoFatia || t.tempoRestante == 0) {
            if (t.tempoRestante == 0) {
                retornoTotal += (tempoAtual - t.ingresso);
                current_task_index++;
            }
            current_fatia = 0;
        }
    }

    int n = tarefas.size();
    cout << fixed << setprecision(2);
    cout << "\nTempo medio de espera: " << (esperaTotal / n)
         << "\nTempo medio de retorno: " << (retornoTotal / n)
         << endl;
}

void srtf(vector<Tarefa>& tarefas) {
    cout << "Executando SRTF..." << endl;

    vector<Tarefa> tarefasPendentes = tarefas;
    sort(tarefasPendentes.begin(), tarefasPendentes.end(), [](const Tarefa& a, const Tarefa& b) {
        return a.ingresso < b.ingresso;
    });

    for (auto& t : tarefasPendentes) {
        t.tempoRestante = t.duracao;
    }

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;
    vector<Tarefa> pendentes = tarefasPendentes;
    vector<Tarefa> prontos;
    vector<int> running_task;

    cout << "\nOrdem de execucao (SRTF):\n";

    while (!pendentes.empty() || !prontos.empty()) {
        // Add tasks that have arrived to prontos
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual) {
            prontos.push_back(pendentes.front());
            pendentes.erase(pendentes.begin());
        }

        if (prontos.empty()) {
            if (!pendentes.empty()) {
                running_task.push_back(-1);
                print_gantt(tarefas, running_task, tempoAtual);
                cout << "Pressione Enter para avancar o tempo..." << endl;
                cin.get();
                tempoAtual++;
            }
            continue;
        }

        sort(prontos.begin(), prontos.end(), [](const Tarefa& a, const Tarefa& b) {
            if (a.tempoRestante != b.tempoRestante) {
                return a.tempoRestante < b.tempoRestante;
            }
            return a.ingresso < b.ingresso;
        });

        Tarefa& atual = prontos.front();
        int inicio = tempoAtual;
        int duracaoFatia = 1; // Process one time unit
        int fim = tempoAtual + duracaoFatia;
        int espera = inicio - atual.ingresso;
        int retorno = fim - atual.ingresso;

        if (atual.tempoRestante == atual.duracao) {
            esperaTotal += espera;
        }

        cout << "Tarefa " << setw(2) << atual.id
             << " | Ingresso: " << setw(2) << atual.ingresso
             << " | Duracao fatia: " << setw(2) << duracaoFatia
             << " | Inicio: " << setw(2) << inicio
             << " | Fim: " << setw(2) << fim
             << " | Espera: " << setw(2) << espera
             << " | Retorno: " << setw(2) << retorno
             << endl;

        running_task.push_back(atual.id);
        atual.tempoRestante--;

        print_gantt(tarefas, running_task, tempoAtual);
        cout << "Pressione Enter para avancar o tempo..." << endl;
        cin.get();
        tempoAtual++;

        if (atual.tempoRestante == 0) {
            retornoTotal += retorno;
            prontos.erase(prontos.begin());
        }
    }

    int n = tarefas.size();
    cout << fixed << setprecision(2);
    cout << "\nTempo medio de espera: " << (esperaTotal / n)
         << "\nTempo medio de retorno: " << (retornoTotal / n)
         << endl;
}

void priop(vector<Tarefa>& tarefas) {
    cout << "Executando PRIOP..." << endl;

    vector<Tarefa> tarefasPendentes = tarefas;
    sort(tarefasPendentes.begin(), tarefasPendentes.end(), [](const Tarefa& a, const Tarefa& b) {
        return a.ingresso < b.ingresso;
    });

    for (auto& t : tarefasPendentes) {
        t.tempoRestante = t.duracao;
    }

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;
    vector<Tarefa> pendentes = tarefasPendentes;
    vector<Tarefa> prontos;
    vector<int> running_task;

    cout << "\nOrdem de execucao (PRIOP):\n";

    while (!pendentes.empty() || !prontos.empty()) {
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual) {
            prontos.push_back(pendentes.front());
            pendentes.erase(pendentes.begin());
        }

        if (prontos.empty()) {
            if (!pendentes.empty()) {
                running_task.push_back(-1);
                print_gantt(tarefas, running_task, tempoAtual);
                cout << "Pressione Enter para avancar o tempo..." << endl;
                cin.get();
                tempoAtual++;
            }
            continue;
        }

        sort(prontos.begin(), prontos.end(), [](const Tarefa& a, const Tarefa& b) {
            if (a.prioridade != b.prioridade) {
                return a.prioridade < b.prioridade;
            }
            return a.ingresso < b.ingresso;
        });

        Tarefa& atual = prontos.front();
        int duracaoFatia = 1; // Process one time unit
        if (!pendentes.empty()) {
            duracaoFatia = min(duracaoFatia, pendentes.front().ingresso - tempoAtual);
        }
        duracaoFatia = min(duracaoFatia, quantum);

        int inicio = tempoAtual;
        int fim = tempoAtual + duracaoFatia;
        int espera = inicio - atual.ingresso;
        int retorno = fim - atual.ingresso;

        if (atual.tempoRestante == atual.duracao) {
            esperaTotal += espera;
        }

        cout << "Tarefa " << setw(2) << atual.id
             << " | Ingresso: " << setw(2) << atual.ingresso
             << " | Duracao fatia: " << setw(2) << duracaoFatia
             << " | Inicio: " << setw(2) << inicio
             << " | Fim: " << setw(2) << fim
             << " | Espera: " << setw(2) << espera
             << " | Retorno: " << setw(2) << retorno
             << endl;

        running_task.push_back(atual.id);
        atual.tempoRestante--;

        print_gantt(tarefas, running_task, tempoAtual);
        cout << "Pressione Enter para avancar o tempo..." << endl;
        cin.get();
        tempoAtual++;

        if (atual.tempoRestante == 0) {
            retornoTotal += retorno;
            prontos.erase(prontos.begin());
        }
    }

    int n = tarefas.size();
    cout << fixed << setprecision(2);
    cout << "\nTempo medio de espera: " << (esperaTotal / n)
         << "\nTempo medio de retorno: " << (retornoTotal / n)
         << endl;
}

std::vector<Tarefa> carregarConfiguracao() {
    const std::string nomeArquivo = "configuracao.txt";
    std::ifstream arquivo(nomeArquivo);
    std::vector<Tarefa> tarefas;

    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << nomeArquivo << std::endl;
        return tarefas;
    }

    std::string linha;
    bool primeiraLinha = true;

    while (std::getline(arquivo, linha)) {
        if (linha.empty()) continue;

        std::stringstream ss(linha);
        std::string campo;

        if (primeiraLinha) {
            std::getline(ss, algoritmo, ';');
            std::getline(ss, campo, ';');
            quantum = std::stoi(campo);
            primeiraLinha = false;
        } else {
            Tarefa t;
            std::getline(ss, campo, ';'); t.id = std::stoi(campo);
            std::getline(ss, t.cor, ';');
            std::getline(ss, campo, ';'); t.ingresso = std::stoi(campo);
            std::getline(ss, campo, ';'); t.duracao = std::stoi(campo);
            std::getline(ss, campo, ';'); t.prioridade = std::stoi(campo);
            std::getline(ss, campo, ';');

            std::stringstream eventosStream(campo);
            std::string evento;
            while (std::getline(eventosStream, evento, ',')) {
                if (!evento.empty())
                    t.eventos.push_back(std::stoi(evento));
            }

            t.tempoRestante = t.duracao;
            tarefas.push_back(t);
        }
    }

    arquivo.close();
    return tarefas;
}