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

// ==================== CORES ====================

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

// ==================== PRINT DADOS ====================

void print_estado_sistema(const vector<Tarefa> &prontos, const vector<Tarefa> &pendentes, const vector<Tarefa> &bloqueadas, int tempoAtual, int currentTaskId)
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

        // Mostra motivo do bloqueio
        if (t.remainingIO > 0)
        {
            cout << " (E/S: " << t.remainingIO << " unidades restantes)";
        }
        else if (!t.eventosMutex.empty())
        {
            cout << " (Mutex: ";
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

// // ==================== IMPRIME GANTT ====================

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

// ==================== EXPORTAR IMAGEM ====================

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

// ==================== CARREGAR CONFIGURACAO ====================

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
        t.tempoExecutado = 0;
        t.bloqueada = false;
        t.remainingIO = 0;

        for (size_t i = 5; i < campos.size(); ++i)
        {
            string acao = campos[i];
            cout << "DEBUG: Processando evento: " << acao << endl; // Para debug

            // EVENTOS MUTEX: ML01:03 ou MU01:05
            if (acao.length() >= 7 && (acao[0] == 'M') && (acao[1] == 'L' || acao[1] == 'U'))
            {
                try
                {
                    char tipo = acao[1]; // 'L' ou 'U'

                    // Encontra os dois pontos
                    size_t colonPos = acao.find(':');
                    if (colonPos != string::npos && colonPos >= 4)
                    {
                        int mutexId = stoi(acao.substr(2, colonPos - 2));
                        int tempoRel = stoi(acao.substr(colonPos + 1));

                        t.eventosMutex.emplace_back(tempoRel, make_pair(tipo, mutexId));
                        mutexes[mutexId];

                        cout << "DEBUG: Mutex " << tipo << " id=" << mutexId
                             << " t=" << tempoRel << endl;
                    }
                }
                catch (...)
                {
                    cerr << "Erro ao processar mutex: " << acao << endl;
                }
            }
            // EVENTOS E/S: IO:02-03
            else if (acao.length() >= 6 && acao.substr(0, 3) == "IO:")
            {
                try
                {
                    // Remove "IO:"
                    string ioStr = acao.substr(3);

                    // Encontra o hífen
                    size_t dashPos = ioStr.find('-');
                    if (dashPos != string::npos)
                    {
                        string tempoStr = ioStr.substr(0, dashPos);
                        string duraStr = ioStr.substr(dashPos + 1);

                        int tempoRel = stoi(tempoStr);
                        int duracao = stoi(duraStr);

                        t.eventosIO.emplace_back(tempoRel, duracao);

                        cout << "DEBUG: E/S t=" << tempoRel << " dur=" << duracao << endl;
                    }
                    else
                    {
                        // Formato sem hífen: assume tempoRel=1
                        int duracao = stoi(ioStr);
                        t.eventosIO.emplace_back(1, duracao);

                        cout << "DEBUG: E/S (formato antigo) t=1 dur=" << duracao << endl;
                    }
                }
                catch (...)
                {
                    cerr << "Erro ao processar E/S: " << acao << endl;
                }
            }
            else
            {
                cerr << "Formato de evento desconhecido: " << acao << endl;
            }
        }
        sort(t.eventosMutex.begin(), t.eventosMutex.end());

        tarefas.push_back(t);
    }
    return tarefas;
}
// ==================== ENVELHECIMENTO ====================

void aplicarEnvelhecimento(vector<Tarefa> &prontos, int tarefaExecutandoId)
{
    if (algoritmo != "PRIOPEnv")
        return;

    for (auto &t : prontos)
    {
        if (t.id != tarefaExecutandoId)
        {
            t.prioridadeDinamica += alpha;
        }
    }
}

// ==================== SIMULADOR ====================

