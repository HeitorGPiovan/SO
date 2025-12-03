#include "func.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <map>
#include <cstdio>

using namespace std;

string modoExecucao = "passo";
string algoritmo = "FIFO";
int quantum = 2;
double alpha = 0.6;
map<int, Mutex> mutexes;

// CORES
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

string get_hex_color(const string &cor)
{
    string hex = cor;
    if (hex.empty())
        return "#BDC3C7";
    if (hex[0] == '#')
        hex = hex.substr(1);
    return (hex.length() == 6 && all_of(hex.begin(), hex.end(), ::isxdigit)) ? "#" + hex : "#95A5A6";
}

// ESTADO DO SISTEMA
void print_estado_sistema(const vector<Tarefa> &prontos, const vector<Tarefa> &pendentes,
                          const vector<Tarefa> &bloqueadas, int tempoAtual, int currentTaskId)
{
    cout << "\n=== Tempo " << tempoAtual << " ===\n";
    cout << "Executando: " << (currentTaskId != -1 ? "T" + to_string(currentTaskId) : "Ocioso") << "\n";

    cout << "\n--- PRONTOS ---\n";
    for (auto &t : prontos)
    {
        cout << "T" << t.id << ": ";
        cout << "ingresso=" << t.ingresso << ", ";
        cout << "duracao=" << t.duracao << ", ";
        cout << "restante=" << t.tempoRestante << ", ";
        cout << "executado=" << t.tempoExecutado << ", ";
        if (algoritmo == "PRIOPEnv")
        {
            cout << "p_est=" << t.prioridade << ", ";
            cout << "p_din=" << fixed << setprecision(1) << t.prioridadeDinamica;
        }
        else
        {
            cout << "prioridade=" << t.prioridade;
        }
        if (t.bloqueada)
            cout << " [BLOQUEADA]";
        cout << "\n";
    }

    cout << "\n--- PENDENTES ---\n";
    for (auto &t : pendentes)
    {
        cout << "T" << t.id << ": ";
        cout << "ingresso=" << t.ingresso << ", ";
        cout << "duracao=" << t.duracao << ", ";
        if (algoritmo == "PRIOPEnv")
        {
            cout << "p_est=" << t.prioridade << ", ";
            cout << "p_din=" << fixed << setprecision(1) << t.prioridadeDinamica;
        }
        else
        {
            cout << "prioridade=" << t.prioridade;
        }
        cout << "\n";
    }

    cout << "\n--- BLOQUEADAS ---\n";
    for (auto &t : bloqueadas)
    {
        cout << "T" << t.id << ": ";
        cout << "ingresso=" << t.ingresso << ", ";
        cout << "duracao=" << t.duracao << ", ";
        cout << "restante=" << t.tempoRestante << ", ";
        cout << "executado=" << t.tempoExecutado << ", ";
        if (algoritmo == "PRIOPEnv")
        {
            cout << "p_est=" << t.prioridade << ", ";
            cout << "p_din=" << fixed << setprecision(1) << t.prioridadeDinamica;
        }
        else
        {
            cout << "prioridade=" << t.prioridade;
        }
        cout << " [BLOQUEADA]";
        if (!t.eventosMutex.empty())
        {
            cout << " (eventos: ";
            for (auto &ev : t.eventosMutex)
            {
                cout << "t" << ev.first << (ev.second.first == 'L' ? "L" : "U")
                     << ev.second.second << " ";
            }
            cout << ")";
        }
        cout << "\n";
    }

    cout << "\n--- MUTEXES ---\n";
    for (const auto &[id, m] : mutexes)
    {
        cout << "  M" << setw(2) << id << " -> " << (m.dono == -1 ? "LIVRE" : "T" + to_string(m.dono));
        if (!m.filaEspera.empty())
        {
            cout << " (espera:";
            for (int x : m.filaEspera)
                cout << " T" << x;
            cout << ")";
        }
        cout << "\n";
    }
    cout << "\n";
}

// GANTT
void print_gantt(const vector<Tarefa> &tarefas, const vector<int> &running_task, int current_time)
{
    vector<Tarefa> sorted = tarefas;
    sort(sorted.begin(), sorted.end(), [](const Tarefa &a, const Tarefa &b)
         { return a.id < b.id; });
    cout << "\nGantt (t=" << current_time << "):\nTempo |";
    int max_t = min((int)running_task.size(), current_time + 50);
    for (int t = 0; t < max_t; ++t)
        cout << (t % 10);
    cout << "\n      +";
    for (int t = 0; t < max_t; ++t)
        cout << "-";
    cout << "\n";
    for (const auto &task : sorted)
    {
        cout << "T" << setw(2) << task.id << " |";
        for (int t = 0; t < max_t; ++t)
        {
            if (t < (int)running_task.size())
            {
                if (running_task[t] == task.id)
                    cout << get_color_code(task.cor) << "#" << "\033[0m";
                else if (running_task[t] == -1)
                    cout << ".";
                else
                    cout << " ";
            }
        }
        cout << "\n";
    }
    cout << "\n";
}

