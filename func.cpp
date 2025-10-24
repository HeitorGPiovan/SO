#include "func.h"   
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> 
#include <iomanip>
#include <map>

using namespace std;

int quantum;
string algoritmo;
string modoExecucao;

struct FatiaTarefa {
    int id;
    int inicio;
    int fim;
    int duracao;
};

string get_color_code(const string& cor) {
    if (cor == "vermelho") return "\033[41m";
    if (cor == "verde") return "\033[42m";
    if (cor == "amarelo") return "\033[43m";
    if (cor == "azul") return "\033[44m";
    if (cor == "magenta") return "\033[45m";
    if (cor == "ciano") return "\033[46m";
    if (cor == "branco") return "\033[47m";
    return "\033[47m";
}

string get_hex_color(const string& cor) {
    if (cor == "vermelho") return "#FF6B6B";
    if (cor == "verde") return "#4ECDC4";
    if (cor == "amarelo") return "#FFEAA7";
    if (cor == "azul") return "#45B7D1";
    if (cor == "magenta") return "#BB8FCE";
    if (cor == "ciano") return "#85C1E9";
    if (cor == "branco") return "#FFFFFF";
    return "#FFFFFF";
}

void print_estado_sistema(const vector<Tarefa>& tarefasPendentes, const vector<Tarefa>& prontos, const vector<Tarefa>& pendentes, int tempoAtual, int currentTaskId) {
    cout << "\nEstado do Sistema (Tempo: " << tempoAtual << "):\n";
    cout << "Tarefa em execucao: ";
    if (currentTaskId != -1) {
        cout << "T" << currentTaskId;
        for (const auto& t : tarefasPendentes) {
            if (t.id == currentTaskId) {
                cout << " (Tempo restante: " << t.tempoRestante << ")";
                break;
            }
        }
    } else {
        cout << "Nenhuma (CPU ociosa)";
    }
    cout << endl;

    cout << "Fila de prontos: ";
    if (prontos.empty()) {
        cout << "Vazia";
    } else {
        for (const auto& t : prontos) {
            cout << "T" << t.id << " (Restante: " << t.tempoRestante << ") ";
        }
    }
    cout << endl;

    cout << "Tarefas pendentes: ";
    if (pendentes.empty()) {
        cout << "Vazia";
    } else {
        for (const auto& t : pendentes) {
            cout << "T" << t.id << " (Ingresso: " << t.ingresso << ") ";
        }
    }
    cout << endl;

    cout << "Detalhes de todas as tarefas:\n";
    for (const auto& t : tarefasPendentes) {
        cout << "T" << t.id << ": Ingresso=" << t.ingresso << ", Duracao=" << t.duracao
             << ", Prioridade=" << t.prioridade << ", Tempo restante=" << t.tempoRestante
             << ", Cor=" << t.cor << ", Eventos=[";
        for (size_t i = 0; i < t.eventos.size(); ++i) {
            cout << t.eventos[i];
            if (i < t.eventos.size() - 1) cout << ",";
        }
        cout << "]" << endl;
    }
}

void print_gantt(const vector<Tarefa>& tarefas, const vector<int>& running_task, int current_time) {
    vector<Tarefa> sorted_tasks = tarefas;
    sort(sorted_tasks.begin(), sorted_tasks.end(), [](const Tarefa& a, const Tarefa& b) {
        return a.id < b.id;
    });

    cout << "\nGrafico de Gantt (Tempo atual: " << current_time << "):\n";
    cout << "Tempo|";
    int max_time = min(static_cast<int>(running_task.size()), current_time + 1);
    for (int t = 0; t < max_time; ++t) {
        cout << (t % 10);
    }
    cout << endl;

    for (const auto& task : sorted_tasks) {
        cout << "T" << setw(2) << task.id << "  |";
        for (int t = 0; t < max_time; ++t) {
            if (running_task[t] == task.id) {
                string color = get_color_code(task.cor);
                cout << color << "#" << "\033[0m";
            } else if (running_task[t] == -1) {
                cout << ".";
            } else {
                cout << " ";
            }
        }
        cout << endl;
    }
}

