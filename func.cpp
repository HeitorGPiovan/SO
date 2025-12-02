#include "func.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <map>
#include <iterator> // for find_if

using namespace std;

// Variáveis globais de configuração
int quantum;         // quantum para algoritmos preemptivos
string algoritmo;    // nome do algoritmo (FIFO, SRTF, PRIOP)
string modoExecucao; // "passo" ou "completo"

// Estrutura para fatias do gráfico de Gantt
struct FatiaTarefa
{
    int id;      // ID da tarefa
    int inicio;  // tempo inicial da fatia
    int fim;     // tempo final da fatia
    int duracao; // duração da fatia
};

// Retorna código ANSI para cor no terminal
string get_color_code(const string &cor)
{
    if (cor == "vermelho") return "\033[41m";
    if (cor == "verde") return "\033[42m";
    if (cor == "amarelo") return "\033[43m";
    if (cor == "azul") return "\033[44m";
    if (cor == "magenta") return "\033[45m";
    if (cor == "ciano") return "\033[46m";
    if (cor == "branco") return "\033[47m";
    return "\033[47m"; // padrão
}

// Retorna cor em hexadecimal para SVG
string get_hex_color(const string &cor)
{
    if (cor == "vermelho") return "#FF6B6B";
    if (cor == "verde") return "#4ECDC4";
    if (cor == "amarelo") return "#FFEAA7";
    if (cor == "azul") return "#45B7D1";
    if (cor == "magenta") return "#BB8FCE";
    if (cor == "ciano") return "#85C1E9";
    if (cor == "branco") return "#FFFFFF";
    return "#FFFFFF"; // padrão
}

// Atualiza tempoRestante em tarefasPendentes baseado em prontos e pendentes
void atualizarTempoRestante(vector<Tarefa>& tarefasPendentes, const vector<Tarefa>& prontos, const vector<Tarefa>& pendentes) {
    map<int, int> tempos;
    for (const auto& t : prontos) {
        tempos[t.id] = t.tempoRestante;
    }
    for (const auto& t : pendentes) {
        tempos[t.id] = t.tempoRestante;
    }
    for (auto& t : tarefasPendentes) {
        auto it = tempos.find(t.id);
        t.tempoRestante = (it != tempos.end()) ? it->second : 0;
    }
}

// Exibe estado atual do sistema
void print_estado_sistema(vector<Tarefa> &tarefasPendentes, const vector<Tarefa> &prontos, const vector<Tarefa> &pendentes, int tempoAtual, int currentTaskId)
{
    cout << "\nEstado do Sistema (Tempo: " << tempoAtual << "):\n";
    cout << "Tarefa em execucao: ";
    if (currentTaskId != -1)
        cout << "T" << currentTaskId;
    else
        cout << "Nenhuma (CPU ociosa)";
    cout << endl;

    cout << "Fila de prontos: ";
    if (prontos.empty())
        cout << "Vazia";
    else
        for (const auto &t : prontos)
            cout << "T" << t.id << " (Restante: " << t.tempoRestante << ") ";
    cout << endl;

    cout << "Tarefas pendentes: ";
    if (pendentes.empty())
        cout << "Vazia";
    else
        for (const auto &t : pendentes)
            cout << "T" << t.id << " (Ingresso: " << t.ingresso << ") ";
    cout << endl;

    cout << "Detalhes de todas as tarefas:\n";
    for (const auto &t : tarefasPendentes)
    {
        cout << "T" << t.id << ": Ingresso=" << t.ingresso << ", Duracao=" << t.duracao
             << ", Prioridade=" << t.prioridade << ", Tempo restante=" << t.tempoRestante
             << ", Cor=" << t.cor << ", Eventos=[";
        for (size_t i = 0; i < t.eventos.size(); ++i)
        {
            cout << t.eventos[i];
            if (i < t.eventos.size() - 1) cout << ",";
        }
        cout << "]" << endl;
    }
}

