// Para compilar o código C++ com suporte a threads, você precisa usar: 
// g++ -std=c++11 -pthread -o main main.cpp
// Para executar com argumentos:
// ./main numBuckets numThreads

#include <iostream>
#include <fstream>
#include <thread>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <climits>
#include <string>
#include <map>

using namespace std;

// Função para ler números de um arquivo
void lerArquivo(const string& nomeArquivo, vector<int>& lista) {
    ifstream file(nomeArquivo);
    int num;
    lista.clear();
    if (!file.is_open()) {
        cerr << "Erro ao abrir o arquivo: " << nomeArquivo << endl;
        return;
    }
    while (file >> num) {
        lista.push_back(num);
    }
    file.close();
}

void salvarTempos(int numElementos, double tempo, const string& metodo, const string& multithread, int numBuckets, int numThreads, const string& tipoEntrada, int execucao) {
    // Nome do arquivo de saída: output_dir_entrada.csv
    FILE *f = fopen(("output_" + tipoEntrada + ".csv").c_str(), "a");
    if (f != NULL) {
        fprintf(f, "%d,%.8f,%s,%s,%d,%d,%d\n", numElementos, tempo, metodo.c_str(), multithread.c_str(), numBuckets, numThreads, execucao);
        fclose(f);
    }
}

/*
void gerarNumerosAleatorios(int qntElementos, vector<int>& lista) {
    srand(time(0));
    int menorValor = INT_MAX;
    for(int i=0; i<qntElementos; i++){
        lista.push_back(rand () % 1000000 + 1);
        // Atualizar o menor valor encontrado
        if(lista[i] < menorValor) {
            menorValor = lista[i];
        }
    }
    // Salvar os números gerados em um arquivo
    FILE *file = fopen("numeros.txt", "w");
    if (file != NULL) {
        for (const auto& num : lista) {
            fprintf(file, "%d\n", num);
        }
        fclose(file);
    }
    cout << "Menor valor gerado: " << menorValor << endl;
}
*/

void insertionSort(vector<int>& lista, int n, const string& tipoEntrada, int execucao) {
    clock_t inicio = clock();
    int aux = lista.size();
    for(int i = 1; i < aux; i++){
        int chave = lista[i];
        int j = i - 1;
        while(j>=0 && lista[j] > chave){
            lista[j + 1] = lista[j];
            j--;
        }
        lista[j + 1] = chave;
    }  
    clock_t fim = clock();
    double tempo = double(fim - inicio) / CLOCKS_PER_SEC;
    cout << "Insertion Sort concluído em " << tempo << " segundos." << endl;
    salvarTempos(n, tempo, "Insertion Sort", "Nao", 0, 1, tipoEntrada, execucao);
}

void insertionSortBucket(vector<int>& lista) {
    int aux = lista.size();
    for(int i = 1; i < aux; i++){
        int chave = lista[i];
        int j = i - 1;
        while(j>=0 && lista[j] > chave){
            lista[j + 1] = lista[j];
            j--;
        }
        lista[j + 1] = chave;
    } 
}

void insertionSortRange(vector<int>& lista, int start, int end) {
    for(int i = start + 1; i < end; i++){
        int chave = lista[i];
        int j = i - 1;
        while(j >= start && lista[j] > chave){
            lista[j + 1] = lista[j];
            j--;
        }
        lista[j + 1] = chave;
    }
}

// Função auxiliar para fazer merge de dois ranges ordenados
void mergeRanges(vector<int>& lista, int left, int mid, int right) {
    vector<int> temp(right - left);
    int i = left, j = mid, k = 0;
    // Enquanto houver elementos em ambos os ranges
    while(i < mid && j < right) {
        // Comparar os elementos e adicionar o menor ao vetor temporário
        // Se o elemento do primeiro range for menor ou igual ao do segundo
        if(lista[i] <= lista[j]) {
            // Adicionar o elemento do primeiro range
            temp[k++] = lista[i++];
        } else {
            // Caso contrário, adicionar o elemento do segundo range
            temp[k++] = lista[j++];
        }
    }
    
    // Adicionar os elementos restantes do primeiro range, se houver
    while(i < mid) temp[k++] = lista[i++];
    // Adicionar os elementos restantes do segundo range, se houver
    while(j < right) temp[k++] = lista[j++];
    
    // Copiar os elementos do vetor temporário de volta para o vetor original
    for(int i = 0; i < k; i++) {
        lista[left + i] = temp[i];
    }
}

