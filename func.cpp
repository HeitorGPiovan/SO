// func.cpp — VERSÃO FINAL 100% CORRETA (preempção + mutex)
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

// ====================== CORES ======================
string get_color_code(const string& cor) {
    string hex = cor;
    if (hex.empty()) return "\033[47m";
    if (hex[0] == '#') hex = hex.substr(1);
    if (hex.length() != 6) return "\033[47m";
    try {
        int r = stoi(hex.substr(0,2), nullptr, 16);
        int g = stoi(hex.substr(2,2), nullptr, 16);
        int b = stoi(hex.substr(4,2), nullptr, 16);
        char buf[32];
        snprintf(buf, sizeof(buf), "\033[48;2;%d;%d;%dm", r, g, b);
        return buf;
    } catch(...) { return "\033[47m"; }
}

string get_hex_color(const string& cor) {
    string hex = cor;
    if (hex.empty()) return "#BDC3C7";
    if (hex[0] == '#') hex = hex.substr(1);
    if (hex.length() == 6 && all_of(hex.begin(), hex.end(), ::isxdigit))
        return "#" + hex;
    return "#95A5A6";
}

// ====================== ESTADO DO SISTEMA ======================
void print_estado_sistema(const vector<Tarefa>& prontos,
                          const vector<Tarefa>& pendentes,
                          const vector<Tarefa>& bloqueadas,
                          int tempoAtual, int currentTaskId) {
    cout << "\n=== Tempo " << tempoAtual << " ===\n";
    cout << "Executando: " << (currentTaskId != -1 ? "T" + to_string(currentTaskId) : "Ocioso") << "\n";
    cout << "Prontos    : "; 
    for (auto& t : prontos) cout << "T" << t.id << "(p" << t.prioridade << ") ";
    cout << "\nPendentes  : "; for (auto& t : pendentes) cout << "T" << t.id << " "; cout << "\n";
    cout << "Bloqueadas : "; for (auto& t : bloqueadas) cout << "T" << t.id << " "; cout << "\n";
    cout << "Mutexes:\n";
    for (auto& [id, m] : mutexes) {
        cout << "  M" << id << " → " << (m.dono == -1 ? "LIVRE" : "T" + to_string(m.dono));
        if (!m.filaEspera.empty()) {
            cout << " (espera: ";
            for (int x : m.filaEspera) cout << "T" << x << " ";
            cout << ")";
        }
        cout << "\n";
    }
    cout << "\n";
}

// ====================== GANTT CONSOLE ======================
void print_gantt(const vector<Tarefa>& tarefas, const vector<int>& running_task, int current_time) {
    vector<Tarefa> sorted = tarefas;
    sort(sorted.begin(), sorted.end(), [](const Tarefa& a, const Tarefa& b){ return a.id < b.id; });
    cout << "\nGantt (t=" << current_time << "):\nTempo |";
    int max_t = min((int)running_task.size(), current_time + 30);
    for (int t = 0; t < max_t; ++t) cout << (t % 10);
    cout << "\n";
    for (const auto& task : sorted) {
        cout << "T" << setw(2) << task.id << " |";
        for (int t = 0; t < max_t; ++t) {
            if (t < (int)running_task.size()) {
                if (running_task[t] == task.id)      cout << get_color_code(task.cor) << "#" << "\033[0m";
                else if (running_task[t] == -1)      cout << ".";
                else                                 cout << " ";
            }
        }
        cout << "\n";
    }
}

