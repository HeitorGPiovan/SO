#include "func.h"   
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> 
#include <iomanip>   


int quantum;
std::string algoritmo;

void fifo(vector<Tarefa>& tarefas)
{
    cout << "Executando FIFO..." << endl;

    // Ordena as tarefas pelo tempo de ingresso (ordem de chegada) usando std::sort
    sort(tarefas.begin(), tarefas.end(), [](const Tarefa& a, const Tarefa& b) {
        return a.ingresso < b.ingresso;
    });

    int tempoAtual = 0;     // Inicializa o tempo atual da CPU
    double esperaTotal = 0, retornoTotal = 0;    // Variáveis para acumular os tempos totais de espera e retorno
    vector<Tarefa> tarefasPendentes = tarefas;    // Cria uma cópia do vetor de tarefas para rastrear o tempo restante sem modificar o original
    
    cout << "\nOrdem de execucao (FIFO):\n";    // Exibe cabeçalho da tabela de execução

    for (auto& t : tarefasPendentes) {

        t.tempoRestante = t.duracao;        // Inicializa o tempo restante da tarefa como sua duração total


        if (tempoAtual < t.ingresso) {        // Se o tempo atual for menor que o tempo de ingresso, a CPU fica ociosa até a chegada

            tempoAtual = t.ingresso;
        }

        // Processa a tarefa em fatias de até 2 tempos (quantum fixo) até completar
        while (t.tempoRestante > 0) {

            int inicio = tempoAtual;            // Marca o início da fatia atual
            int duracaoFatia = min(t.tempoRestante, 2);            // Calcula a duração da fatia: mínimo entre o tempo restante e o quantum (2)
            int fim = tempoAtual + duracaoFatia;            // Marca o fim da fatia
            int espera = inicio - t.ingresso;            // Calcula a espera (tempo desde o ingresso até o início da fatia)
            int retorno = fim - t.ingresso;            // Calcula o retorno (tempo desde o ingresso até o fim da fatia)


            if (t.tempoRestante == t.duracao) {            // Soma a espera apenas na primeira fatia da tarefa

                esperaTotal += espera;
            }
            if (t.tempoRestante - duracaoFatia <= 0) {            // Soma o retorno apenas na última fatia, quando a tarefa é concluída

                retornoTotal += retorno;
            }

            // Exibe informações da fatia com formatação alinhada
            cout << "Tarefa " << setw(2) << t.id
                 << " | Ingresso: " << setw(2) << t.ingresso
                 << " | Duracao fatia: " << setw(2) << duracaoFatia
                 << " | Inicio: " << setw(2) << inicio
                 << " | Fim: " << setw(2) << fim
                 << " | Espera: " << setw(2) << espera
                 << " | Retorno: " << setw(2) << retorno
                 << endl;

            tempoAtual = fim;            // Avança o tempo atual para o fim da fatia

            t.tempoRestante -= duracaoFatia;            // Reduz o tempo restante da tarefa pela duração da fatia

        }
    }

    int n = tarefas.size();    // Calcula o número de tarefas para médias

    cout << fixed << setprecision(2);    // Configura a saída para exibir números com 2 casas decimais

    // Exibe os tempos médios de espera e retorno
    cout << "\nTempo medio de espera: " << (esperaTotal / n)
         << "\nTempo medio de retorno: " << (retornoTotal / n)
         << endl;
}

void srtf(vector<Tarefa>& tarefas)
{
    cout << "Executando SRTF" << endl;

    // Cria uma cópia do vetor de tarefas para não modificar o original
    vector<Tarefa> tarefasPendentes = tarefas;

    // Ordena as tarefas pendentes pelo tempo de ingresso
    sort(tarefasPendentes.begin(), tarefasPendentes.end(), [](const Tarefa& a, const Tarefa& b) {
        return a.ingresso < b.ingresso;
    });

    for (auto& t : tarefasPendentes) {    // Inicializa o tempo restante para cada tarefa

        t.tempoRestante = t.duracao;
    }

    // Inicializa variáveis
    int tempoAtual = 0;
    double esperaTotal = 0, retornoTotal = 0;

    vector<Tarefa> pendentes = tarefasPendentes;    // Vetor para tarefas pendentes (ainda não chegadas)

    vector<Tarefa> prontos;    // Vetor para tarefas prontas (chegadas e não concluídas)


    cout << "\nOrdem de execucao (SRTF):\n";

    while (!pendentes.empty() || !prontos.empty()) {    // Loop principal: continua enquanto houver tarefas pendentes ou prontas

        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual) {        // Adiciona tarefas que chegaram ao tempo atual ao vetor de prontos

            prontos.push_back(pendentes.front());
            pendentes.erase(pendentes.begin());
        }

        if (prontos.empty()) {        // Se não houver tarefas prontas, avança para a próxima chegada

            if (!pendentes.empty()) {
                tempoAtual = pendentes.front().ingresso;
            }
            continue;
        }

        sort(prontos.begin(), prontos.end(), [](const Tarefa& a, const Tarefa& b) {        // Ordena as tarefas prontas pelo menor tempo restante (desempate por ingresso)

            if (a.tempoRestante != b.tempoRestante) {
                return a.tempoRestante < b.tempoRestante;
            }
            return a.ingresso < b.ingresso;
        });

        Tarefa& atual = prontos.front();        // Seleciona a tarefa atual (a com menor tempo restante)


        int duracaoFatia = min(atual.tempoRestante, 2);        // Calcula a duração da fatia: mínimo entre o tempo restante e o quantum (2)


        // Calcula início, fim, espera e retorno da fatia
        int inicio = tempoAtual;
        int fim = tempoAtual + duracaoFatia;
        int espera = inicio - atual.ingresso;
        int retorno = fim - atual.ingresso;

        if (atual.tempoRestante == atual.duracao) {        // Soma espera apenas na primeira fatia da tarefa

            esperaTotal += espera;
        }

        atual.tempoRestante -= duracaoFatia;        // Executa a fatia: reduz o tempo restante


        // Imprime informações da fatia
        cout << "Tarefa " << setw(2) << atual.id
             << " | Ingresso: " << setw(2) << atual.ingresso
             << " | Duracao fatia: " << setw(2) << duracaoFatia
             << " | Inicio: " << setw(2) << inicio
             << " | Fim: " << setw(2) << fim
             << " | Espera: " << setw(2) << espera
             << " | Retorno: " << setw(2) << retorno
             << endl;

        // Avança o tempo atual para o fim da fatia
        tempoAtual = fim;

        // Se a tarefa foi concluída, soma o retorno e remove da fila de prontos
        if (atual.tempoRestante == 0) {
            retornoTotal += retorno;
            prontos.erase(prontos.begin());
        }
    }

    int n = tarefas.size();    // Calcula o número de tarefas

    cout << fixed << setprecision(2);    // Configura a saída para exibir números com 2 casas decimais

    // Exibe os tempos médios de espera e retorno
    cout << "\nTempo medio de espera: " << (esperaTotal / n)
         << "\nTempo medio de retorno: " << (retornoTotal / n)
         << endl;
}