void insertionSortMultithread(vector<int>& lista, int numThreads, int n, const string& tipoEntrada, int execucao) {
    clock_t inicio = clock();
    int size = lista.size();
    int chunkSize = size / numThreads;
    vector<thread> threads;
    
    // Criar threads para ordenar cada chunk
    for(int i = 0; i < numThreads; i++) {
        int start = i * chunkSize;
        int end = (i == numThreads - 1) ? size : (i + 1) * chunkSize;
        threads.push_back(thread(insertionSortRange, ref(lista), start, end));
    }
    
    // Aguardar todas as threads terminarem
    for(auto& t : threads) {
        t.join();
    }
    
    // Merge dos chunks ordenados 
    for(int gap = chunkSize; gap < size; gap *= 2) {
        for(int i = 0; i < size; i += gap * 2) {
            int mid = min(i + gap, size);
            int end = min(i + gap * 2, size);
            if(mid < end) {
                mergeRanges(lista, i, mid, end);
            }
        }
    }
    clock_t fim = clock();
    double tempo = double(fim - inicio) / CLOCKS_PER_SEC;
    cout << "Insertion Sort multithread concluído em " << tempo << " segundos." << endl;
    salvarTempos(n, tempo, "Insertion Sort", "Sim", 0, numThreads, tipoEntrada, execucao);
}

void separarBuckets(const vector<int>& lista, vector<vector<int>>& buckets, int numBuckets) {
    if (lista.empty()) return;
    
    int maxValue = *max_element(lista.begin(), lista.end());
    int minValue = *min_element(lista.begin(), lista.end());
    int range = maxValue - minValue + 1;
    
    // Se há mais buckets que elementos únicos possíveis, ajustar numBuckets
    int effectiveBuckets = min(numBuckets, range);
    int bucketSize = max(1, range / effectiveBuckets);

    for(int num : lista) {
        int bucketIndex = (num - minValue) / bucketSize;
        if(bucketIndex >= effectiveBuckets) bucketIndex = effectiveBuckets - 1;
        buckets[bucketIndex].push_back(num);
    }
}

void bucketSort(vector<int>& lista, int numBuckets, int n, const string& tipoEntrada, int execucao) {
    clock_t inicio = clock();
    vector<vector<int>> buckets(numBuckets);
    
    // Separar os números em buckets
    separarBuckets(lista, buckets, numBuckets);
    
    // Ordenar cada bucket usando insertion sort
    for(auto& bucket : buckets) {
        insertionSortBucket(bucket);
    }
    
    // Concatenar os buckets de volta na lista original
    lista.clear();
    for(const auto& bucket : buckets) {
        lista.insert(lista.end(), bucket.begin(), bucket.end());
    }
    clock_t fim = clock();
    double tempo = double(fim - inicio) / CLOCKS_PER_SEC;
    cout << "Bucket Sort concluído em " << tempo << " segundos." << endl;
    salvarTempos(n, tempo, "BucketSort", "Nao", numBuckets, 1, tipoEntrada, execucao);
}

void bucketSortMultithread(vector<int>& lista, int numBuckets, int numThreads, int n, const string& tipoEntrada, int execucao) {
    clock_t inicio = clock();
    vector<vector<int>> buckets(numBuckets);
    
    // Separar os números em buckets
    separarBuckets(lista, buckets, numBuckets);
    
    vector<thread> threads;
    
    // Limitar o número de threads ao valor especificado
    int threadsToUse = min(numThreads, numBuckets);
    int bucketsPerThread = numBuckets / threadsToUse;
    
    // Ordenar buckets usando o número limitado de threads
    for(int t = 0; t < threadsToUse; t++) {
        int startBucket = t * bucketsPerThread;
        int endBucket = (t == threadsToUse - 1) ? numBuckets : (t + 1) * bucketsPerThread;
        
        threads.push_back(thread([&buckets, startBucket, endBucket, n, tipoEntrada]() {
            for(int i = startBucket; i < endBucket; i++) {
                insertionSortBucket(buckets[i]);
            }
        }));
    }
    
    // Aguardar todas as threads terminarem
    for(auto& t : threads) {
        t.join();
    }
    
    // Concatenar os buckets de volta na lista original
    lista.clear();
    for(const auto& bucket : buckets) {
        lista.insert(lista.end(), bucket.begin(), bucket.end());
    }
    clock_t fim = clock();
    double tempo = double(fim - inicio) / CLOCKS_PER_SEC;
    cout << "Bucket Sort multithread concluído em " << tempo << " segundos." << endl;
    salvarTempos(n, tempo, "BucketSort", "Sim", numBuckets, numThreads, tipoEntrada, execucao);
}