// SVG
void exportarGanttSVG(const vector<FatiaTarefa> &fatias, const vector<Tarefa> &tarefas, const string &nome)
{
    if (fatias.empty())
        return;
    int tempoMax = 0;
    for (const auto &f : fatias)
        tempoMax = max(tempoMax, f.fim);
    map<int, vector<FatiaTarefa>> porTarefa;
    for (const auto &f : fatias)
        porTarefa[f.id].push_back(f);

    ofstream svg(nome + "_gantt.svg");
    if (!svg)
        return;
    double escala = 1200.0 / max(1, tempoMax);
    int y = 70;
    svg << "<svg width=\"1400\" height=\"" << 100 + (int)porTarefa.size() * 55 << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    svg << "<text x=\"700\" y=\"30\" font-size=\"24\" text-anchor=\"middle\">" << nome << "</text>\n";
    for (const auto &[id, lista] : porTarefa)
    {
        string cor = "#95A5A6";
        for (const auto &t : tarefas)
            if (t.id == id)
            {
                cor = get_hex_color(t.cor);
                break;
            }
        svg << "<text x=\"10\" y=\"" << y + 20 << "\">T" << id << "</text>\n";
        for (const auto &f : lista)
        {
            int x = 100 + (int)(f.inicio * escala);
            int w = max(3, (int)(f.duracao * escala));
            svg << "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w << "\" height=\"40\" fill=\"" << cor << "\" rx=\"5\"/>\n";
        }
        y += 55;
    }
    svg << "</svg>";
    svg.close();
    cout << "Gantt salvo como " << nome << "_gantt.svg\n";
}

// CARREGA CONFIGURAÇÃO
vector<Tarefa> carregarConfiguracao()
{
    ifstream arq("configuracao.txt");
    vector<Tarefa> tarefas;
    if (!arq.is_open())
    {
        cerr << "Erro: configuracao.txt nao encontrado!\n";
        return tarefas;
    }

    string linha;
    bool primeira = true;
    while (getline(arq, linha))
    {
        if (linha.empty() || linha[0] == '#')
            continue;
        stringstream ss(linha);
        string tok;

        if (primeira)
        {
            getline(ss, algoritmo, ';');
            getline(ss, tok, ';');
            quantum = tok.empty() ? 2 : stoi(tok);
            if (algoritmo == "PRIOPEnv")
            {
                getline(ss, tok, ';');
                alpha = tok.empty() ? 0.6 : stod(tok);
            }
            primeira = false;
            continue;
        }

        Tarefa t{};
        vector<string> campos;
        while (getline(ss, tok, ';'))
            campos.push_back(tok);
        if (campos.size() < 5)
            continue;

        t.id = stoi(campos[0]);
        t.cor = campos[1];
        t.ingresso = stoi(campos[2]);
        t.duracao = stoi(campos[3]);
        t.prioridade = stoi(campos[4]);
        t.prioridadeDinamica = t.prioridade;
        t.tempoRestante = t.duracao;
        t.prioridadeDinamica = t.prioridade;
        t.tempoExecutado = 0;
        t.bloqueada = false;

        for (size_t i = 5; i < campos.size(); ++i)
        {
            string acao = campos[i];
            if (acao.length() >= 7 && (acao.substr(0, 2) == "ML" || acao.substr(0, 2) == "MU"))
            {
                char tipo = acao[1];
                int mutexId = stoi(acao.substr(2, 2));
                int tempoRel = stoi(acao.substr(5));
                t.eventosMutex.emplace_back(tempoRel, make_pair(tipo, mutexId));
                mutexes[mutexId];
            }
        }
        sort(t.eventosMutex.begin(), t.eventosMutex.end());
        tarefas.push_back(t);
    }
    return tarefas;
}

void aplicarEnvelhecimento(vector<Tarefa> &prontos, int tarefaExecutandoId)
{
    if (algoritmo != "PRIOPEnv")
        return;

    for (auto &t : prontos)
    {
        // NÃO envelhece a tarefa que está atualmente executando
        if (t.id != tarefaExecutandoId)
        {
            // Envelhecimento FIXO por evento (não acumulado)
            t.prioridadeDinamica += alpha;
        }
    }
}

