#include <iostream>
#include <fstream>
#include <pthread.h>
#include "countmin.h"

#define WIDTH 524288 //POTENCIA DE 2  //tam do filtro 32/524288
#define DEPTH 32 //POTENCIA DE 2
#define NOMEARQUIVO "entrada_teste.txt"

#define BUFFER_SIZE 1
#define NUMTHREADS 4

using namespace std;

int main(int argc, char *argv[])
{
    string palavra;
    int resultado;

    ifstream arquivo(NOMEARQUIVO);

    if(!arquivo)
    {
        cout << "ERRO NO ARQUIVO!";
        return 0;
    }

    cout << "Inicializando..." << endl;
    inicializa(DEPTH,WIDTH);

    cout << "Memoria alocada, fazendo hashes..." << endl;

    int cnt=0;
    while(!arquivo.eof())
    {
        arquivo >> palavra;

        update(palavra,0);
        cnt++;
        if(cnt%100000==0)
        {
            cout << cnt << " palavras lidas" << endl;
        }
    }

    //imprime(0);

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