int main(int argc, char* argv[]) {
    // Corrigir os caminhos para a estrutura de diretórios correta
    map<string, string> dirs_entrada = {
        {"Aleatorios", "../Arquivos/input/Aleatorios"},
        {"Ordenados", "../Arquivos/input/Ordenados"},
        {"Reversos", "../Arquivos/input/Decrescentes"},
        {"Parcialmente_Ordenados", "../../Arquivos/input/ParcialmenteOrdenados"}
    };
    vector<int> num_elementos = {100,200,500,1000,2000,5000,7500,10000,15000,30000,50000,75000,100000,200000,500000,750000,1000000,1250000,1500000,2000000};

    int numBuckets;
    int numThreads;
    int maxThreads = thread::hardware_concurrency();
    cout << "Número máximo de threads disponíveis: " << maxThreads << endl;

    if (argc > 1) {
        numThreads = atoi(argv[1]);
        if (numThreads <= 0 || numThreads > maxThreads) {
            cout << "Número de threads inválido. Usando valor padrão: 4" << endl;
            numThreads = 4;
        }
    }
    else {
        numThreads = 8; // Valor padrão
    }

    cout << "Número de threads a serem usadas: " << numThreads << endl;
    
    // Verificar se o diretório de entrada existe
    cout << "Verificando estrutura de diretórios..." << endl;
    int execucao;
    for (execucao = 1; execucao <= 5; execucao++) {
        for (const auto& tipo : dirs_entrada) {
            for (const auto& n : num_elementos) {
                    // Montar nome do arquivo conforme padrão Python
                string prefixo = tipo.first.substr(0,1);
                transform(prefixo.begin(), prefixo.end(), prefixo.begin(), ::tolower);
                string nomeArquivo = tipo.second + "/" + prefixo + to_string(n) + ".txt";

                // Debug: mostrar o caminho que está sendo procurado
                cout << "Tentando ler arquivo: " << nomeArquivo << endl;

                vector<int> dadosOriginais;
                lerArquivo(nomeArquivo, dadosOriginais);
                if (dadosOriginais.empty()) {
                    cout << "Arquivo vazio ou não encontrado: " << nomeArquivo << endl;
                    continue;
                }
                cout << "\n=== Executando métodos para " << tipo.first << " " << n << " ===" << endl;

                // Insertion Sort sequencial
                vector<int> listaSequencial = dadosOriginais;
                cout << "\n=== Executando Insertion Sort sequencial ===" << endl;
                insertionSort(listaSequencial, n, tipo.first, execucao);

                // Insertion Sort multithread
                vector<int> listaMultithread = dadosOriginais;
                cout << "\n=== Executando Insertion Sort multithread ===" << endl;
                insertionSortMultithread(listaMultithread, numThreads, n, tipo.first, execucao);


                // Bucket Sort sequencial
                vector<int> listaBucket = dadosOriginais;
                cout << "\n=== Executando Bucket Sort sequencial ===" << endl;
                cout << "Número de buckets: " << numBuckets << endl;
                bucketSort(listaBucket, numBuckets, n, tipo.first, execucao);

                // Bucket Sort multithread
                vector<int> listaBucketMultithread = dadosOriginais;
                cout << "\n=== Executando Bucket Sort multithread ===" << endl;
                cout << "Número de buckets: " << numBuckets << endl;
                bucketSortMultithread(listaBucketMultithread, numBuckets, numThreads, n, tipo.first, execucao);
            }
        }
    }
    cout << "\nTodos os métodos de ordenação foram executados com sucesso." << endl;
    return 0;
}