void simulador(vector<Tarefa> &tarefasOriginais)
{
    // Cria cópia das tarefas originais para manter estado original
    vector<Tarefa> pendentes;
    for (const auto &t : tarefasOriginais) {
        Tarefa nova = t;
        nova.tempoRestante = nova.duracao;
        nova.tempoExecutado = 0;
        nova.bloqueada = false;
        nova.remainingIO = 0;
        nova.prioridadeDinamica = nova.prioridade;
        pendentes.push_back(nova);
    }
    
    // Ordena por tempo de ingresso
    sort(pendentes.begin(), pendentes.end(),
         [](const Tarefa &a, const Tarefa &b)
         { return a.ingresso < b.ingresso; });

    vector<Tarefa> prontos, bloqueadas;
    vector<int> running_task;  // Histórico de execucao para Gantt
    vector<FatiaTarefa> fatias; // Fatias de tempo para SVG

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;
    int currentId = -1;  // ID da tarefa executando atualmente

    // Histórico para modo passo-a-passo (requisito 1.5.2)
    vector<vector<Tarefa>> historicoProntos;
    vector<vector<Tarefa>> historicoPendentes;
    vector<vector<Tarefa>> historicoBloqueadas;
    vector<int> historicoRunningTask;
    vector<int> historicoCurrentId;
    vector<int> historicoTempo;
    vector<map<int, Mutex>> historicoMutexes;

    bool modoPasso = (modoExecucao == "passo");
    int passoAtual = 0;
    bool retroceder = false;

    while (true)
    {
        // SALVA ESTADO ATUAL NO HISTÓRICO (requisito 1.5.2)
        if (modoPasso) {
            historicoProntos.push_back(prontos);
            historicoPendentes.push_back(pendentes);
            historicoBloqueadas.push_back(bloqueadas);
            historicoRunningTask.push_back(currentId);
            historicoCurrentId.push_back(currentId);
            historicoTempo.push_back(tempoAtual);
            historicoMutexes.push_back(mutexes);
        }

        // 1. MOVIMENTA TAREFAS DE PENDENTES PARA PRONTOS (quando chega tempo de ingresso)
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual)
        {
            Tarefa nova = pendentes.front();
            prontos.push_back(nova);
            pendentes.erase(pendentes.begin());
            cout << ">>> T" << nova.id << " ingressou no sistema (t=" << tempoAtual << ")\n";
        }

        // 2. PROCESSAMENTO DE TAREFAS BLOQUEADAS (E/S)
        for (auto it = bloqueadas.begin(); it != bloqueadas.end();)
        {
            if (it->remainingIO > 0)
            {
                it->remainingIO--;
                if (it->remainingIO == 0)
                {
                    cout << ">>> T" << it->id << " concluiu E/S e voltou para pronta (t=" << tempoAtual << ")\n";
                    Tarefa desbloqueada = *it;
                    desbloqueada.bloqueada = false;
                    prontos.push_back(desbloqueada);
                    it = bloqueadas.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            else
            {
                ++it; // Tarefa bloqueada por mutex
            }
        }

        // 3. APLICA ENVELHECIMENTO PARA PRIOPEnv (requisito B.1)
        if (algoritmo == "PRIOPEnv" && !prontos.empty())
        {
            aplicarEnvelhecimento(prontos, currentId);
        }

        // 4. SE NaO HÁ TAREFA EXECUTANDO, ESCOLHE UMA NOVA
        if (currentId == -1 && !prontos.empty())
        {
            int idx = escalonador(prontos);
            if (idx >= 0 && idx < (int)prontos.size())
            {
                currentId = prontos[idx].id;
                cout << ">>> Escalonador escolheu T" << currentId 
                     << " para executar (t=" << tempoAtual << ")\n";
            }
        }

        // 5. VERIFICA FIM DA SIMULAcaO
        bool todasConcluidas = (currentId == -1 && prontos.empty() && 
                               pendentes.empty() && bloqueadas.empty());
        if (todasConcluidas)
        {
            cout << "\n=== SIMULACAO CONCLUIDA no tempo " << tempoAtual << " ===\n";
            break;
        }

        // 6. CPU OCIOSA (nenhuma tarefa para executar)
        if (currentId == -1)
        {
            running_task.push_back(-1); // -1 indica CPU ociosa
            cout << ">>> CPU ociosa (t=" << tempoAtual << ")\n";
            
            // AVANcA TEMPO
            tempoAtual++;
            
            // MODO PASSO-A-PASSO
            if (modoPasso)
            {
                // Exibe estado atual
                print_gantt(tarefasOriginais, running_task, tempoAtual);
                print_estado_sistema(prontos, pendentes, bloqueadas, tempoAtual, currentId);
                
                // Menu de controle
                cout << "\n=== CONTROLE PASSO-A-PASSO ===\n";
                cout << "1. Avancar 1 passo\n";
                cout << "2. Retroceder 1 passo\n";
                cout << "3. Avancar ate o final\n";
                cout << "Escolha: ";
                
                string opcao;
                getline(cin, opcao);
                
                if (opcao == "2" && passoAtual > 0) {
                    // RETROCEDE 1 PASSO
                    passoAtual--;
                    prontos = historicoProntos[passoAtual];
                    pendentes = historicoPendentes[passoAtual];
                    bloqueadas = historicoBloqueadas[passoAtual];
                    currentId = historicoCurrentId[passoAtual];
                    tempoAtual = historicoTempo[passoAtual];
                    mutexes = historicoMutexes[passoAtual];
                    
                    // Ajusta running_task
                    if (running_task.size() > 0) {
                        running_task.pop_back();
                    }
                    
                    // Remove histórico futuro
                    historicoProntos.resize(passoAtual);
                    historicoPendentes.resize(passoAtual);
                    historicoBloqueadas.resize(passoAtual);
                    historicoRunningTask.resize(passoAtual);
                    historicoCurrentId.resize(passoAtual);
                    historicoTempo.resize(passoAtual);
                    historicoMutexes.resize(passoAtual);
                    
                    cout << ">>> Retrocedido para passo " << passoAtual << "\n";
                    continue;
                }
                else if (opcao == "3") {
                    // MUDAR PARA MODO COMPLETO
                    modoPasso = false;
                    cout << ">>> Continuando ate o final...\n";
                }
                else {
                    // AVANcA NORMALMENTE
                    passoAtual++;
                }
            }
            continue;
        }

        // 7. LOCALIZA TAREFA ATUAL NA LISTA DE PRONTOS
        auto itTarefa = find_if(prontos.begin(), prontos.end(),
                               [currentId](const Tarefa &t)
                               { return t.id == currentId; });
        
        if (itTarefa == prontos.end())
        {
            // Tarefa nao encontrada (pode ter sido bloqueada)
            currentId = -1;
            continue;
        }
        
        Tarefa &tarefa = *itTarefa;

        // 8. REGISTRA TEMPO DE ESPERA (quando comeca a executar pela primeira vez)
        if (tarefa.tempoRestante == tarefa.duracao)
        {
            esperaTotal += tempoAtual - tarefa.ingresso;
        }

        // 9. EXECUTA 1 UNIDADE DE TEMPO DA TAREFA
        cout << ">>> T" << tarefa.id << " executando (t=" << tempoAtual 
             << ", restam=" << tarefa.tempoRestante - 1 << ")\n";
        
        tarefa.tempoRestante--;
        tarefa.tempoExecutado++;
        running_task.push_back(tarefa.id);
        fatias.push_back({tarefa.id, tempoAtual, tempoAtual + 1, 1});

        // 10. VERIFICA EVENTOS DE E/S (requisito B.3)
        bool processouEvento = false;
        
        // 10.1 Eventos de E/S
        for (auto itIO = tarefa.eventosIO.begin(); itIO != tarefa.eventosIO.end();)
        {
            if (itIO->first == tarefa.tempoExecutado)
            {
                cout << ">>> T" << tarefa.id << " solicitou E/S por " 
                     << itIO->second << " unidades (t=" << tempoAtual << ")\n";
                
                // Cria cópia da tarefa para lista de bloqueadas
                Tarefa tarefaBloqueada = tarefa;
                tarefaBloqueada.bloqueada = true;
                tarefaBloqueada.remainingIO = itIO->second;
                
                // Remove evento processado
                tarefaBloqueada.eventosIO.erase(
                    tarefaBloqueada.eventosIO.begin() + (itIO - tarefa.eventosIO.begin()));
                
                // Move para bloqueadas
                bloqueadas.push_back(tarefaBloqueada);
                
                // Remove de prontos
                prontos.erase(itTarefa);
                
                currentId = -1;
                processouEvento = true;
                break;
            }
            ++itIO;
        }
        
        if (processouEvento)
        {
            tempoAtual++;
            
            if (modoPasso)
            {
                print_gantt(tarefasOriginais, running_task, tempoAtual);
                print_estado_sistema(prontos, pendentes, bloqueadas, tempoAtual, currentId);
                
                cout << "\n=== CONTROLE PASSO-A-PASSO ===\n";
                cout << "1. Avancar 1 passo\n";
                cout << "2. Retroceder 1 passo\n";
                cout << "3. Avancar ate o final\n";
                cout << "Escolha: ";
                
                string opcao;
                getline(cin, opcao);
                
                if (opcao == "2" && passoAtual > 0) {
                    passoAtual--;
                    // Restaura estado (código similar ao anterior)
                    continue;
                }
                else if (opcao == "3") {
                    modoPasso = false;
                }
                else {
                    passoAtual++;
                }
            }
            continue;
        }

        // 10.2 Eventos de Mutex (requisito B.2)
        for (auto itMutex = tarefa.eventosMutex.begin(); itMutex != tarefa.eventosMutex.end();)
        {
            if (itMutex->first == tarefa.tempoExecutado)
            {
                int mutexId = itMutex->second.second;
                char operacao = itMutex->second.first; // 'L' ou 'U'
                
                if (operacao == 'L') // LOCK (solicitacao)
                {
                    if (mutexes[mutexId].dono == -1)
                    {
                        // Mutex livre, atribui à tarefa
                        mutexes[mutexId].dono = tarefa.id;
                        cout << ">>> T" << tarefa.id << " adquiriu M" 
                             << mutexId << " (t=" << tempoAtual << ")\n";
                    }
                    else
                    {
                        // Mutex ocupado, bloqueia tarefa
                        cout << ">>> T" << tarefa.id << " bloqueou no M" 
                             << mutexId << " (t=" << tempoAtual << ")\n";
                        
                        Tarefa tarefaBloqueada = tarefa;
                        tarefaBloqueada.bloqueada = true;
                        mutexes[mutexId].filaEspera.push_back(tarefa.id);
                        
                        bloqueadas.push_back(tarefaBloqueada);
                        prontos.erase(itTarefa);
                        currentId = -1;
                        processouEvento = true;
                    }
                }
                else if (operacao == 'U') // UNLOCK (liberacao)
                {
                    if (mutexes[mutexId].dono == tarefa.id)
                    {
                        cout << ">>> T" << tarefa.id << " liberou M" 
                             << mutexId << " (t=" << tempoAtual << ")\n";
                        
                        mutexes[mutexId].dono = -1;
                        
                        // Verifica se há tarefas esperando
                        if (!mutexes[mutexId].filaEspera.empty())
                        {
                            int proxId = mutexes[mutexId].filaEspera.front();
                            mutexes[mutexId].filaEspera.erase(
                                mutexes[mutexId].filaEspera.begin());
                            mutexes[mutexId].dono = proxId;
                            
                            // Desbloqueia a tarefa
                            auto itBloqueada = find_if(bloqueadas.begin(), bloqueadas.end(),
                                                      [proxId](const Tarefa &t)
                                                      { return t.id == proxId; });
                            if (itBloqueada != bloqueadas.end())
                            {
                                itBloqueada->bloqueada = false;
                                prontos.push_back(*itBloqueada);
                                bloqueadas.erase(itBloqueada);
                                cout << ">>> T" << proxId << " desbloqueada e adquiriu M" 
                                     << mutexId << "\n";
                            }
                        }
                    }
                }
                
                // Remove evento processado
                tarefa.eventosMutex.erase(itMutex);
                processouEvento = true;
                break;
            }
            ++itMutex;
        }
        
        if (processouEvento)
        {
            tempoAtual++;
            
            if (modoPasso)
            {
                // Exibe estado atual
                print_gantt(tarefasOriginais, running_task, tempoAtual);
                print_estado_sistema(prontos, pendentes, bloqueadas, tempoAtual, currentId);
                
                // Menu de controle
                cout << "\n=== CONTROLE PASSO-A-PASSO ===\n";
                cout << "1. Avancar 1 passo\n";
                cout << "2. Retroceder 1 passo\n";
                cout << "3. Avancar ate o final\n";
                cout << "Escolha: ";
                
                string opcao;
                getline(cin, opcao);
                
                if (opcao == "2" && passoAtual > 0) {
                    // RETROCEDE 1 PASSO
                    passoAtual--;
                    prontos = historicoProntos[passoAtual];
                    pendentes = historicoPendentes[passoAtual];
                    bloqueadas = historicoBloqueadas[passoAtual];
                    currentId = historicoCurrentId[passoAtual];
                    tempoAtual = historicoTempo[passoAtual];
                    mutexes = historicoMutexes[passoAtual];
                    
                    // Ajusta running_task
                    if (running_task.size() > 0) {
                        running_task.pop_back();
                    }
                    
                    // Remove histórico futuro
                    historicoProntos.resize(passoAtual);
                    historicoPendentes.resize(passoAtual);
                    historicoBloqueadas.resize(passoAtual);
                    historicoRunningTask.resize(passoAtual);
                    historicoCurrentId.resize(passoAtual);
                    historicoTempo.resize(passoAtual);
                    historicoMutexes.resize(passoAtual);
                    
                    cout << ">>> Retrocedido para passo " << passoAtual << "\n";
                    continue;
                }
                else if (opcao == "3") {
                    // MUDAR PARA MODO COMPLETO
                    modoPasso = false;
                    cout << ">>> Continuando ate o final...\n";
                }
                else {
                    // AVANcA NORMALMENTE
                    passoAtual++;
                }
            }
            continue;
        }

        // 11. VERIFICA SE TAREFA TERMINOU APÓS EXECUcaO
        if (tarefa.tempoRestante <= 0)
        {
            retornoTotal += tempoAtual + 1 - tarefa.ingresso;
            cout << ">>> T" << tarefa.id << " terminou execucao (t=" 
                 << tempoAtual + 1 << ")\n";
            
            prontos.erase(itTarefa);
            currentId = -1;
        }

        // 12. AVANcA TEMPO
        tempoAtual++;
        
        // 13. MODO PASSO-A-PASSO (após execucao normal)
        if (modoPasso && !processouEvento)
        {
            print_gantt(tarefasOriginais, running_task, tempoAtual);
            print_estado_sistema(prontos, pendentes, bloqueadas, tempoAtual, currentId);
            
            cout << "\n=== CONTROLE PASSO-A-PASSO ===\n";
            cout << "1. Avancar 1 passo\n";
            cout << "2. Retroceder 1 passo\n";
            cout << "3. Avancar ate o final\n";
            cout << "Escolha: ";
            
            string opcao;
            getline(cin, opcao);
            
            if (opcao == "2" && passoAtual > 0) {
                passoAtual--;
                // Restaura estado
                continue;
            }
            else if (opcao == "3") {
                modoPasso = false;
                cout << ">>> Continuando ate o final...\n";
            }
            else {
                passoAtual++;
            }
        }
    }

    // 14. EXIBE RESULTADOS FINAIS
    cout << "\n" << string(60, '=') << "\n";
    cout << "RESUMO DA EXECUCAO - Grafico de Gantt\n";
    cout << string(60, '=') << "\n";

    print_gantt(tarefasOriginais, running_task, tempoAtual);

    // 15. GERA SVG FINAL (requisito 2.3)
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
    
    // 16. ESTATÍSTICAS FINAIS
    cout << fixed << setprecision(2);
    cout << "\n=== ESTATISTICAS FINAIS ===\n";
    cout << "Tempo total de simulacao: " << tempoAtual << "\n";
    cout << "Tempo medio de espera: " << esperaTotal / tarefasOriginais.size() << "\n";
    cout << "Tempo medio de retorno: " << retornoTotal / tarefasOriginais.size() << "\n";
    cout << "Algoritmo usado: " << algoritmo << "\n";
    if (algoritmo == "PRIOPEnv") {
        cout << "Alpha (envelhecimento): " << alpha << "\n";
    }
}

// ==================== ESCALONADOR ====================

int escalonador(std::vector<Tarefa> &prontos)
{
    if (prontos.empty())
        return -1;

    int best = 0;

    if (algoritmo == "SRTF")
    {
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
    else if (algoritmo == "PRIOP" || algoritmo == "PRIO")
    {
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
    else if (algoritmo == "FIFO" || algoritmo == "SJF")
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