// Imprime gráfico de Gantt no console
void print_gantt(const vector<Tarefa> &tarefas, const vector<int> &running_task, int current_time)
{
    //ordena as tarefas por ID para mostrar
    vector<Tarefa> sorted_tasks = tarefas;
    sort(sorted_tasks.begin(), sorted_tasks.end(), [](const Tarefa &a, const Tarefa &b)
         { return a.id < b.id; });

    cout << "\nGrafico de Gantt (Tempo atual: " << current_time << "):\n";
    cout << "Tempo|";

    //Imprime os tempos
    int max_time = min(static_cast<int>(running_task.size()), current_time);
    for (int t = 0; t < max_time; ++t)
        cout << (t % 10);
    cout << endl;

     //Imprime as tarefas
    for (const auto &task : sorted_tasks)
    {
        cout << "T" << setw(2) << task.id << "  |";

        //Imprime a barra da tarefa
        for (int t = 0; t < max_time; ++t)
        {
            if (running_task[t] == task.id)
                cout << get_color_code(task.cor) << "#" << "\033[0m";
            else if (running_task[t] == -1)
                cout << ".";
            else
                cout << " ";
        }
        cout << endl;
    }
}

// Exporta gráfico de Gantt para SVG
void exportarGanttSVG(const vector<FatiaTarefa> &fatias, const vector<Tarefa> &tarefas, const string &nomeAlgoritmo)
{
    if (fatias.empty())
    {
        cerr << "Nenhuma fatia para exportar!" << endl;
        return;
    }

    int tempoMaximo = 0;
    map<int, vector<FatiaTarefa>> fatiasPorTarefa;
    for (const auto &fatia : fatias)
    {
        tempoMaximo = max(tempoMaximo, fatia.fim);
        fatiasPorTarefa[fatia.id].push_back(fatia);
    }

    const int LARGURA_SVG = 1400, ALTURA_SVG = 300 + (fatiasPorTarefa.size() * 45);
    const int ALTURA_BARRA = 35, ESPACO_VERTICAL = 10, MARGEM_ESQUERDA = 80, MARGEM_DIREITA = 60;
    double escalaTempo = (LARGURA_SVG - MARGEM_ESQUERDA - MARGEM_DIREITA) / static_cast<double>(tempoMaximo);

    string nomeArquivo = nomeAlgoritmo + "_gantt.svg";
    ofstream svg(nomeArquivo);
    if (!svg.is_open())
    {
        cerr << "Erro ao criar arquivo SVG: " << nomeArquivo << endl;
        return;
    }

    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    svg << "<svg width=\"" << LARGURA_SVG << "\" height=\"" << ALTURA_SVG << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
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
    svg << "</marker>\n</defs>\n";

    svg << "<text x=\"" << (LARGURA_SVG / 2) << "\" y=\"30\" class=\"titulo\" text-anchor=\"middle\">Grafico de Gantt - " << nomeAlgoritmo << "</text>\n";
    int yEixo = 70;
    svg << "<line x1=\"" << MARGEM_ESQUERDA << "\" y1=\"" << yEixo << "\" x2=\"" << (LARGURA_SVG - MARGEM_DIREITA) << "\" y2=\"" << yEixo << "\" class=\"eixo\"/>\n";

    int intervalo = max(1, tempoMaximo / 12);
    for (int t = 0; t <= tempoMaximo; t += intervalo)
    {
        int x = MARGEM_ESQUERDA + static_cast<int>(t * escalaTempo);
        svg << "<line x1=\"" << x << "\" y1=\"" << (yEixo - 8) << "\" x2=\"" << x << "\" y2=\"" << (yEixo + 8) << "\" class=\"eixo\"/>\n";
        svg << "<text x=\"" << x << "\" y=\"" << (yEixo + 22) << "\" class=\"tempo\" text-anchor=\"middle\">" << t << "</text>\n";
        svg << "<line x1=\"" << x << "\" y1=\"" << yEixo << "\" x2=\"" << x << "\" y2=\"" << (yEixo + 20) << "\" class=\"grid\"/>\n";
    }

    int yAtual = yEixo + 35;
    vector<string> coresPadrao = {"vermelho", "verde", "amarelo", "azul", "magenta", "ciano", "branco"};
    int corIndex = 0;
    for (const auto &[idTarefa, fatiasTarefa] : fatiasPorTarefa)
    {
        string corNome, corHex;
        int ingresso = 0;
        for (const auto &t : tarefas)
        {
            if (t.id == idTarefa)
            {
                corNome = t.cor;
                ingresso = t.ingresso;
                break;
            }
        }
        if (corNome.empty()) corNome = coresPadrao[corIndex++ % coresPadrao.size()];
        corHex = get_hex_color(corNome);

        svg << "<text x=\"10\" y=\"" << (yAtual + 22) << "\" font-size=\"14px\" font-weight=\"bold\" fill=\"#2c3e50\">T" << idTarefa << "</text>\n";

        for (const auto &fatia : fatiasTarefa)
        {
            int xInicio = MARGEM_ESQUERDA + static_cast<int>(fatia.inicio * escalaTempo);
            int larguraBarra = max(3, static_cast<int>(fatia.duracao * escalaTempo));
            svg << "<rect x=\"" << xInicio << "\" y=\"" << yAtual << "\" width=\"" << larguraBarra << "\" height=\"" << ALTURA_BARRA
                << "\" rx=\"4\" ry=\"4\" fill=\"" << corHex << "\" stroke=\"#2c3e50\" stroke-width=\"1.5\" opacity=\"0.95\"/>\n";
            if (larguraBarra >= 25)
                svg << "<text x=\"" << (xInicio + larguraBarra / 2) << "\" y=\"" << (yAtual + 22) << "\" class=\"tarefa\">" << fatia.duracao << "</text>\n";
        }

        int xIngresso = MARGEM_ESQUERDA + static_cast<int>(ingresso * escalaTempo);
        svg << "<line x1=\"" << xIngresso << "\" y1=\"" << (yAtual + ALTURA_BARRA -15) << "\" x2=\"" << xIngresso << "\" y2=\"" << (yAtual + ALTURA_BARRA + 15) << "\" class=\"ingresso\"/>\n";
        yAtual += ALTURA_BARRA + ESPACO_VERTICAL;
    }

    svg << "</svg>\n";
    svg.close();
    cout << "Gráfico de Gantt exportado para " << nomeArquivo << endl;
}