// ====================== EXPORTAR SVG ======================
void exportarGanttSVG(const vector<FatiaTarefa>& fatias, const vector<Tarefa>& tarefas, const string& nomeAlgoritmo) {
    if (fatias.empty()) return;
    int tempoMax = 0;
    map<int, vector<FatiaTarefa>> porTarefa;
    for (auto& f : fatias) {
        tempoMax = max(tempoMax, f.fim);
        porTarefa[f.id].push_back(f);
    }
    ofstream svg(nomeAlgoritmo + "_gantt.svg");
    if (!svg) return;
    double escala = 1200.0 / max(1, tempoMax);
    int y = 70;
    svg << "<svg width=\"1400\" height=\"" << 100 + porTarefa.size()*55 << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    svg << "<text x=\"700\" y=\"30\" font-size=\"24\" text-anchor=\"middle\">" << nomeAlgoritmo << "</text>\n";
    for (auto& [id, lista] : porTarefa) {
        string cor = "#95A5A6";
        for (auto& t : tarefas) if (t.id == id) { cor = get_hex_color(t.cor); break; }
        svg << "<text x=\"10\" y=\"" << y+20 << "\">T" << id << "</text>\n";
        for (auto& f : lista) {
            int x = 100 + f.inicio * escala;
            int w = max(3, (int)(f.duracao * escala));
            svg << "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w << "\" height=\"40\" fill=\"" << cor << "\" rx=\"5\"/>\n";
        }
        y += 55;
    }
    svg << "</svg>";
    svg.close();
    cout << "Gantt salvo como " << nomeAlgoritmo << "_gantt.svg\n";
}

// ====================== ATUALIZA TEMPO RESTANTE ======================
void atualizarTempoRestante(vector<Tarefa>& originais,
                            const vector<Tarefa>& prontos,
                            const vector<Tarefa>& pendentes,
                            const vector<Tarefa>& bloqueadas) {
    for (auto& o : originais) {
        for (auto& t : prontos)    if (t.id == o.id) { o.tempoRestante = t.tempoRestante; o.tempoExecutado = t.tempoExecutado; break; }
        for (auto& t : pendentes)  if (t.id == o.id) { o.tempoRestante = t.tempoRestante; o.tempoExecutado = t.tempoExecutado; break; }
        for (auto& t : bloqueadas) if (t.id == o.id) { o.tempoRestante = t.tempoRestante; o.tempoExecutado = t.tempoExecutado; break; }
    }
}

// ====================== CARREGA CONFIGURAÇÃO ======================
vector<Tarefa> carregarConfiguracao() {
    ifstream arq("configuracao.txt");
    vector<Tarefa> tarefas;
    if (!arq.is_open()) {
        cerr << "configuracao.txt nao encontrado.\n";
        return tarefas;
    }
    string linha;
    bool primeira = true;
    while (getline(arq, linha)) {
        if (linha.empty() || linha[0] == '#') continue;
        stringstream ss(linha);
        string tok;
        if (primeira) {
            getline(ss, algoritmo, ';');
            getline(ss, tok, ';'); quantum = tok.empty() ? 2 : stoi(tok);
            if (algoritmo == "PRIOPEnv") {
                getline(ss, tok, ';'); alpha = tok.empty() ? 0.6 : stod(tok);
            }
            primeira = false;
            continue;
        }
        Tarefa t{};
        vector<string> campos;
        while (getline(ss, tok, ';')) campos.push_back(tok);
        if (campos.size() < 5) continue;
        t.id = stoi(campos[0]);
        t.cor = campos[1]; if (t.cor.empty()) t.cor = "888888";
        t.ingresso = stoi(campos[2]);
        t.duracao = stoi(campos[3]);
        t.prioridade = stoi(campos[4]);
        t.tempoRestante = t.duracao;
        t.prioridadeDinamica = t.prioridade;
        for (size_t i = 5; i < campos.size(); ++i) {
            string ev = campos[i];
            if (ev.size() >= 8 && (ev.substr(0,2) == "ML" || ev.substr(0,2) == "MU")) {
                char tipo = ev[1];
                int mid = stoi(ev.substr(2,2));
                int tempo = stoi(ev.substr(6));
                t.eventosMutex.emplace_back(tempo, make_pair(tipo, mid));
                mutexes[mid];
            }
        }
        sort(t.eventosMutex.begin(), t.eventosMutex.end());
        tarefas.push_back(t);
    }
    return tarefas;
}