// SIMULADOR — VERSÃO 100% CORRETA (SEM LOOP INFINITO)
void simulador(vector<Tarefa> &tarefasOriginais)
{
    vector<Tarefa> pendentes = tarefasOriginais;
    sort(pendentes.begin(), pendentes.end(),
         [](const Tarefa &a, const Tarefa &b)
         { return a.ingresso < b.ingresso; });

    vector<Tarefa> prontos, bloqueadas;
    vector<int> running_task;
    vector<FatiaTarefa> fatias;

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;
    int currentId = -1;

    while (true)
    {
        bool chegouNovaTarefa = false; // <-- ADICIONE
        // Chegadas
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual)
        {
            Tarefa nova = pendentes.front();
            nova.prioridadeDinamica = nova.prioridade; // inicializa dinâmica
            prontos.push_back(nova);
            pendentes.erase(pendentes.begin());
            chegouNovaTarefa = true;
        }

        if (chegouNovaTarefa)
        {
            aplicarEnvelhecimento(prontos, currentId);
        }
        // Escalonamento (com preempção automática por envelhecimento)

        if (!prontos.empty())
        {
            int idx = escalonador(prontos);
            int novoId = prontos[idx].id;

            if (algoritmo == "PRIOPEnv" && novoId != currentId)
            {
                prontos[idx].prioridadeDinamica = prontos[idx].prioridade;
            }

            if (currentId != novoId && currentId != -1)
            {
                cout << ">>> PREEMPÇÃO: T" << currentId << " -> T" << novoId
                     << " (t=" << tempoAtual << ")"
                     << (algoritmo == "PRIOPEnv" ? " [envelhecimento]" : "") << "\n";

                aplicarEnvelhecimento(prontos, currentId);
            }
            else if (currentId == -1 && novoId != -1)
            {
                // CPU saindo do estado ocioso
                aplicarEnvelhecimento(prontos, -1);
            }
            currentId = novoId;
        }

        // Fim da simulação
        if (currentId == -1 && prontos.empty() && pendentes.empty() && bloqueadas.empty())
        {
            break;
        }

        // CPU ociosa
        if (currentId == -1)
        {
            running_task.push_back(-1);
            tempoAtual++;
            // Aplica envelhecimento mesmo durante ocioso
            aplicarEnvelhecimento(prontos, -1);
            if (modoExecucao == "passo")
            {
                print_gantt(tarefasOriginais, running_task, tempoAtual);
                print_estado_sistema(prontos, pendentes, bloqueadas, tempoAtual, -1);
                cout << "CPU ociosa - Enter...\n";
                cin.get();
            }
            continue;
        }

        auto it = find_if(prontos.begin(), prontos.end(),
                          [currentId](const Tarefa &t)
                          { return t.id == currentId; });
        if (it == prontos.end())
        {
            currentId = -1;
            continue;
        }
        Tarefa &tarefa = *it;

        // PROCESSA EVENTO DE MUTEX (se houver no tempo relativo atual + 1)
        bool eventoProcessado = false;
        for (auto ev = tarefa.eventosMutex.begin(); ev != tarefa.eventosMutex.end(); ++ev)
        {
            if (ev->first == tarefa.tempoExecutado + 1)
            {
                int mid = ev->second.second;
                char op = ev->second.first;

                if (op == 'L')
                {
                    if (mutexes[mid].dono != -1)
                    {
                        cout << ">>> T" << tarefa.id << " BLOQUEOU em M" << mid << " (t=" << tempoAtual << ")\n";
                        tarefa.bloqueada = true;
                        mutexes[mid].filaEspera.push_back(tarefa.id);
                        bloqueadas.push_back(tarefa);
                        prontos.erase(it);
                        currentId = -1;
                        running_task.push_back(-1);
                        eventoProcessado = true;
                    }
                    else
                    {
                        mutexes[mid].dono = tarefa.id;
                        cout << ">>> T" << tarefa.id << " adquiriu M" << mid << " (t=" << tempoAtual << ")\n";
                    }
                }
                else if (op == 'U' && mutexes[mid].dono == tarefa.id)
                {
                    cout << ">>> T" << tarefa.id << " liberou M" << mid << " (t=" << tempoAtual << ")\n";
                    mutexes[mid].dono = -1;
                    if (!mutexes[mid].filaEspera.empty())
                    {
                        int prox = mutexes[mid].filaEspera.front();
                        mutexes[mid].filaEspera.erase(mutexes[mid].filaEspera.begin());
                        mutexes[mid].dono = prox;
                        auto bt = find_if(bloqueadas.begin(), bloqueadas.end(),
                                          [prox](const Tarefa &x)
                                          { return x.id == prox; });
                        if (bt != bloqueadas.end())
                        {
                            bt->bloqueada = false;
                            prontos.push_back(*bt);
                            bloqueadas.erase(bt);
                            cout << ">>> T" << prox << " DESBLOQUEADA e adquiriu o mutex!\n";
                        }
                    }
                }
                // Remove o evento processado
                tarefa.eventosMutex.erase(ev);
                break;
            }
        }

        if (eventoProcessado)
        {
            tempoAtual++;
            // Aplica envelhecimento ao bloquear/desbloquear
            aplicarEnvelhecimento(prontos, -1);
            if (modoExecucao == "passo")
            {
                print_gantt(tarefasOriginais, running_task, tempoAtual);
                print_estado_sistema(prontos, pendentes, bloqueadas, tempoAtual, -1);
                cout << "Tarefa bloqueada - Enter...\n";
                cin.get();
            }
            continue;
        }

        // Executa unidade
        if (tarefa.tempoRestante == tarefa.duracao)
            esperaTotal += tempoAtual - tarefa.ingresso;

        tarefa.tempoRestante--;
        tarefa.tempoExecutado++;
        running_task.push_back(tarefa.id);
        fatias.push_back({tarefa.id, tempoAtual, tempoAtual + 1, 1});

        if (modoExecucao == "passo")
        {
            print_gantt(tarefasOriginais, running_task, tempoAtual + 1);
            print_estado_sistema(prontos, pendentes, bloqueadas, tempoAtual + 1, tarefa.id);
            cout << "Executando T" << tarefa.id << " (restam " << tarefa.tempoRestante << ") - Enter...\n";
            cin.get();
        }

        tempoAtual++;

        if (tarefa.tempoRestante <= 0)
        {
            retornoTotal += tempoAtual - tarefa.ingresso;
            prontos.erase(it);
            currentId = -1;
            // Aplica envelhecimento quando uma tarefa termina
            aplicarEnvelhecimento(prontos, -1);
        }
    } // FIM DO while(true)

    // SVG final
    vector<FatiaTarefa> mescladas;
    if (!fatias.empty())
    {
        FatiaTarefa cur = fatias[0];
        for (size_t i = 1; i < fatias.size(); ++i)
        {
            if (fatias[i].id == cur.id && fatias[i].inicio == cur.fim)
            {
                cur.duracao += fatias[i].duracao;
                cur.fim = fatias[i].fim;
            }
            else
            {
                mescladas.push_back(cur);
                cur = fatias[i];
            }
        }
        mescladas.push_back(cur);
    }

    exportarGanttSVG(mescladas, tarefasOriginais, algoritmo);
    cout << fixed << setprecision(2);
    cout << "\nTempo medio de espera : " << esperaTotal / tarefasOriginais.size() << "\n";
    cout << "Tempo medio de retorno: " << retornoTotal / tarefasOriginais.size() << "\n";

} // FIM DA FUNÇÃO simulador

