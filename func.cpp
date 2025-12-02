#include "func.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <map>

using namespace std;

// Variáveis globais de configuração
int quantum;         // quantum para algoritmos preemptivos
string algoritmo;    // nome do algoritmo (FIFO, SRTF, PRIOP)
string modoExecucao; // "passo" ou "completo"
double alpha = 1;

// Estrutura para fatias do gráfico de Gantt
struct FatiaTarefa
{
    int id;      // ID da tarefa
    int inicio;  // tempo inicial da fatia
    int fim;     // tempo final da fatia
    int duracao; // duração da fatia
};

// 1. Função que retorna código ANSI para cor no terminal (aproximado)

string get_color_code(const string &cor)
{
    string hex = cor;
    if (hex.empty())
        return "\033[47m";
    if (hex[0] == '#')
        hex = hex.substr(1);
    if (hex.length() != 6)
        return "\033[47m";

    try
    {
        int r = stoi(hex.substr(0, 2), nullptr, 16);
        int g = stoi(hex.substr(2, 2), nullptr, 16);
        int b = stoi(hex.substr(4, 2), nullptr, 16);

        char buf[32];
        snprintf(buf, sizeof(buf), "\033[48;2;%d;%d;%dm", r, g, b);
        return buf;
    }
    catch (...)
    {
        return "\033[47m";
    }
}

// Retorna cor em hexadecimal para SVG
string get_hex_color(const string &cor)
{
    string hex = cor;
    if (hex.empty())
        return "#BDC3C7"; // cinza neutro como fallback

    // Aceita com ou sem #
    if (hex[0] == '#')
    {
        hex = hex.substr(1);
    }

    // Se já tem 6 dígitos hexadecimais válidos → retorna com #
    if (hex.length() == 6 && all_of(hex.begin(), hex.end(), ::isxdigit))
    {
        return "#" + hex;
    }

    // Fallback final caso a cor seja inválida
    return "#95A5A6";
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
            if (i < t.eventos.size() - 1)
                cout << ",";
        }
        cout << "]" << endl;
    }
}

// Imprime gráfico de Gantt no console
void print_gantt(const vector<Tarefa> &tarefas, const vector<int> &running_task, int current_time)
{
    // ordena as tarefas por ID para mostrar
    vector<Tarefa> sorted_tasks = tarefas;
    sort(sorted_tasks.begin(), sorted_tasks.end(), [](const Tarefa &a, const Tarefa &b)
         { return a.id < b.id; });

    cout << "\nGrafico de Gantt (Tempo atual: " << current_time << "):\n";
    cout << "Tempo|";

    // Imprime os tempos
    int max_time = min(static_cast<int>(running_task.size()), current_time + 1);
    for (int t = 0; t < max_time; ++t)
        cout << (t % 10);
    cout << endl;

    // Imprime as tarefas
    for (const auto &task : sorted_tasks)
    {
        cout << "T" << setw(2) << task.id << "  |";

        // Immprime a barra da tarefa
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
    vector<string> coresPadrao = {"E74C3C", "27AE60", "F1C40F", "3498DB", "9B59B6"};
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
        if (corNome.empty())
            corNome = coresPadrao[corIndex++ % coresPadrao.size()];
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
        svg << "<line x1=\"" << xIngresso << "\" y1=\"" << (yAtual + ALTURA_BARRA - 15) << "\" x2=\"" << xIngresso << "\" y2=\"" << (yAtual + ALTURA_BARRA + -5) << "\" class=\"ingresso\"/>\n";
        yAtual += ALTURA_BARRA + ESPACO_VERTICAL;
    }

    svg << "</svg>\n";
    svg.close();
    cout << "Grafico Gantt salvo como: " << nomeArquivo << endl;
}