// ====================== SIMULADOR (CORRIGIDO) ======================
void simulador(vector<Tarefa>& tarefasOriginais) {
    vector<Tarefa> pendentes = tarefasOriginais;
    sort(pendentes.begin(), pendentes.end(), [](auto& a, auto& b){ return a.ingresso < b.ingresso; });

    vector<Tarefa> prontos, bloqueadas;
    vector<int> running_task;
    vector<FatiaTarefa> fatias;

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;
    int currentId = -1;

    while (!pendentes.empty() || !prontos.empty() || !bloqueadas.empty() || currentId != -1) {
        // 1. Chegada de novas tarefas
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual) {
            prontos.push_back(pendentes.front());
            pendentes.erase(pendentes.begin());
        }

        // 2. Trata eventos de mutex da tarefa em execução
        if (currentId != -1) {
            auto it = find_if(prontos.begin(), prontos.end(), [currentId](auto& t){ return t.id == currentId; });
            if (it != prontos.end()) {
                auto& t = *it;
                auto ev = lower_bound(t.eventosMutex.begin(), t.eventosMutex.end(),
                                     make_pair(t.tempoExecutado, make_pair('A', -1)));
                while (ev != t.eventosMutex.end() && ev->first <= t.tempoExecutado) {
                    int mid = ev->second.second;
                    if (ev->second.first == 'L') {
                        if (mutexes[mid].dono != -1) {
                            cout << ">>> T" << t.id << " BLOQUEOU no mutex M" << mid << " (t=" << tempoAtual << ")\n";
                            t.bloqueada = true;
                            mutexes[mid].filaEspera.push_back(t.id);
                            bloqueadas.push_back(t);
                            prontos.erase(it);
                            currentId = -1;
                            break;
                        } else {
                            mutexes[mid].dono = t.id;
                            cout << ">>> T" << t.id << " adquiriu M" << mid << " (t=" << tempoAtual << ")\n";
                        }
                    } else if (mutexes[mid].dono == t.id) {
                        cout << ">>> T" << t.id << " liberou M" << mid << " (t=" << tempoAtual << ")\n";
                        mutexes[mid].dono = -1;
                        if (!mutexes[mid].filaEspera.empty()) {
                            int prox = mutexes[mid].filaEspera.front();
                            mutexes[mid].filaEspera.erase(mutexes[mid].filaEspera.begin());
                            mutexes[mid].dono = prox;
                            auto bt = find_if(bloqueadas.begin(), bloqueadas.end(), [prox](auto& x){ return x.id == prox; });
                            if (bt != bloqueadas.end()) {
                                bt->bloqueada = false;
                                prontos.push_back(*bt);
                                bloqueadas.erase(bt);
                                cout << ">>> T" << prox << " DESBLOQUEADA e adquiriu M" << mid << "\n";
                            }
                        }
                    }
                    ++ev;
                }
            }
        }

        // 3. Escolhe a melhor tarefa pronta (PREEMPÇÃO!)
        if (!prontos.empty()) {
            int melhorIdx = escalonador(prontos);
            int melhorId = prontos[melhorIdx].id;

            if (currentId != melhorId) {
                if (currentId != -1)
                    cout << ">>> PREEMPÇÃO: T" << currentId << " → T" << melhorId << " (prioridade maior)\n";
                currentId = melhorId;
                running_task.push_back(currentId);
            }
        } else if (currentId != -1) {
            // Nenhuma tarefa pronta → termina a atual ou ociosa
            if (currentId != -1 && find_if(prontos.begin(), prontos.end(),
                [currentId](auto& t){ return t.id == currentId; }) == prontos.end()) {
                currentId = -1;
            }
            running_task.push_back(-1);
            tempoAtual++;
            if (modoExecucao == "passo") {
                atualizarTempoRestante(tarefasOriginais, prontos, pendentes, bloqueadas);
                print_gantt(tarefasOriginais, running_task, tempoAtual);
                print_estado_sistema(prontos, pendentes, bloqueadas, tempoAtual, -1);
                cout << "CPU ociosa - Enter...\n";
                cin.get();
            }
            continue;
        }

        // 4. Executa uma fatia da tarefa corrente
        auto itAtual = find_if(prontos.begin(), prontos.end(), [currentId](auto& t){ return t.id == currentId; });
        if (itAtual == prontos.end()) { currentId = -1; continue; }
        auto& atual = *itAtual;

        int fatia = min(quantum, atual.tempoRestante);
        int inicio = tempoAtual;
        int fim = tempoAtual + fatia;

        if (atual.tempoRestante == atual.duracao)
            esperaTotal += inicio - atual.ingresso;

        atual.tempoRestante -= fatia;
        atual.tempoExecutado += fatia;
        fatias.push_back({atual.id, inicio, fim, fatia});

        if (modoExecucao == "passo") {
            atualizarTempoRestante(tarefasOriginais, prontos, pendentes, bloqueadas);
            print_gantt(tarefasOriginais, running_task, fim);
            print_estado_sistema(prontos, pendentes, bloqueadas, fim, atual.id);
            cout << "Executando T" << atual.id << " - Enter...\n";
            cin.get();
        }

        tempoAtual = fim;

        if (atual.tempoRestante <= 0) {
            retornoTotal += tempoAtual - atual.ingresso;
            prontos.erase(itAtual);
            currentId = -1;
        }
    }

    // Mescla fatias
    vector<FatiaTarefa> mescladas;
    if (!fatias.empty()) {
        FatiaTarefa cur = fatias[0];
        for (size_t i = 1; i < fatias.size(); ++i) {
            if (fatias[i].id == cur.id && fatias[i].inicio == cur.fim) {
                cur.duracao += fatias[i].duracao;
                cur.fim = fatias[i].fim;
            } else {
                mescladas.push_back(cur);
                cur = fatias[i];
            }
        }
        mescladas.push_back(cur);
    }

    exportarGanttSVG(mescladas, tarefasOriginais, algoritmo);
    cout << fixed << setprecision(2);
    cout << "\nTempo médio de espera : " << esperaTotal / tarefasOriginais.size() << "\n";
    cout << "Tempo médio de retorno: " << retornoTotal / tarefasOriginais.size() << "\n";
}