// Função escalonador: seleciona a próxima tarefa a ser executada
Tarefa* selectTask(vector<Tarefa>& prontos, const string& alg) {
    
    if (prontos.empty()) return nullptr;

    if (alg == "FIFO") {
        // Prontos já em ordem de chegada
        return &prontos.front();
    } else if (alg == "SRTF") {
        sort(prontos.begin(), prontos.end(), [](const Tarefa& a, const Tarefa& b) {
            return (a.tempoRestante < b.tempoRestante) || (a.tempoRestante == b.tempoRestante && a.ingresso < b.ingresso);
        });
    } else if (alg == "PRIOP") {
        sort(prontos.begin(), prontos.end(), [](const Tarefa& a, const Tarefa& b) {
            return (a.prioridade < b.prioridade) || (a.prioridade == b.prioridade && a.ingresso < b.ingresso);
        });
    } else {
        cerr << "Algoritmo desconhecido: " << alg << ". Usando SRTF como padrão.\n";
        sort(prontos.begin(), prontos.end(), [](const Tarefa& a, const Tarefa& b) {
            return (a.tempoRestante < b.tempoRestante) || (a.tempoRestante == b.tempoRestante && a.ingresso < b.ingresso);
        });
    }
    return &prontos.front();
}

// Função unificada de simulação
void simulate(vector<Tarefa>& tarefas) {
    if (algoritmo != "FIFO" && algoritmo != "SRTF" && algoritmo != "PRIOP") {
        cerr << "Algoritmo desconhecido: " << algoritmo << ". Usando SRTF.\n";
        algoritmo = "SRTF";
    }

    cout << "Executando " << algoritmo << " (modo " << (modoExecucao == "passo" ? "passo-a-passo" : "completo") << ")...\n";

    vector<Tarefa> tarefasPendentes = tarefas;
    sort(tarefasPendentes.begin(), tarefasPendentes.end(), [](const Tarefa &a, const Tarefa &b) { return a.id < b.id; });

    // Inicializa cores e tempo restante
    vector<string> coresPadrao = {"vermelho", "verde", "amarelo", "azul", "magenta", "ciano", "branco"};
    for (size_t i = 0; i < tarefas.size(); ++i) {
        if (tarefas[i].cor.empty()) tarefas[i].cor = coresPadrao[i % coresPadrao.size()];
        tarefas[i].tempoRestante = tarefas[i].duracao;
    }

    // Ordena pendentes por ingresso
    vector<Tarefa> pendentes = tarefas;
    sort(pendentes.begin(), pendentes.end(), [](const Tarefa &a, const Tarefa &b) { return a.ingresso < b.ingresso; });

    vector<Tarefa> prontos;
    vector<int> running_task;
    vector<FatiaTarefa> fatias;

    int tempoAtual = 0;
    double esperaTotal = 0.0, retornoTotal = 0.0;
    Tarefa* current = nullptr;
    bool is_preemptive = (algoritmo != "FIFO");
    int last_task = -1;
    int slice_start = 0;

    while (true) {
        // Move tarefas chegadas para prontos
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual) {
            prontos.push_back(pendentes.front());
            pendentes.erase(pendentes.begin());
        }

        int task_id = -1;
        if (!prontos.empty()) {
            if (current == nullptr || is_preemptive) {
                current = selectTask(prontos, algoritmo);
            }
            task_id = current->id;

            if (current->tempoRestante == current->duracao) {
                esperaTotal += static_cast<double>(tempoAtual - current->ingresso);
            }

            running_task.push_back(task_id);
            current->tempoRestante--;

            if (current->tempoRestante <= 0) {
                retornoTotal += static_cast<double>(tempoAtual + 1 - current->ingresso);
                auto it = find_if(prontos.begin(), prontos.end(), [current](const Tarefa& t) { return t.id == current->id; });
                if (it != prontos.end()) prontos.erase(it);
                current = nullptr;
            }
        } else {
            running_task.push_back(-1);
        }

        // Atualiza fatias
        if (task_id != last_task) {
            if (last_task != -1) {
                int dur = (tempoAtual - slice_start) + 1;
                fatias.push_back({last_task, slice_start, tempoAtual + 1, dur});
            }
            last_task = task_id;
            if (task_id != -1) {
                slice_start = tempoAtual;
            }
        }

        // Modo passo-a-passo
        if (modoExecucao == "passo") {
            atualizarTempoRestante(tarefasPendentes, prontos, pendentes);
            print_gantt(tarefasPendentes, running_task, tempoAtual + 1);
            print_estado_sistema(tarefasPendentes, prontos, pendentes, tempoAtual + 1, task_id);
            cout << (task_id != -1 ? "Executando T" + to_string(task_id) : "CPU ociosa") << " no tempo " << tempoAtual << " - Pressione Enter para proximo tick...\n";
            cin.get();
        }

        tempoAtual++;

        if (prontos.empty() && pendentes.empty()) {
            if (last_task != -1) {
                int dur = (tempoAtual - slice_start);
                fatias.push_back({last_task, slice_start, tempoAtual, dur});
            }
            break;
        }
    }

    exportarGanttSVG(fatias, tarefasPendentes, algoritmo);

    if (modoExecucao == "completo")
        print_gantt(tarefasPendentes, running_task, tempoAtual);

    cout << fixed << setprecision(2);
    cout << "\nRESULTADO FINAL:\n";
    cout << "Tempo medio de espera: " << (esperaTotal / tarefas.size()) << endl;
    cout << "Tempo medio de retorno: " << (retornoTotal / tarefas.size()) << endl;
}