void exportarGanttSVG(const vector<FatiaTarefa>& fatias, const vector<Tarefa>& tarefas, const string& nomeAlgoritmo) {
    if (fatias.empty()) {
        cerr << "Nenhuma fatia para exportar!" << endl;
        return;
    }
    
    int tempoMaximo = 0;
    map<int, vector<FatiaTarefa>> fatiasPorTarefa;
    
    for (const auto& fatia : fatias) {
        tempoMaximo = max(tempoMaximo, fatia.fim);
        fatiasPorTarefa[fatia.id].push_back(fatia);
    }
    
    const int LARGURA_SVG = 1400;
    const int ALTURA_SVG = 300 + (static_cast<int>(fatiasPorTarefa.size()) * 45);
    const int ALTURA_BARRA = 35;
    const int ESPACO_VERTICAL = 10;
    const int MARGEM_ESQUERDA = 80;
    const int MARGEM_DIREITA = 60;
    
    int larguraDisponivel = LARGURA_SVG - MARGEM_ESQUERDA - MARGEM_DIREITA;
    double escalaTempo = larguraDisponivel / static_cast<double>(tempoMaximo);
    
    string nomeArquivo = nomeAlgoritmo + "_gantt.svg";
    ofstream svg(nomeArquivo);
    
    if (!svg.is_open()) {
        cerr << "Erro ao criar arquivo SVG: " << nomeArquivo << endl;
        return;
    }
    
    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    svg << "<svg width=\"" << LARGURA_SVG << "\" height=\"" << ALTURA_SVG 
        << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    
    svg << "<defs>\n<style type=\"text/css\">\n";
    svg << "text { font-family: 'Segoe UI', sans-serif; }\n";
    svg << ".titulo { font-size: 22px; font-weight: bold; fill: #2c3e50; }\n";
    svg << ".tempo { font-size: 12px; font-weight: bold; fill: #34495e; }\n";
    svg << ".tarefa { font-size: 13px; font-weight: bold; fill: #2c3e50; text-anchor: middle; }\n";
    svg << ".eixo { stroke: #2c3e50; stroke-width: 2; }\n";
    svg << ".grid { stroke: #bdc3c7; stroke-width: 1; stroke-dasharray: 3,3; opacity: 0.7; }\n";
    svg << ".ingresso { stroke: #e74c3c; stroke-width: 2.5; stroke-dasharray: 5,5; marker-end: url(#arrow); }\n";
    svg << "</style>\n";
    
    svg << "<marker id=\"arrow\" markerWidth=\"10\" markerHeight=\"7\" refX=\"9\" refY=\"3.5\" orient=\"auto\">\n";
    svg << "<polygon points=\"0 0, 10 3.5, 0 7\" fill=\"#e74c3c\" stroke=\"none\"/>\n";
    svg << "</marker>\n";
    svg << "</defs>\n";
    
    int yTitulo = 30;
    svg << "<text x=\"" << (LARGURA_SVG/2) << "\" y=\"" << yTitulo 
        << "\" class=\"titulo\" text-anchor=\"middle\">Grafico de Gantt - " << nomeAlgoritmo << "</text>\n";
    
    int yEixo = 70;
    svg << "<line x1=\"" << MARGEM_ESQUERDA << "\" y1=\"" << yEixo 
        << "\" x2=\"" << (LARGURA_SVG - MARGEM_DIREITA) << "\" y2=\"" << yEixo 
        << "\" class=\"eixo\"/>\n";
    
    int intervalo = max(1, tempoMaximo / 12);
    for (int t = 0; t <= tempoMaximo; t += intervalo) {
        int x = MARGEM_ESQUERDA + static_cast<int>(t * escalaTempo);
        svg << "<line x1=\"" << x << "\" y1=\"" << (yEixo - 8) 
            << "\" x2=\"" << x << "\" y2=\"" << (yEixo + 8) << "\" class=\"eixo\"/>\n";
        svg << "<text x=\"" << x << "\" y=\"" << (yEixo + 22) 
            << "\" class=\"tempo\" text-anchor=\"middle\">" << t << "</text>\n";
        svg << "<line x1=\"" << x << "\" y1=\"" << yEixo 
            << "\" x2=\"" << x << "\" y2=\"" << (yEixo + 20) << "\" class=\"grid\"/>\n";
    }
    
    int yAtual = yEixo + 35;
    vector<string> coresPadrao = {"vermelho", "verde", "amarelo", "azul", "magenta", "ciano", "branco"};
    int corIndex = 0;
    
    for (const auto& [idTarefa, fatiasTarefa] : fatiasPorTarefa) {
        string corNome;
        string corHex;
        int ingresso = 0;
        for (const auto& t : tarefas) {
            if (t.id == idTarefa) {
                corNome = t.cor;
                ingresso = t.ingresso;
                break;
            }
        }
        if (corNome.empty()) {
            corNome = coresPadrao[corIndex % coresPadrao.size()];
            corIndex++;
        }
        corHex = get_hex_color(corNome);
        
        svg << "<text x=\"10\" y=\"" << (yAtual + 22) 
            << "\" font-size=\"14px\" font-weight=\"bold\" fill=\"#2c3e50\">T" << idTarefa << "</text>\n";
        
        for (const auto& fatia : fatiasTarefa) {
            int xInicio = MARGEM_ESQUERDA + static_cast<int>(fatia.inicio * escalaTempo);
            int larguraBarra = static_cast<int>(fatia.duracao * escalaTempo);
            if (larguraBarra < 3) larguraBarra = 3;
            
            svg << "<rect x=\"" << xInicio << "\" y=\"" << yAtual 
                << "\" width=\"" << larguraBarra << "\" height=\"" << ALTURA_BARRA 
                << "\" rx=\"4\" ry=\"4\" fill=\"" << corHex 
                << "\" stroke=\"#2c3e50\" stroke-width=\"1.5\" opacity=\"0.95\"/>\n";
            
            if (larguraBarra >= 25) {
                int xTexto = xInicio + (larguraBarra / 2);
                svg << "<text x=\"" << xTexto << "\" y=\"" << (yAtual + 22) 
                    << "\" class=\"tarefa\">" << fatia.duracao << "</text>\n";
            }
        }
        
        int xIngresso = MARGEM_ESQUERDA + static_cast<int>(ingresso * escalaTempo);
        svg << "<line x1=\"" << xIngresso << "\" y1=\"" << (yAtual + ALTURA_BARRA + 2) 
            << "\" x2=\"" << xIngresso << "\" y2=\"" << (yAtual + ALTURA_BARRA + 12) 
            << "\" class=\"ingresso\"/>\n";
        svg << "<text x=\"" << (xIngresso - 8) << "\" y=\"" << (yAtual + ALTURA_BARRA + 28) 
            << "\" font-size=\"11px\" fill=\"#e74c3c\" text-anchor=\"end\">" << ingresso << "</text>\n";
        
        yAtual += ALTURA_BARRA + ESPACO_VERTICAL;
    }
    
    int yLegenda = yEixo + 15;
    svg << "<text x=\"" << (LARGURA_SVG - MARGEM_DIREITA + 15) << "\" y=\"" << (yLegenda + 18) 
        << "\" font-size=\"14px\" font-weight=\"bold\" fill=\"#2c3e50\">Legenda:</text>\n";
    svg << "<text x=\"" << (LARGURA_SVG - MARGEM_DIREITA + 15) << "\" y=\"" << (yLegenda + 35) 
        << "\" font-size=\"12px\" fill=\"#34495e\">Fatias de execucao</text>\n";
    svg << "<text x=\"" << (LARGURA_SVG - MARGEM_DIREITA + 15) << "\" y=\"" << (yLegenda + 52) 
        << "\" font-size=\"12px\" fill=\"#e74c3c\">Tempo de ingresso</text>\n";
    
    svg << "</svg>\n";
    svg.close();
    
    cout << "Grafico Gantt salvo como: " << nomeArquivo << endl;
}

