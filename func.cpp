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

    // Encontra tempo mAximo
    int tempoMax = 0;
    for (const auto &f : fatias)
        tempoMax = max(tempoMax, f.fim);

    // Adiciona margem para tarefas que podem terminar depois
    for (const auto &t : tarefas)
        tempoMax = max(tempoMax, t.ingresso + t.duracao + 2);

    // Agrupa fatias por tarefa
    map<int, vector<FatiaTarefa>> porTarefa;
    for (const auto &f : fatias)
        porTarefa[f.id].push_back(f);

    // Encontra cores das tarefas
    map<int, string> coresTarefas;
    for (const auto &t : tarefas)
        coresTarefas[t.id] = get_hex_color(t.cor);

    // Encontra tempos de ingresso
    map<int, int> ingressos;
    for (const auto &t : tarefas)
        ingressos[t.id] = t.ingresso;

    ofstream svg(nome + "_gantt.svg");
    if (!svg)
        return;

    double escala = 40.0; // 40 pixels por unidade de tempo
    int alturaLinha = 30;
    int espacamentoLinhas = 10;
    int margemSuperior = 80;
    int margemEsquerda = 80;
    int larguraTotal = margemEsquerda + (int)(tempoMax * escala) + 50;
    int alturaTotal = margemSuperior + (int)porTarefa.size() * (alturaLinha + espacamentoLinhas) + 50;

    svg << "<svg width=\"" << larguraTotal << "\" height=\"" << alturaTotal
        << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

    // Fundo branco
    svg << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";

    // Titulo
    svg << "<text x=\"" << larguraTotal / 2 << "\" y=\"30\" font-size=\"20\" "
        << "text-anchor=\"middle\" font-family=\"Arial\" font-weight=\"bold\">"
        << nome << "</text>\n";

    // Linha do tempo
    int yLinhaTempo = margemSuperior - 10;
    svg << "<line x1=\"" << margemEsquerda << "\" y1=\"" << yLinhaTempo
        << "\" x2=\"" << margemEsquerda + (int)(tempoMax * escala)
        << "\" y2=\"" << yLinhaTempo
        << "\" stroke=\"black\" stroke-width=\"2\"/>\n";

    // Marcadores de tempo
    for (int t = 0; t <= tempoMax; t++)
    {
        int x = margemEsquerda + (int)(t * escala);

        // Marcador pequeno para cada unidade
        svg << "<line x1=\"" << x << "\" y1=\"" << (yLinhaTempo - 3)
            << "\" x2=\"" << x << "\" y2=\"" << (yLinhaTempo + 3)
            << "\" stroke=\"black\" stroke-width=\"1\"/>\n";

        // Número a cada 5 unidades ou no inicio/fim
        if (t == 0 || t == tempoMax || t % 5 == 0)
        {
            svg << "<text x=\"" << x << "\" y=\"" << (yLinhaTempo - 15)
                << "\" font-size=\"12\" text-anchor=\"middle\" font-family=\"Arial\">"
                << t << "</text>\n";

            // Linha vertical tracejada
            svg << "<line x1=\"" << x << "\" y1=\"" << yLinhaTempo
                << "\" x2=\"" << x << "\" y2=\""
                << margemSuperior + (int)porTarefa.size() * (alturaLinha + espacamentoLinhas)
                << "\" stroke=\"#ccc\" stroke-width=\"1\" stroke-dasharray=\"2,2\"/>\n";
        }
    }

    // Desenha tarefas
    int y = margemSuperior;
    for (const auto &[id, lista] : porTarefa)
    {
        string cor = "#95A5A6";
        if (coresTarefas.find(id) != coresTarefas.end())
            cor = coresTarefas[id];

        // Label da tarefa
        svg << "<text x=\"" << margemEsquerda - 10 << "\" y=\"" << y + alturaLinha / 2 + 5
            << "\" font-size=\"14\" text-anchor=\"end\" font-family=\"Arial\">"
            << "T" << id << "</text>\n";

        // Marcacao de ingresso (linha vertical com marcador)
        if (ingressos.find(id) != ingressos.end())
        {
            int xIngresso = margemEsquerda + (int)(ingressos[id] * escala);

            // Linha vertical do ingresso
            svg << "<line x1=\"" << xIngresso << "\" y1=\"" << y
                << "\" x2=\"" << xIngresso << "\" y2=\"" << (y + alturaLinha)
                << "\" stroke=\"" << cor << "\" stroke-width=\"2\" stroke-dasharray=\"3,3\"/>\n";

            // Triângulo indicador
            svg << "<polygon points=\""
                << xIngresso << "," << (y - 5) << " "
                << (xIngresso - 5) << "," << y << " "
                << (xIngresso + 5) << "," << y
                << "\" fill=\"" << cor << "\"/>\n";
        }

        // Fatias de execucao
        for (const auto &f : lista)
        {
            int x = margemEsquerda + (int)(f.inicio * escala);
            int w = max(2, (int)(f.duracao * escala));

            // Retângulo da fatia
            svg << "<rect x=\"" << x << "\" y=\"" << y
                << "\" width=\"" << w << "\" height=\"" << alturaLinha
                << "\" fill=\"" << cor << "\" rx=\"2\" ry=\"2\" "
                << "stroke=\"#333\" stroke-width=\"1\"/>\n";
        }

        y += alturaLinha + espacamentoLinhas;
    }

    svg << "</svg>";
    svg.close();

    cout << "GrAfico Gantt salvo como " << nome << "_gantt.svg\n";
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
        // Remove espaços em branco no início e fim
        linha.erase(0, linha.find_first_not_of(" \t"));
        linha.erase(linha.find_last_not_of(" \t") + 1);

        if (linha.empty() || linha[0] == '#')
            continue;

        stringstream ss(linha);
        string tok;

        if (primeira)
        {
            getline(ss, algoritmo, ';');
            // Remove espaços do algoritmo
            algoritmo.erase(remove(algoritmo.begin(), algoritmo.end(), ' '), algoritmo.end());

            if (getline(ss, tok, ';'))
            {
                tok.erase(remove(tok.begin(), tok.end(), ' '), tok.end());
                quantum = tok.empty() ? 2 : stoi(tok);
            }

            if (algoritmo == "PRIOPEnv")
            {
                if (getline(ss, tok, ';'))
                {
                    tok.erase(remove(tok.begin(), tok.end(), ' '), tok.end());
                    alpha = tok.empty() ? 0.6 : stod(tok);
                }
            }
            primeira = false;
            continue;
        }

        Tarefa t{};
        vector<string> campos;
        while (getline(ss, tok, ';'))
            campos.push_back(tok);

        if (campos.size() < 5)
        {
            cerr << "Erro: linha com menos de 5 campos: " << linha << endl;
            continue;
        }

        // CONVERTE ID NO FORMATO "tXX" PARA NÚMERO
        string idStr = campos[0];
        int idNum = 0;

        // Remove espaços
        idStr.erase(remove(idStr.begin(), idStr.end(), ' '), idStr.end());

        // Remove 't' do início se existir
        if (!idStr.empty() && (idStr[0] == 't' || idStr[0] == 'T'))
        {
            idStr = idStr.substr(1); // Remove primeiro caractere
        }

        try
        {
            if (!idStr.empty())
            {
                idNum = stoi(idStr);
            }
            else
            {
                cerr << "Erro: ID vazio na linha: " << linha << endl;
                continue;
            }
        }
        catch (const std::invalid_argument &e)
        {
            cerr << "Erro: ID invalido '" << campos[0] << "' na linha: " << linha << endl;
            continue;
        }

        t.id = idNum;

        // Cor (pode ter # ou não)
        t.cor = campos[1];
        t.cor.erase(remove(t.cor.begin(), t.cor.end(), ' '), t.cor.end());

        // Ingresso
        try
        {
            string ingressoStr = campos[2];
            ingressoStr.erase(remove(ingressoStr.begin(), ingressoStr.end(), ' '), ingressoStr.end());
            t.ingresso = ingressoStr.empty() ? 0 : stoi(ingressoStr);
        }
        catch (const std::invalid_argument &e)
        {
            cerr << "Erro: ingresso invalido '" << campos[2] << "' para T" << t.id << endl;
            t.ingresso = 0;
        }

        // Duração
        try
        {
            string duracaoStr = campos[3];
            duracaoStr.erase(remove(duracaoStr.begin(), duracaoStr.end(), ' '), duracaoStr.end());
            t.duracao = duracaoStr.empty() ? 0 : stoi(duracaoStr);
        }
        catch (const std::invalid_argument &e)
        {
            cerr << "Erro: duracao invalida '" << campos[3] << "' para T" << t.id << endl;
            t.duracao = 0;
        }

        // Prioridade
        try
        {
            string prioridadeStr = campos[4];
            prioridadeStr.erase(remove(prioridadeStr.begin(), prioridadeStr.end(), ' '), prioridadeStr.end());
            t.prioridade = prioridadeStr.empty() ? 1 : stoi(prioridadeStr);
        }
        catch (const std::invalid_argument &e)
        {
            cerr << "Erro: prioridade invalida '" << campos[4] << "' para T" << t.id << endl;
            t.prioridade = 1;
        }

        t.prioridadeDinamica = t.prioridade;
        t.tempoRestante = t.duracao;
        t.tempoExecutado = 0;
        t.bloqueada = false;
        t.remainingIO = 0;

        // Processa eventos (campos 5 em diante)
        for (size_t i = 5; i < campos.size(); ++i)
        {
            string acao = campos[i];
            // Remove espaços
            acao.erase(remove(acao.begin(), acao.end(), ' '), acao.end());

            if (acao.empty())
                continue;

            // EVENTOS MUTEX: ML01:03 ou MU01:05 ou MU01:00 ou MU01:0
            if (acao.length() >= 6 && (acao[0] == 'M') && (acao[1] == 'L' || acao[1] == 'U'))
            {
                try
                {
                    char tipo = acao[1]; // 'L' ou 'U'

                    // Encontra os dois pontos
                    size_t colonPos = acao.find(':');
                    if (colonPos != string::npos && colonPos >= 3) // Mínimo: "MU1:0"
                    {
                        string mutexIdStr = acao.substr(2, colonPos - 2);
                        int mutexId = 0;

                        // Remove zeros à esquerda se necessário
                        if (!mutexIdStr.empty())
                        {
                            // Remove todos os zeros à esquerda
                            size_t firstNonZero = mutexIdStr.find_first_not_of('0');
                            if (firstNonZero != string::npos)
                            {
                                mutexIdStr = mutexIdStr.substr(firstNonZero);
                            }
                            else
                            {
                                mutexIdStr = "0"; // Se era só zeros
                            }

                            if (!mutexIdStr.empty())
                            {
                                mutexId = stoi(mutexIdStr);
                            }
                        }

                        string tempoStr = acao.substr(colonPos + 1);
                        tempoStr.erase(remove(tempoStr.begin(), tempoStr.end(), ' '), tempoStr.end());

                        // Remove zeros à esquerda do tempo também
                        size_t firstNonZeroTime = tempoStr.find_first_not_of('0');
                        if (firstNonZeroTime != string::npos)
                        {
                            tempoStr = tempoStr.substr(firstNonZeroTime);
                        }
                        else
                        {
                            tempoStr = "0"; // Se era só zeros
                        }

                        int tempoRel = tempoStr.empty() ? 0 : stoi(tempoStr);

                        t.eventosMutex.emplace_back(tempoRel, make_pair(tipo, mutexId));
                        mutexes[mutexId]; // Garante que o mutex existe

                        cout << "DEBUG: Mutex " << tipo << " id=" << mutexId
                             << " t=" << tempoRel << " para T" << t.id << endl;
                    }
                    else
                    {
                        cerr << "Erro: Formato de mutex inválido '" << acao
                             << "' para T" << t.id << endl;
                    }
                }
                catch (const exception &e)
                {
                    cerr << "Erro ao processar mutex '" << acao << "' para T" << t.id
                         << " - " << e.what() << endl;
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

                        // Remove espaços
                        tempoStr.erase(remove(tempoStr.begin(), tempoStr.end(), ' '), tempoStr.end());
                        duraStr.erase(remove(duraStr.begin(), duraStr.end(), ' '), duraStr.end());

                        int tempoRel = stoi(tempoStr);
                        int duracao = stoi(duraStr);

                        t.eventosIO.emplace_back(tempoRel, duracao);

                        cout << "DEBUG: E/S t=" << tempoRel << " dur=" << duracao
                             << " para T" << t.id << endl;
                    }
                    else
                    {
                        // Formato sem hífen
                        ioStr.erase(remove(ioStr.begin(), ioStr.end(), ' '), ioStr.end());
                        int duracao = stoi(ioStr);
                        t.eventosIO.emplace_back(1, duracao);

                        cout << "DEBUG: E/S t=1 dur=" << duracao
                             << " para T" << t.id << endl;
                    }
                }
                catch (const exception &e)
                {
                    cerr << "Erro ao processar E/S '" << acao << "' para T" << t.id
                         << " - " << e.what() << endl;
                }
            }
            else if (!acao.empty())
            {
                cerr << "Formato de evento desconhecido: '" << acao
                     << "' para T" << t.id << endl;
            }
        }

        // Ordena eventos de mutex por tempo
        sort(t.eventosMutex.begin(), t.eventosMutex.end());

        tarefas.push_back(t);
    }

    if (tarefas.empty())
    {
        cerr << "Aviso: Nenhuma tarefa carregada do arquivo!\n";
    }
    else
    {
        cout << "Carregadas " << tarefas.size() << " tarefas do arquivo." << endl;
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
            cout << ">>> Envelhecimento: T" << t.id << " prio "
                 << (t.prioridadeDinamica - alpha) << " -> " << t.prioridadeDinamica << endl;
        }
    }
}