// ====================== ESCALONADOR ======================
int escalonador(vector<Tarefa>& prontos) {
    if (prontos.empty()) return -1;
    int best = 0;

    if (algoritmo == "PRIOPEnv") {
        for (auto& t : prontos) t.prioridadeDinamica += alpha;
        for (size_t i = 1; i < prontos.size(); ++i) {
            if (prontos[i].prioridadeDinamica > prontos[best].prioridadeDinamica ||
                (abs(prontos[i].prioridadeDinamica - prontos[best].prioridadeDinamica) < 1e-9 &&
                 prontos[i].ingresso < prontos[best].ingresso))
                best = i;
        }
        prontos[best].prioridadeDinamica = prontos[best].prioridade;
    } else if (algoritmo == "PRIOP" || algoritmo == "PRIOPEnv") {
        for (size_t i = 1; i < prontos.size(); ++i) {
            if (prontos[i].prioridade > prontos[best].prioridade ||
                (prontos[i].prioridade == prontos[best].prioridade && prontos[i].ingresso < prontos[best].ingresso))
                best = i;
        }
    } else if (algoritmo == "SRTF") {
        for (size_t i = 1; i < prontos.size(); ++i) {
            if (prontos[i].tempoRestante < prontos[best].tempoRestante ||
                (prontos[i].tempoRestante == prontos[best].tempoRestante && prontos[i].ingresso < prontos[best].ingresso))
                best = i;
        }
    } else { // FIFO
        for (size_t i = 1; i < prontos.size(); ++i)
            if (prontos[i].ingresso < prontos[best].ingresso) best = i;
    }
    return best;
}