void fifo(vector<Tarefa>& tarefas) {
    if (modoExecucao == "passo") {
        cout << "Executando FIFO (modo passo-a-passo)..." << endl;
    } else {
        cout << "Executando FIFO (modo completo)..." << endl;
    }

    sort(tarefas.begin(), tarefas.end(), [](const Tarefa& a, const Tarefa& b) {
        return a.ingresso < b.ingresso;
    });

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;
    vector<Tarefa> tarefasPendentes = tarefas;
    vector<int> running_task;
    vector<FatiaTarefa> fatias;

    vector<string> coresPadrao = {"vermelho", "verde", "amarelo", "azul", "magenta", "ciano", "branco"};
    for (size_t i = 0; i < tarefasPendentes.size(); ++i) {
        if (tarefasPendentes[i].cor.empty()) {
            tarefasPendentes[i].cor = coresPadrao[i % coresPadrao.size()];
        }
        tarefasPendentes[i].tempoRestante = tarefasPendentes[i].duracao;
    }

    if (modoExecucao == "passo") {
        cout << "\nOrdem de execucao (FIFO):\n";
    }

    vector<Tarefa> prontos;
    vector<Tarefa> pendentes = tarefasPendentes;

    while (!pendentes.empty() || !prontos.empty()) {
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual) {
            prontos.push_back(pendentes.front());
            pendentes.erase(pendentes.begin());
        }

        if (prontos.empty()) {
            running_task.push_back(-1);
            if (modoExecucao == "passo") {
                print_gantt(tarefasPendentes, running_task, tempoAtual);
                print_estado_sistema(tarefasPendentes, prontos, pendentes, tempoAtual, -1);
                cout << "CPU ociosa - Pressione Enter para proximo tick..." << endl;
                cin.get();
            }
            tempoAtual++;
            continue;
        }

        Tarefa& atual = prontos.front();
        int duracaoFatia = 1;

        int inicio = tempoAtual;
        int fim = tempoAtual + duracaoFatia;
        int espera = inicio - atual.ingresso;

        if (atual.tempoRestante == atual.duracao) {
            esperaTotal += espera;
        }

        if (modoExecucao == "passo") {
            cout << "Tarefa " << setw(2) << atual.id
                 << " | Ingresso: " << setw(2) << atual.ingresso
                 << " | Duracao fatia: " << setw(2) << duracaoFatia
                 << " | Inicio: " << setw(2) << inicio
                 << " | Fim: " << setw(2) << fim
                 << " | Espera: " << setw(2) << espera
                 << " | Retorno: " << setw(2) << (fim - atual.ingresso)
                 << endl;
        }

        running_task.push_back(atual.id);
        atual.tempoRestante -= duracaoFatia;

        fatias.push_back({atual.id, inicio, fim, duracaoFatia});

        if (modoExecucao == "passo") {
            print_gantt(tarefasPendentes, running_task, tempoAtual);
            print_estado_sistema(tarefasPendentes, prontos, pendentes, tempoAtual, atual.id);
            cout << "Executando T" << atual.id << " - Pressione Enter para proximo tick..." << endl;
            cin.get();
        }

        tempoAtual = fim;

        if (atual.tempoRestante <= 0) {
            retornoTotal += (tempoAtual - atual.ingresso);
            prontos.erase(prontos.begin());
        }
    }

    vector<FatiaTarefa> fatiasMescladas;
    if (!fatias.empty()) {
        int current_id = fatias[0].id;
        int inicio = fatias[0].inicio;
        int duracao = fatias[0].duracao;
        for (size_t i = 1; i < fatias.size(); ++i) {
            if (fatias[i].id == current_id && fatias[i].inicio == fatias[i-1].fim) {
                duracao += fatias[i].duracao;
            } else {
                fatiasMescladas.push_back({current_id, inicio, inicio + duracao, duracao});
                current_id = fatias[i].id;
                inicio = fatias[i].inicio;
                duracao = fatias[i].duracao;
            }
        }
        fatiasMescladas.push_back({current_id, inicio, inicio + duracao, duracao});
    }

    exportarGanttSVG(fatiasMescladas, tarefasPendentes, "FIFO");

    if (modoExecucao == "completo") {
        print_gantt(tarefasPendentes, running_task, tempoAtual);
    }

    int n = tarefas.size();
    cout << fixed << setprecision(2);
    cout << "\nRESULTADO FINAL:\n";
    cout << "Tempo medio de espera: " << (esperaTotal / n) << endl;
    cout << "Tempo medio de retorno: " << (retornoTotal / n) << endl;
}