// Atualiza tempoRestante nas tarefas originais com base nas filas
void atualizarTempoRestante(vector<Tarefa> &tarefasPendentes, const vector<Tarefa> &prontos, const vector<Tarefa> &pendentes)
{
    for (auto &t_orig : tarefasPendentes)
    {
        bool encontrado = false;
        for (const auto &t : prontos)
        {
            if (t_orig.id == t.id)
            {
                t_orig.tempoRestante = t.tempoRestante;
                encontrado = true;
                break;
            }
        }
        if (!encontrado)
        {
            for (const auto &t : pendentes)
            {
                if (t_orig.id == t.id)
                {
                    t_orig.tempoRestante = t.tempoRestante;
                    break;
                }
            }
        }
    }
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
        if (linha.empty() || linha[0] == '#' || linha.find_first_not_of(" \t") == string::npos)
            continue;

        stringstream ss(linha);
        string token;

        if (primeiraLinha)
        {
            // --- Lê o algoritmo ---
            if (!getline(ss, algoritmo, ';'))
                algoritmo = "FIFO";

            // --- Lê o quantum (sempre presente) ---
            string qStr;
            if (getline(ss, qStr, ';'))
            {
                try
                {
                    quantum = stoi(qStr);
                }
                catch (...)
                {
                    quantum = 2;
                }
            }
            else
                quantum = 2;

            // --- Só lê alpha se for PRIOPEnv ---
            if (algoritmo == "PRIOPEnv")
            {
                string aStr;
                if (getline(ss, aStr, ';') && !aStr.empty())
                {
                    try
                    {
                        alpha = stod(aStr);
                    }
                    catch (...)
                    {
                        alpha = 0.6;
                    }
                }
                else
                    alpha = 0.6;

                if (alpha <= 0 || alpha > 1.0)
                    alpha = 0.6;
            }
            else
            {
                alpha = 0.5; // valor padrão irrelevante
            }

            cout << "Algoritmo: " << algoritmo
                 << " | Quantum: " << quantum
                 << " | Alpha: " << alpha << endl;

            primeiraLinha = false;
            continue;
        }

        // --- Leitura normal das tarefas ---
        Tarefa t = {};
        vector<string> campos;

        while (getline(ss, token, ';'))
            campos.push_back(token);

        if (campos.size() < 5)
            continue;

        try
        {
            t.id = stoi(campos[0]);
            t.cor = campos[1];
            if (t.cor.empty())
                t.cor = "888888";
            t.ingresso = stoi(campos[2]);
            t.duracao = stoi(campos[3]);
            t.prioridade = stoi(campos[4]);
        }
        catch (...)
        {
            continue;
        }

        t.tempoRestante = t.duracao;
        t.prioridadeDinamica = t.prioridade;
        tarefas.push_back(t);
    }

    arquivo.close();
    return tarefas;
}

void simulador(vector<Tarefa> &tarefasOriginais)
{
    cout << "Executando " << algoritmo << " (modo " << (modoExecucao == "passo" ? "passo-a-passo" : "completo") << ")...\n";

    vector<Tarefa> pendentes = tarefasOriginais;
    sort(pendentes.begin(), pendentes.end(), [](const Tarefa &a, const Tarefa &b)
         { return a.ingresso < b.ingresso; });

    vector<Tarefa> prontos;
    vector<int> running_task;
    vector<FatiaTarefa> fatias;

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;

    while (!pendentes.empty() || !prontos.empty())
    {
        // 1. Traz tarefas que já chegaram
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual)
        {
            Tarefa nova = pendentes.front();
            nova.prioridadeDinamica = nova.prioridade; // reseta ao entrar no sistema
            prontos.push_back(nova);
            pendentes.erase(pendentes.begin());
        }

        // 2. Escolhe a próxima tarefa
        int idx = escalonador(prontos);

        if (idx == -1) // CPU ociosa
        {
            running_task.push_back(-1);
            if (modoExecucao == "passo")
            {
                atualizarTempoRestante(tarefasOriginais, prontos, pendentes);
                print_gantt(tarefasOriginais, running_task, tempoAtual);
                print_estado_sistema(tarefasOriginais, prontos, pendentes, tempoAtual, -1);
                cout << "CPU ociosa - Pressione Enter...\n";
                cin.get();
            }
            tempoAtual++;
            continue;
        }

        Tarefa &atual = prontos[idx];

        if (algoritmo == "PRIOPEnv")
        {
            atual.prioridadeDinamica = atual.prioridade;
        }

        int inicio = tempoAtual;

        int duracaoFatia = (algoritmo == "FIFO" || algoritmo == "PRIOP" || algoritmo == "PRIOPEnv" || algoritmo == "SRTF")
                               ? min(quantum, atual.tempoRestante)
                               : 1;

        int fim = tempoAtual + duracaoFatia;

        // Contabiliza espera apenas na primeira execução da tarefa
        if (atual.tempoRestante == atual.duracao)
            esperaTotal += (inicio - atual.ingresso);

        running_task.push_back(atual.id);
        atual.tempoRestante -= duracaoFatia;
        fatias.push_back({atual.id, inicio, fim, duracaoFatia});

        if (modoExecucao == "passo")
        {
            atualizarTempoRestante(tarefasOriginais, prontos, pendentes);
            print_gantt(tarefasOriginais, running_task, tempoAtual);
            print_estado_sistema(tarefasOriginais, prontos, pendentes, tempoAtual, atual.id);
            cout << "Executando T" << atual.id << " - Pressione Enter...\n";
            cin.get();
        }

        tempoAtual = fim;

        // Remove se terminou
        if (atual.tempoRestante <= 0)
        {
            retornoTotal += (tempoAtual - atual.ingresso);
            prontos.erase(prontos.begin() + idx);
        }
    }

    // Mescla fatias consecutivas para o SVG
    vector<FatiaTarefa> mescladas;
    if (!fatias.empty())
    {
        FatiaTarefa atualFatia = fatias[0];
        for (size_t i = 1; i < fatias.size(); ++i)
        {
            if (fatias[i].id == atualFatia.id && fatias[i].inicio == atualFatia.fim)
                atualFatia.duracao += fatias[i].duracao, atualFatia.fim = fatias[i].fim;
            else
            {
                mescladas.push_back(atualFatia);
                atualFatia = fatias[i];
            }
        }
        mescladas.push_back(atualFatia);
    }

    exportarGanttSVG(mescladas, tarefasOriginais, algoritmo);

    if (modoExecucao == "completo")
        print_gantt(tarefasOriginais, running_task, tempoAtual - 1);

    cout << fixed << setprecision(2);
    cout << "\nRESULTADO FINAL (" << algoritmo << "):\n";
    cout << "Tempo medio de espera:  " << (esperaTotal / tarefasOriginais.size()) << endl;
    cout << "Tempo medio de retorno: " << (retornoTotal / tarefasOriginais.size()) << endl;
}