// Carrega configuração e tarefas do arquivo
vector<Tarefa> carregarConfiguracao()
{
    const string nomeArquivo = "configuracao.txt";
    ifstream arquivo(nomeArquivo);
    vector<Tarefa> tarefas;

    if (!arquivo.is_open())
    {
        cerr << "Arquivo configuracao.txt nao encontrado. Usando dados padrao.\n";
        return tarefas;
    }

    string linha;
    bool primeiraLinha = true;
    while (getline(arquivo, linha))
    {
        if (linha.empty()) continue;
        stringstream ss(linha);
        string campo;

        if (primeiraLinha)
        {
            getline(ss, algoritmo, ';');
            getline(ss, campo, ';');
            quantum = stoi(campo);
            primeiraLinha = false;
        }
        else
        {
            Tarefa t;
            getline(ss, campo, ';');
            t.id = stoi(campo);
            getline(ss, t.cor, ';');
            getline(ss, campo, ';');
            t.ingresso = stoi(campo);
            getline(ss, campo, ';');
            t.duracao = stoi(campo);
            getline(ss, campo, ';');
            t.prioridade = stoi(campo);
            getline(ss, campo, ';');
            stringstream eventosStream(campo);
            string evento;
            while (getline(eventosStream, evento, ','))
                if (!evento.empty()) t.eventos.push_back(stoi(evento));
            t.tempoRestante = t.duracao;
            tarefas.push_back(t);
        }
    }

    arquivo.close();

    vector<string> coresPadrao = {"vermelho", "verde", "amarelo", "azul", "magenta", "ciano", "branco"};
    for (size_t i = 0; i < tarefas.size(); ++i)
        if (tarefas[i].cor.empty())
            tarefas[i].cor = coresPadrao[i % coresPadrao.size()];

    return tarefas;
}