void srtf(vector<Tarefa>& tarefas) {
    if (modoExecucao == "passo") {
        cout << "Executando SRTF (modo passo-a-passo)..." << endl;
    } else {
        cout << "Executando SRTF (modo completo)..." << endl;
    }

    vector<Tarefa> tarefasPendentes = tarefas;
    sort(tarefasPendentes.begin(), tarefasPendentes.end(), [](const Tarefa& a, const Tarefa& b) {
        return a.ingresso < b.ingresso;
    });

    vector<string> coresPadrao = {"vermelho", "verde", "amarelo", "azul", "magenta", "ciano", "branco"};
    for (size_t i = 0; i < tarefasPendentes.size(); ++i) {
        if (tarefasPendentes[i].cor.empty()) {
            tarefasPendentes[i].cor = coresPadrao[i % coresPadrao.size()];
        }
        tarefasPendentes[i].tempoRestante = tarefasPendentes[i].duracao;
    }

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;
    vector<Tarefa> pendentes = tarefasPendentes;
    vector<Tarefa> prontos;
    vector<int> running_task;
    vector<FatiaTarefa> fatias;

    if (modoExecucao == "passo") {
        cout << "\nOrdem de execucao (SRTF):\n";
    }

    while (!pendentes.empty() || !prontos.empty()) {
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual) {
            prontos.push_back(pendentes.front());
            pendentes.erase(pendentes.begin());
        }

        if (prontos.empty()) {
            running_task.push_back(-1);
            if (modoExecucao == "passo") {
                print_gantt(tarefasPendentes, running_task, tempoAtual);
                print_estado_sistema(tarefasPendentes, prontos, pendentes, tempoAtual, -1);
                cout << "CPU ociosa - Pressione Enter para proximo tick..." << endl;
                cin.get();
            }
            tempoAtual++;
            continue;
        }

        sort(prontos.begin(), prontos.end(), [](const Tarefa& a, const Tarefa& b) {
            if (a.tempoRestante != b.tempoRestante) return a.tempoRestante < b.tempoRestante;
            return a.ingresso < b.ingresso;
        });

        Tarefa& atual = prontos.front();
        int duracaoFatia = 1;

        int inicio = tempoAtual;
        int fim = tempoAtual + duracaoFatia;
        int espera = inicio - atual.ingresso;

        if (atual.tempoRestante == atual.duracao) {
            esperaTotal += espera;
        }

        if (modoExecucao == "passo") {
            cout << "Tarefa " << setw(2) << atual.id
                 << " | Ingresso: " << setw(2) << atual.ingresso
                 << " | Duracao fatia: " << setw(2) << duracaoFatia
                 << " | Inicio: " << setw(2) << inicio
                 << " | Fim: " << setw(2) << fim
                 << " | Espera: " << setw(2) << espera
                 << " | Retorno: " << setw(2) << (fim - atual.ingresso)
                 << endl;
        }

        running_task.push_back(atual.id);
        atual.tempoRestante -= duracaoFatia;

        fatias.push_back({atual.id, inicio, fim, duracaoFatia});

        if (modoExecucao == "passo") {
            print_gantt(tarefasPendentes, running_task, tempoAtual);
            print_estado_sistema(tarefasPendentes, prontos, pendentes, tempoAtual, atual.id);
            cout << "Executando T" << atual.id << " - Pressione Enter para proximo tick..." << endl;
            cin.get();
        }

        tempoAtual = fim;

        if (atual.tempoRestante <= 0) {
            retornoTotal += (tempoAtual - atual.ingresso);
            prontos.erase(prontos.begin());
        }
    }

    vector<FatiaTarefa> fatiasMescladas;
    if (!fatias.empty()) {
        int current_id = fatias[0].id;
        int inicio = fatias[0].inicio;
        int duracao = fatias[0].duracao;
        for (size_t i = 1; i < fatias.size(); ++i) {
            if (fatias[i].id == current_id && fatias[i].inicio == fatias[i-1].fim) {
                duracao += fatias[i].duracao;
            } else {
                fatiasMescladas.push_back({current_id, inicio, inicio + duracao, duracao});
                current_id = fatias[i].id;
                inicio = fatias[i].inicio;
                duracao = fatias[i].duracao;
            }
        }
        fatiasMescladas.push_back({current_id, inicio, inicio + duracao, duracao});
    }

    exportarGanttSVG(fatiasMescladas, tarefasPendentes, "SRTF");

    if (modoExecucao == "completo") {
        print_gantt(tarefasPendentes, running_task, tempoAtual);
    }

    int n = tarefas.size();
    cout << fixed << setprecision(2);
    cout << "\nRESULTADO FINAL:\n";
    cout << "Tempo medio de espera: " << (esperaTotal / n) << endl;
    cout << "Tempo medio de retorno: " << (retornoTotal / n) << endl;
}