int escalonador(vector<Tarefa>& prontos)   // ← tem que ser NÃO const! (modificamos a prioridade dinâmica)
{
    if (prontos.empty())
        return -1;

    // ==============================================================
    // 1. PRIOPEnv – Prioridade com Envelhecimento (maior número = melhor)
    // ==============================================================
    if (algoritmo == "PRIOPEnv")
    {
        // Envelhece TODAS as tarefas na fila de prontos
        for (auto& t : prontos)
        {
            t.prioridadeDinamica += alpha;   // envelhecimento → fica mais prioritária!
        }

        // Escolhe a tarefa com a MAIOR prioridade dinâmica
        int melhor = 0;
        for (size_t i = 1; i < prontos.size(); ++i)
        {
            if (prontos[i].prioridadeDinamica > prontos[melhor].prioridadeDinamica ||
                (abs(prontos[i].prioridadeDinamica - prontos[melhor].prioridadeDinamica) < 1e-9 &&
                 prontos[i].ingresso < prontos[melhor].ingresso))
            {
                melhor = i;
            }
        }

        // A tarefa escolhida "rejuvenesce" → volta à prioridade estática
        prontos[melhor].prioridadeDinamica = prontos[melhor].prioridade;

        return melhor;
    }

    // ==============================================================
    // 2. PRIOP – Prioridade normal (sem envelhecimento)
    // ==============================================================
    else if (algoritmo == "PRIOP")
    {
        int melhor = 0;
        for (size_t i = 1; i < prontos.size(); ++i)
        {
            // Maior número = melhor prioridade
            if (prontos[i].prioridade > prontos[melhor].prioridade ||
                (prontos[i].prioridade == prontos[melhor].prioridade &&
                 prontos[i].ingresso < prontos[melhor].ingresso))
            {
                melhor = i;
            }
        }
        return melhor;
    }

    // ==============================================================
    // 3. SRTF – Shortest Remaining Time First
    // ==============================================================
    else if (algoritmo == "SRTF")
    {
        int melhor = 0;
        for (size_t i = 1; i < prontos.size(); ++i)
        {
            if (prontos[i].tempoRestante < prontos[melhor].tempoRestante ||
                (prontos[i].tempoRestante == prontos[melhor].tempoRestante &&
                 prontos[i].ingresso < prontos[melhor].ingresso))
            {
                melhor = i;
            }
        }
        return melhor;
    }

    // ==============================================================
    // 4. FIFO (padrão) – First In First Out
    // ==============================================================
    else // FIFO ou qualquer outro
    {
        int melhor = 0;
        for (size_t i = 1; i < prontos.size(); ++i)
        {
            if (prontos[i].ingresso < prontos[melhor].ingresso)
            {
                melhor = i;
            }
        }
        return melhor;
    }
}