void priop(vector<Tarefa>& tarefas)
{
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

    cout << "\nOrdem de execucao (PRIOP):\n";

    while (!pendentes.empty() || !prontos.empty()) {
        // Adiciona tarefas que chegaram ao tempo atual ao vetor de prontos
        while (!pendentes.empty() && pendentes.front().ingresso <= tempoAtual) {
            prontos.push_back(pendentes.front());
            pendentes.erase(pendentes.begin());
        }

        // Se não houver tarefas prontas, avança para a próxima chegada
        if (prontos.empty()) {
            if (!pendentes.empty()) {
                tempoAtual = pendentes.front().ingresso;
            }
            continue;
        }

        // Ordena as tarefas prontas pela maior prioridade (menor valor de prioridade, desempate por ingresso)
        sort(prontos.begin(), prontos.end(), [](const Tarefa& a, const Tarefa& b) {
            if (a.prioridade != b.prioridade) {
                return a.prioridade < b.prioridade;
            }
            return a.ingresso < b.ingresso;
        });

        // Seleciona a tarefa atual (a com maior prioridade)
        Tarefa& atual = prontos.front();

        // Calcula a duração da fatia: mínimo entre tempo restante, quantum (2) e próxima chegada
        int duracaoFatia = atual.tempoRestante;
        if (!pendentes.empty()) {
            duracaoFatia = min(duracaoFatia, pendentes.front().ingresso - tempoAtual);
        }
        duracaoFatia = min(duracaoFatia, 2); // Limita ao quantum de 2 tempos

        // Calcula início, fim, espera e retorno da fatia
        int inicio = tempoAtual;
        int fim = tempoAtual + duracaoFatia;
        int espera = inicio - atual.ingresso;
        int retorno = fim - atual.ingresso;

        // Soma espera apenas na primeira fatia da tarefa
        if (atual.tempoRestante == atual.duracao) {
            esperaTotal += espera;
        }

        // Executa a fatia: reduz o tempo restante
        atual.tempoRestante -= duracaoFatia;

        // Imprime informações da fatia
        cout << "Tarefa " << setw(2) << atual.id
             << " | Ingresso: " << setw(2) << atual.ingresso
             << " | Duracao fatia: " << setw(2) << duracaoFatia
             << " | Inicio: " << setw(2) << inicio
             << " | Fim: " << setw(2) << fim
             << " | Espera: " << setw(2) << espera
             << " | Retorno: " << setw(2) << retorno
             << endl;

        // Avança o tempo atual para o fim da fatia
        tempoAtual = fim;

        // Se a tarefa foi concluída, soma o retorno e remove da fila de prontos
        if (atual.tempoRestante == 0) {
            retornoTotal += retorno;
            prontos.erase(prontos.begin());
        }
    }

    // Calcula o número de tarefas
    int n = tarefas.size();
    // Configura a saída para exibir números com 2 casas decimais
    cout << fixed << setprecision(2);
    // Exibe os tempos médios de espera e retorno
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
            std::getline(ss, algoritmo, ';'); // Lê nome do algoritmo
            std::getline(ss, campo, ';');
            quantum = std::stoi(campo);       // Converte quantum para inteiro
            primeiraLinha = false;
        } else {
            Tarefa t;
            std::getline(ss, campo, ';'); t.id = std::stoi(campo);
            std::getline(ss, t.cor, ';');
            std::getline(ss, campo, ';'); t.ingresso = std::stoi(campo);
            std::getline(ss, campo, ';'); t.duracao = std::stoi(campo);
            std::getline(ss, campo, ';'); t.prioridade = std::stoi(campo);
            std::getline(ss, campo, ';'); // Lista de eventos

            // Divide lista de eventos por vírgula
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