void priop(vector<Tarefa>& tarefas) {
    if (modoExecucao == "passo") {
        cout << "Executando PRIOP (modo passo-a-passo)..." << endl;
    } else {
        cout << "Executando PRIOP (modo completo)..." << endl;
    }

    vector<Tarefa> tarefasPendentes = tarefas;
    sort(tarefasPendentes.begin(), tarefasPendentes.end(), [](const Tarefa& a, const Tarefa& b) {
        return a.ingresso < b.ingresso;
    });

    vector<string> coresPadrao = {"vermelho", "verde", "amarelo", "azul", "magenta", "ciano", "branco"};
    for (size_t i = 0; i < tarefasPendentes.size(); ++i) {
        if (tarefasPendentes[i].cor.empty()) {
            tarefasPendentes[i].cor = coresPadrao[i % coresPadrao.size()];
        }
        tarefasPendentes[i].tempoRestante = tarefasPendentes[i].duracao;
    }

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;
    vector<Tarefa> pendentes = tarefasPendentes;
    vector<Tarefa> prontos;
    vector<int> running_task;
    vector<FatiaTarefa> fatias;

    if (modoExecucao == "passo") {
        cout << "\nOrdem de execucao (PRIOP):\n";
    }

    while (!pendentes.empty() || !prontos.empty()) {
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual) {
            prontos.push_back(pendentes.front());
            pendentes.erase(pendentes.begin());
        }

        if (prontos.empty()) {
            running_task.push_back(-1);
            if (modoExecucao == "passo") {
                print_gantt(tarefasPendentes, running_task, tempoAtual);
                print_estado_sistema(tarefasPendentes, prontos, pendentes, tempoAtual, -1);
                cout << "CPU ociosa - Pressione Enter para proximo tick..." << endl;
                cin.get();
            }
            tempoAtual++;
            continue;
        }

        sort(prontos.begin(), prontos.end(), [](const Tarefa& a, const Tarefa& b) {
            if (a.prioridade != b.prioridade) return a.prioridade < b.prioridade;
            return a.ingresso < b.ingresso;
        });

        Tarefa& atual = prontos.front();
        int duracaoFatia = 1;

        int inicio = tempoAtual;
        int fim = tempoAtual + duracaoFatia;
        int espera = inicio - atual.ingresso;

        if (atual.tempoRestante == atual.duracao) {
            esperaTotal += espera;
        }

        if (modoExecucao == "passo") {
            cout << "Tarefa " << setw(2) << atual.id
                 << " | Ingresso: " << setw(2) << atual.ingresso
                 << " | Duracao fatia: " << setw(2) << duracaoFatia
                 << " | Inicio: " << setw(2) << inicio
                 << " | Fim: " << setw(2) << fim
                 << " | Espera: " << setw(2) << espera
                 << " | Retorno: " << setw(2) << (fim - atual.ingresso)
                 << endl;
        }

        running_task.push_back(atual.id);
        atual.tempoRestante -= duracaoFatia;

        fatias.push_back({atual.id, inicio, fim, duracaoFatia});

        if (modoExecucao == "passo") {
            print_gantt(tarefasPendentes, running_task, tempoAtual);
            print_estado_sistema(tarefasPendentes, prontos, pendentes, tempoAtual, atual.id);
            cout << "Executando T" << atual.id << " - Pressione Enter para proximo tick..." << endl;
            cin.get();
        }

        tempoAtual = fim;

        if (atual.tempoRestante <= 0) {
            retornoTotal += (tempoAtual - atual.ingresso);
            prontos.erase(prontos.begin());
        }
    }

    vector<FatiaTarefa> fatiasMescladas;
    if (!fatias.empty()) {
        int current_id = fatias[0].id;
        int inicio = fatias[0].inicio;
        int duracao = fatias[0].duracao;
        for (size_t i = 1; i < fatias.size(); ++i) {
            if (fatias[i].id == current_id && fatias[i].inicio == fatias[i-1].fim) {
                duracao += fatias[i].duracao;
            } else {
                fatiasMescladas.push_back({current_id, inicio, inicio + duracao, duracao});
                current_id = fatias[i].id;
                inicio = fatias[i].inicio;
                duracao = fatias[i].duracao;
            }
        }
        fatiasMescladas.push_back({current_id, inicio, inicio + duracao, duracao});
    }

    exportarGanttSVG(fatiasMescladas, tarefasPendentes, "PRIOP");

    if (modoExecucao == "completo") {
        print_gantt(tarefasPendentes, running_task, tempoAtual);
    }

    int n = tarefas.size();
    cout << fixed << setprecision(2);
    cout << "\nRESULTADO FINAL:\n";
    cout << "Tempo medio de espera: " << (esperaTotal / n) << endl;
    cout << "Tempo medio de retorno: " << (retornoTotal / n) << endl;
}

