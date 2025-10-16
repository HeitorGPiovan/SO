#include "func.h"   
#include <algorithm> 
#include <iomanip>   

int quantum;
std::string algoritmo;

void fifo(vector<Tarefa>& tarefas)
{
    cout << "Executando FIFO com quantum 2..." << endl;

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
    cout << "Executando SRTF..." << endl;
}

void priop(vector<Tarefa>& tarefas)
{
    cout << "Executando PRIOP..." << endl;
}