#include <iostream>
#include <fstream>
#include <pthread.h>
#include "countmin.h"

#define WIDTH 512 //POTENCIA DE 2  //tam do filtro ideal para a base de 500Mb 32/524288 (experimental)
#define DEPTH 8 //POTENCIA DE 2
#define NOMEARQUIVO "entrada_teste.txt"

#define BUFFER_SIZE 100 //(experimental) 1% da base
#define NUMTHREADS 2 /*NUM DE THREADS NA MAIN SERÁ SEMRE 2. UMA PRODUTORA (INSERE NO BUFFER)
                       E OUTRA CONSUMIDORA (INSERE NOS SKETCHES E RETIRA DO BUFFER). A THREAD CONSUMIDORA
                       CRIARÁ OUTRAS THREADS PARA PARALELIZAR O TRABALHO DELA.*/

using namespace std;


int main(int argc, char *argv[])
{
    string palavra;
    int resultado;
    list <Buffertype> buffer;
    
    atomic_int buffer_current_size; //variável de controle do tamanho atual do buffer
    buffer_current_size.store(BUFFER_SIZE);
    
    ifstream arquivo(NOMEARQUIVO);
    if(!arquivo.is_open())
    {
        cout << "ERRO NO ARQUIVO!";
        return 0;
    }
    
    cout << "Inicializando..." << endl;
    inicializa(DEPTH,WIDTH);
    
    buffer.clear();
    startBuffer(&arquivo,&buffer,BUFFER_SIZE);
    
    //FIM DA PARTE SERIAL
    cout << "Memoria alocada, fazendo hashes..." << endl;
    
    //PARTE PARALELA E CONTROLE TEMPORAL
    pthread_t threads[NUMTHREADS];
    pthread_attr_t attr;

    update_t updateargs;
    feedBuffer_t feedBufferargs;
    
    updateargs.buffer=&buffer;
    updateargs.index=0;
    updateargs.buffer_current_size=&buffer_current_size;
    
    feedBufferargs.buffer=&buffer;
    feedBufferargs.file=&arquivo;
    feedBufferargs.buffer_current_size=&buffer_current_size;
    feedBufferargs.buffer_size=BUFFER_SIZE;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    pthread_create(&threads[0],&attr,feedBuffer,(void *) &feedBufferargs); //produtora
    pthread_create(&threads[1],&attr,update,(void *) &updateargs); //consumidor
    
    
    pthread_join(threads[1],NULL); //UPDATE DEPENDE DE COND_WAIT, PORTANTO TEM DE ACABAR PRIMEIRO
    pthread_join(threads[0],NULL);
    
    buffer.clear();
    //printFilter(0);
    
    

    cout << "Informe uma letra pra continuar: ";

    getchar(); //pausa

    do
    {
        cout << "Informe palavras para estimar sua frequência:" << endl;
        cin >> palavra;
        resultado=estimate(palavra,0);
        cout << "A palavra " << palavra << " apareceu " << resultado << " vezes!" << endl;
    }
    while(palavra!="exit");

}