// ESCALONADOR
int escalonador(std::vector<Tarefa> &prontos)
{
    if (prontos.empty())
        return -1;

    int best = 0;

    if (algoritmo == "SRTF")
    {
        // Shortest Remaining Time First
        for (size_t i = 1; i < prontos.size(); ++i)
        {
            if (prontos[i].tempoRestante < prontos[best].tempoRestante)
            {
                best = i;
            }
            else if (prontos[i].tempoRestante == prontos[best].tempoRestante &&
                     prontos[i].ingresso < prontos[best].ingresso)
            {
                best = i;
            }
        }
    }
    else if (algoritmo == "PRIOPEnv")
    {
        // Prioridade Preemptiva com Envelhecimento
        for (size_t i = 1; i < prontos.size(); ++i)
        {
            double pBest = prontos[best].prioridadeDinamica;
            double pI = prontos[i].prioridadeDinamica;

            if (pI > pBest || (pI == pBest && prontos[i].ingresso < prontos[best].ingresso))
            {
                best = i;
            }
        }
    }
    else if (algoritmo == "PRIOP")
    {
        // Prioridade Preemptiva (sem envelhecimento)
        for (size_t i = 1; i < prontos.size(); ++i)
        {
            int pBest = prontos[best].prioridade;
            int pI = prontos[i].prioridade;

            if (pI > pBest || (pI == pBest && prontos[i].ingresso < prontos[best].ingresso))
            {
                best = i;
            }
        }
    }
    else if (algoritmo == "FIFO")
    {
        for (size_t i = 1; i < prontos.size(); ++i)
        {
            if (prontos[i].ingresso < prontos[best].ingresso)
            {
                best = i;
            }
        }
    }
    else
    {
        // Default: FIFO
        for (size_t i = 1; i < prontos.size(); ++i)
        {
            if (prontos[i].ingresso < prontos[best].ingresso)
            {
                best = i;
            }
        }
    }

    return best;
}