vector<Tarefa> carregarConfiguracao() {
    const string nomeArquivo = "configuracao.txt";
    ifstream arquivo(nomeArquivo);
    vector<Tarefa> tarefas;

    if (!arquivo.is_open()) {
        cerr << "Arquivo configuracao.txt nao encontrado. Usando dados padrao." << endl;
        return tarefas;
    }

    string linha;
    bool primeiraLinha = true;

    while (getline(arquivo, linha)) {
        if (linha.empty()) continue;

        stringstream ss(linha);
        string campo;

        if (primeiraLinha) {
            getline(ss, algoritmo, ';');
            getline(ss, campo, ';');
            quantum = stoi(campo);
            primeiraLinha = false;
        } else {
            Tarefa t;
            getline(ss, campo, ';'); t.id = stoi(campo);
            getline(ss, t.cor, ';');
            getline(ss, campo, ';'); t.ingresso = stoi(campo);
            getline(ss, campo, ';'); t.duracao = stoi(campo);
            getline(ss, campo, ';'); t.prioridade = stoi(campo);
            getline(ss, campo, ';');

            stringstream eventosStream(campo);
            string evento;
            while (getline(eventosStream, evento, ',')) {
                if (!evento.empty())
                    t.eventos.push_back(stoi(evento));
            }

            t.tempoRestante = t.duracao;
            tarefas.push_back(t);
        }
    }

    arquivo.close();

    vector<string> coresPadrao = {"vermelho", "verde", "amarelo", "azul", "magenta", "ciano", "branco"};
    for (size_t i = 0; i < tarefas.size(); ++i) {
        if (tarefas[i].cor.empty()) {
            tarefas[i].cor = coresPadrao[i % coresPadrao.size()];
        }
    }

    return tarefas;
}