// ==================== SIMULADOR ====================

void simulador(vector<Tarefa> &tarefasOriginais)
{
    // Cria cópia das tarefas originais para manter estado original
    vector<Tarefa> pendentes;
    for (const auto &t : tarefasOriginais)
    {
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
    vector<int> running_task;
    vector<FatiaTarefa> fatias;

    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;
    int currentId = -1;
    int quantumCounter = quantum;

    bool modoPasso = (modoExecucao == "passo");

    while (true)
    {
        cout << "\n=== CICLO tempo=" << tempoAtual << " ===" << endl;
        cout << "Estado: currentId=" << currentId
             << ", Prontos=" << prontos.size()
             << ", Pendentes=" << pendentes.size()
             << ", Bloqueadas=" << bloqueadas.size() << endl;

        // 1. MOVIMENTA TAREFAS DE PENDENTES PARA PRONTOS
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual)
        {
            Tarefa nova = pendentes.front();
            prontos.push_back(nova);
            pendentes.erase(pendentes.begin());
            cout << ">>> T" << nova.id << " ingressou (t=" << tempoAtual << ")" << endl;
        }

        // 2. APLICA ENVELHECIMENTO PARA PRIOPEnv
        if (algoritmo == "PRIOPEnv" && currentId != -1 && !prontos.empty())
        {
            aplicarEnvelhecimento(prontos, currentId);
        }

        // 3. PROCESSAMENTO DE TAREFAS BLOQUEADAS (E/S)
        vector<Tarefa> paraDesbloquear;
        for (auto it = bloqueadas.begin(); it != bloqueadas.end();)
        {
            if (it->remainingIO > 0)
            {
                it->remainingIO--;
                cout << ">>> T" << it->id << " em E/S, restam " << it->remainingIO << endl;
                if (it->remainingIO == 0)
                {
                    cout << ">>> T" << it->id << " concluiu E/S" << endl;
                    Tarefa desbloqueada = *it;
                    desbloqueada.bloqueada = false;
                    // Resetar prioridade dinâmica ao voltar para prontos
                    // Mantém prioridade dinâmica (não reseta completamente)
                    // Garante que seja pelo menos a prioridade base
                    desbloqueada.prioridadeDinamica = max(desbloqueada.prioridadeDinamica, static_cast<double>(desbloqueada.prioridade));
                    paraDesbloquear.push_back(desbloqueada);
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

        // Adiciona tarefas desbloqueadas de volta
        for (auto &t : paraDesbloquear)
        {
            prontos.push_back(t);
        }

        // Deadlock detection: se não há prontos nem pendentes, mas há bloqueadas sem IO, provavelmente há deadlock.
        if (currentId == -1 && prontos.empty() && pendentes.empty() && !bloqueadas.empty())
        {
            bool anyIO = false;
            for (const auto &b : bloqueadas)
            {
                if (b.remainingIO > 0)
                {
                    anyIO = true;
                    break;
                }
            }
            if (!anyIO)
            {
                cout << ">>> ERRO: Deadlock detectado no tempo " << tempoAtual
                     << " - tarefas bloqueadas por mutex sem E/S em andamento!" << "\n";

                cout << "   Bloqueadas: ";
                for (const auto &b : bloqueadas)
                    cout << "T" << b.id << " ";
                cout << "\n";

                cout << "   Estado dos mutexes:\n";
                for (const auto &p : mutexes)
                {
                    const auto &m = p.second;
                    cout << "    M" << setw(2) << p.first << " -> dono="
                         << ((m.dono == -1) ? string("LIVRE") : (string("T") + to_string(m.dono)));
                    if (!m.filaEspera.empty())
                    {
                        cout << " (espera:";
                        for (int x : m.filaEspera)
                            cout << " T" << x;
                        cout << ")";
                    }
                    cout << "\n";
                }

                cout << ">>> Interrompendo simulacao para evitar loop infinito." << "\n";
                break;
            }
        }

        // 4. VERIFICA SE HÁ TAREFAS PARA EXECUTAR
        if (currentId == -1 && !prontos.empty())
        {
            int idx = escalonador(prontos);
            if (idx >= 0 && idx < (int)prontos.size())
            {
                currentId = prontos[idx].id;
                quantumCounter = quantum;
                cout << ">>> Escalonador escolheu T" << currentId << " (idx=" << idx << ")" << endl;

                if (algoritmo == "PRIOPEnv")
                {
                    // Ao iniciar execução, garante que prioridade dinâmica seja a base
                    prontos[idx].prioridadeDinamica = prontos[idx].prioridade;
                }
            }
            else if (prontos.empty())
            {
                cout << ">>> AVISO: Escalonador retornou -1 mas há " << prontos.size() << " tarefas prontas!" << endl;
            }
        }

        // 5. VERIFICA FIM DA SIMULAÇÃO
        bool todasConcluidas = (currentId == -1 && prontos.empty() &&
                                pendentes.empty() && bloqueadas.empty());
        if (todasConcluidas)
        {
            cout << "\n=== FIM DA SIMULAÇÃO no tempo " << tempoAtual << " ===" << endl;
            break;
        }

        // 6. SE NÃO HÁ TAREFA PARA EXECUTAR, AVANÇA O TEMPO
        if (currentId == -1)
        {
            cout << ">>> CPU ociosa (t=" << tempoAtual << ")" << endl;
            cout << "   Prontos: ";
            for (const auto &t : prontos)
                cout << "T" << t.id << " ";
            cout << endl;
            cout << "   Bloqueadas: ";
            for (const auto &t : bloqueadas)
                cout << "T" << t.id << " ";
            cout << endl;
            cout << "   Pendentes: ";
            for (const auto &t : pendentes)
                cout << "T" << t.id << "(" << t.ingresso << ") ";
            cout << endl;

            running_task.push_back(-1);
            tempoAtual++;

            if (modoPasso)
            {
                print_gantt(tarefasOriginais, running_task, tempoAtual);
                print_estado_sistema(prontos, pendentes, bloqueadas, tempoAtual, currentId);
                cout << "Pressione Enter...";
                cin.get();
            }
            continue;
        }

        // 7. ENCONTRA TAREFA ATUAL
        auto itTarefa = find_if(prontos.begin(), prontos.end(),
                                [currentId](const Tarefa &t)
                                { return t.id == currentId; });

        if (itTarefa == prontos.end())
        {
            cout << ">>> ERRO: T" << currentId << " não encontrada em prontos!" << endl;
            cout << "   Tarefas em prontos: ";
            for (const auto &t : prontos)
                cout << "T" << t.id << " ";
            cout << endl;
            currentId = -1;
            quantumCounter = quantum;
            continue;
        }

        Tarefa &tarefa = *itTarefa;

        // 8. REGISTRA ESPERA (primeira execução)
        if (tarefa.tempoRestante == tarefa.duracao)
        {
            esperaTotal += tempoAtual - tarefa.ingresso;
        }

        // 9. EXECUTA 1 UNIDADE
        cout << ">>> Executando T" << tarefa.id << " (restam " << tarefa.tempoRestante - 1
             << ", executado=" << tarefa.tempoExecutado << ")" << endl;

        tarefa.tempoRestante--;
        tarefa.tempoExecutado++;
        running_task.push_back(tarefa.id);
        fatias.push_back({tarefa.id, tempoAtual, tempoAtual + 1, 1});
        quantumCounter--;

        // 10. VERIFICA EVENTOS
        bool bloqueouPorIO = false;
        bool bloqueouPorMutex = false;
        int tarefaId = tarefa.id;

        // 10.1 Eventos de E/S
        for (auto itIO = tarefa.eventosIO.begin(); itIO != tarefa.eventosIO.end();)
        {
            if (itIO->first == tarefa.tempoExecutado)
            {
                cout << ">>> T" << tarefa.id << " inicia E/S por " << itIO->second << " unidades" << endl;

                // Remover evento da tarefa original antes de copiar
                auto evento = *itIO;
                tarefa.eventosIO.erase(itIO);

                Tarefa tarefaBloqueada = tarefa;
                tarefaBloqueada.bloqueada = true;
                tarefaBloqueada.remainingIO = evento.second;

                // Adiciona às bloqueadas
                bloqueadas.push_back(tarefaBloqueada);

                // Marca para remover
                bloqueouPorIO = true;
                break;
            }
            else
            {
                ++itIO;
            }
        }

        // 10.2 Eventos de Mutex (só se não bloqueou por E/S)
        if (!bloqueouPorIO)
        {
            for (auto itMutex = tarefa.eventosMutex.begin(); itMutex != tarefa.eventosMutex.end();)
            {
                if (itMutex->first == tarefa.tempoExecutado)
                {
                    int mutexId = itMutex->second.second;
                    char op = itMutex->second.first;

                    cout << ">>> T" << tarefa.id << " evento MUTEX " << op << " para M" << mutexId << endl;

                    if (op == 'L') // LOCK
                    {
                        // Remova o evento da tarefa original primeiro para evitar que a cópia da bloqueada mantenha o evento
                        auto evento = *itMutex;
                        tarefa.eventosMutex.erase(itMutex);

                        if (mutexes[mutexId].dono == -1)
                        {
                            mutexes[mutexId].dono = tarefa.id;
                            cout << ">>> T" << tarefa.id << " adquiriu M" << mutexId << endl;
                        }
                        else
                        {
                            cout << ">>> T" << tarefa.id << " bloqueou no M" << mutexId
                                 << " (dono=T" << mutexes[mutexId].dono << ")" << endl;

                            Tarefa tarefaBloqueada = tarefa;
                            tarefaBloqueada.bloqueada = true;

                            // garante que não insere duplicado na fila de espera
                            if (find(mutexes[mutexId].filaEspera.begin(), mutexes[mutexId].filaEspera.end(), tarefa.id) == mutexes[mutexId].filaEspera.end())
                                mutexes[mutexId].filaEspera.push_back(tarefa.id);

                            bloqueadas.push_back(tarefaBloqueada);
                            bloqueouPorMutex = true;
                        }
                    }
                    else if (op == 'U') // UNLOCK
                    {
                        // Remova o evento da tarefa original primeiro
                        auto evento = *itMutex;
                        tarefa.eventosMutex.erase(itMutex);

                        if (mutexes[mutexId].dono == tarefa.id)
                        {
                            cout << ">>> T" << tarefa.id << " liberou M" << mutexId << endl;
                            mutexes[mutexId].dono = -1;

                            if (!mutexes[mutexId].filaEspera.empty())
                            {
                                int proxId = mutexes[mutexId].filaEspera.front();
                                mutexes[mutexId].filaEspera.erase(mutexes[mutexId].filaEspera.begin());
                                mutexes[mutexId].dono = proxId;

                                auto itBloq = find_if(bloqueadas.begin(), bloqueadas.end(),
                                                      [proxId](const Tarefa &t)
                                                      { return t.id == proxId; });
                                if (itBloq != bloqueadas.end())
                                {
                                    itBloq->bloqueada = false;
                                    // Resetar prioridade dinâmica ao voltar para prontos
                                    // Mantém prioridade dinâmica (não reseta completamente)
                                    itBloq->prioridadeDinamica = max(itBloq->prioridadeDinamica, static_cast<double>(itBloq->prioridade));
                                    prontos.push_back(*itBloq);
                                    bloqueadas.erase(itBloq);
                                    cout << ">>> T" << proxId << " desbloqueada" << endl;
                                }
                            }
                        }
                    }
                    break;
                }
                else
                {
                    ++itMutex;
                }
            }
        }

        // 11. REMOVE TAREFA SE BLOQUEOU
        if (bloqueouPorIO || bloqueouPorMutex)
        {
            prontos.erase(remove_if(prontos.begin(), prontos.end(),
                                    [tarefaId](const Tarefa &t)
                                    { return t.id == tarefaId; }),
                          prontos.end());

            currentId = -1;
            quantumCounter = quantum;
        }
        // 12. VERIFICA SE TERMINOU
        else if (tarefa.tempoRestante <= 0)
        {
            retornoTotal += tempoAtual + 1 - tarefa.ingresso;
            cout << ">>> T" << tarefa.id << " terminou" << endl;

            prontos.erase(remove_if(prontos.begin(), prontos.end(),
                                    [tarefaId](const Tarefa &t)
                                    { return t.id == tarefaId; }),
                          prontos.end());

            currentId = -1;
            quantumCounter = quantum;
        }
        // 13. VERIFICA QUANTUM
        else if ((algoritmo == "RR" || algoritmo == "PRIOP" || algoritmo == "PRIOPEnv") &&
                 quantumCounter <= 0)
        {
            cout << ">>> Quantum expirado para T" << tarefa.id << endl;
            currentId = -1;
            quantumCounter = quantum;
        }

        // 14. AVANÇA TEMPO
        tempoAtual++;

        if (modoPasso)
        {
            print_gantt(tarefasOriginais, running_task, tempoAtual);
            print_estado_sistema(prontos, pendentes, bloqueadas, tempoAtual, currentId);
            cout << "Pressione Enter...";
            cin.get();
        }

        // Limitar tempo máximo para evitar loop infinito
        if (tempoAtual > 10000)
        {
            cout << "\n>>> ERRO: Tempo máximo excedido (10000 unidades). Loop infinito detectado!" << endl;
            cout << "   Prontos: " << prontos.size() << ", Bloqueadas: " << bloqueadas.size()
                 << ", Pendentes: " << pendentes.size() << endl;
            break;
        }
    }

    // 14. EXIBE RESULTADOS FINAIS
    cout << "\n"
         << string(60, '=') << "\n";
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

    // 16. ESTATiSTICAS FINAIS
    cout << fixed << setprecision(2);
    cout << "\n=== ESTATISTICAS FINAIS ===\n";
    cout << "Tempo total de simulacao: " << tempoAtual << "\n";
    cout << "Tempo medio de espera: " << esperaTotal / tarefasOriginais.size() << "\n";
    cout << "Tempo medio de retorno: " << retornoTotal / tarefasOriginais.size() << "\n";
    cout << "Algoritmo usado: " << algoritmo << "\n";
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
