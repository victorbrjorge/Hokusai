#include <pthread.h>
#include "countmin.h"
#include <string.h>
#include <time.h>
#include <climits>

#define NUMTHREADS 4 //NÚMERO DE WORKER THREADS (sempre múltiplo de 2)

Hokusai sketches;
int **hash_parameter; //Guarda os parametros A e B das hashes, de acordo com o paper do count-min sketch

//DECLARAÇÃO E INICIALIZAÇÃO DOS LOCKS

//pthread_mutex_t lock_cnt = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_buffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t can_produce = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_consume = PTHREAD_COND_INITIALIZER;

int newFilter(int profundidade, int largura,int insert_pos)
{
    int index = sketches.numOfSketches;
    CMSketch *new_sketch; 

    new_sketch = new CMSketch;
    
    new_sketch->width = largura;
    new_sketch->depth = profundidade/NUMTHREADS;

    new_sketch->filter = new atomic_int**[NUMTHREADS];
    for(int i=0; i<NUMTHREADS; i++)
    {
        new_sketch->filter[i] = new atomic_int*[profundidade];
    }
    for(int i=0; i<NUMTHREADS; i++)
    {
        for(int j=0; j<profundidade; j++)
        {
            new_sketch->filter[i][j] = new atomic_int[largura];
        }
    }

    for(int i=0; i<NUMTHREADS; i++)
    {
        for(int j=0; j<profundidade; j++)
        {
            for(int k=0; k<largura; k++)
            {
                new_sketch->filter[i][j][k]=0;
            }
        }
    }
    sketches.CMS.insert(sketches.CMS.begin()+insert_pos,new_sketch);
    sketches.numOfSketches++;
    return index;
}

void deleteFilter(int index)
{
    CMSketch *deletion_filter;
    
    deletion_filter = sketches.CMS.at(index);
    
    for (int i = 0; i < deletion_filter->depth; i++)
    {
        for (int j = 0; j < deletion_filter->width; j++)
        {
            delete[] deletion_filter->filter[i][j];
        }
        delete[] deletion_filter->filter[i];
    }
    delete[] deletion_filter->filter;
    sketches.CMS.erase(sketches.CMS.begin()+index);

    sketches.numOfSketches--;
}

void printFilter(int index)
{

    for(int i=0; i<NUMTHREADS; i++)
    {
        for(int j=0; j<sketches.CMS[index]->depth; j++)
        {
            for(int k=0; k<sketches.CMS[index]->width; k++)
            {
                cout << sketches.CMS[index]->filter[i][j][k] <<" ";
            }
            cout << endl;
        }
        cout << endl;
    }
    cout << "IMPRESSAO FEITA" << endl;
}

void copyFilter(int index1, int index2) //copia o filtro 2 para o filtro 1, ignora tamanho
{
    for(int i=0; i<NUMTHREADS; i++)
    {
        for(int j=0; j<sketches.CMS[index1]->depth; j++)
        {
            for (int k=0; k<sketches.CMS[index1]->width; k++)
            {
                sketches.CMS[index1]->filter[i][j][k].store(sketches.CMS[index2]->filter[i][j][k],memory_order_relaxed);
            }
        }
    }
}

void inicializa(int profundidade, int largura) //aloca memória, gera os valores aleatório para as hashes
{
    hash_parameter = new int*[profundidade];
    for(int i=0; i<profundidade; i++)
    {
        hash_parameter[i]= new int[2];
    }

    srand(time(NULL));
    for(int m=0; m<profundidade; m++)
    {
        for(int n=0; n<2; n++)
        {
            hash_parameter[m][n]= rand() % 2147483647; //2147483647 é um num primo = 2³¹-1
        }
    } //SELECIONA O A E B DE CADA LINHA PARA A FUNÇÃO HASH

    sketches.numOfSketches=0;
    newFilter(profundidade,largura,0);
}

void *update(void *threadupdate) //thread consumidora, chama suas workers e controla quando apagar dados do buffer
{   
    update_t *args;
    args = (update_t *) threadupdate;
    
    atomic_int *buffer_current_size;
    list <Buffertype> *buffer;
    list<Buffertype>::iterator it;
    
    buffer_current_size=args->buffer_current_size;
    buffer=args->buffer;
    
    pthread_t threads[NUMTHREADS];
    pthread_attr_t attr;

    hashParallel_t hashargs [NUMTHREADS];

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    for(int j=0; j<NUMTHREADS; j++)
    {   
        hashargs[j].pos=j;
        hashargs[j].buffer=buffer;
        hashargs[j].index=args->index;
        hashargs[j].buffer_current_size=buffer_current_size;

        pthread_create(&threads[j],&attr,hashParallel,(void *) &hashargs[j]);
    }
    
    //REGIÃO DE CONTROLE DO BUFFER. RETIRA A PRIMEIRA POS DO BUFFER CASO TODAS AS THREADS TENHAM A LIDO.
    pthread_mutex_lock(&lock_buffer);
    while(*buffer_current_size>-1){
        pthread_mutex_unlock(&lock_buffer);
        
        pthread_mutex_lock(&lock_buffer);
        if(*buffer_current_size==0)
            continue;
        
        it=buffer->begin();
        if(it->cnt == NUMTHREADS){
            args->buffer->pop_front();
            (*buffer_current_size)--;
                
            cout << "Tam buffer(update): " << *buffer_current_size << endl;
            pthread_cond_signal(&can_produce);
        }
    }
    pthread_mutex_unlock(&lock_buffer);
    for(int i=0; i<NUMTHREADS; i++)
    {
        pthread_join(threads[i],NULL);
    }
    cout << "PALAVRAS INSERIDAS" << endl;
    pthread_exit(NULL);
}

int estimate(string palavra,int index)
{
    int hash_keys[sketches.CMS[index]->depth*NUMTHREADS];

    for(int j=0; j<sketches.CMS[index]->depth*NUMTHREADS; j++)
    {
        hash_keys[j]=hashSerial(palavra,hash_parameter[j][0],hash_parameter[j][1],index);
    }

    int menor=INT_MAX;
    int k=0;

    for(int i=0; i<NUMTHREADS; i++)
    {
        for(int j=0; j<sketches.CMS[index]->depth; j++)
        {
            if(sketches.CMS[index]->filter[i][j][hash_keys[k]]<menor)
            {
                menor=sketches.CMS[index]->filter[i][j][hash_keys[k]];
            }
            k++;
        }
    }
    return menor;
}

void *hashParallel(void *threadhash)
{
    int pos,index;

    int a,b;
    atomic_int *buffer_current_size;
    
    hashParallel_t *args;
    args = (hashParallel_t *) threadhash;

    pos=args->pos;
    index=args->index;
    buffer_current_size=args->buffer_current_size;
    
    char ch[1000];
    
    int saida,c;

    unsigned long hash;
    
    string palavra;
    
    pthread_mutex_lock(&lock_buffer);
    list<Buffertype>::iterator it=args->buffer->begin();
    
    while(!(*buffer_current_size<=-1 && it==args->buffer->end())) {
        pthread_mutex_unlock(&lock_buffer);
        
        //REALIZA O PROCESSO DE HASHING DA PALAVRA DBJ2 + A*palavra + B mod p mod n
        palavra=it->palavra;
        strcpy(ch, palavra.c_str());
        
        hash = 5381;
        for (int i=0; i < palavra.length(); i++)
        {
            c = ch[i];
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        }

        for(int i=0; i<sketches.CMS[index]->depth; i++)
        {
            a=hash_parameter[sketches.CMS[index]->depth*pos+i][0];
            b=hash_parameter[sketches.CMS[index]->depth*pos+i][1];

            saida = ((a*hash + b) % 2147483647) % (sketches.CMS[index]->width);
            sketches.CMS[index]->filter[pos][i][saida]++; //atualiza o valor no sketch
        }
        //FIM DO PROCESSO DE HASHING
        
        pthread_mutex_lock(&lock_buffer); //lock somente para garatir integridade do valor
            (it->cnt)++;
            it++;
        pthread_mutex_unlock(&lock_buffer);
        
        pthread_mutex_lock(&mutex);
        while (*buffer_current_size==0) { //se o buffer estiver vazio, espere
            pthread_cond_wait(&can_consume, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        
        
        pthread_mutex_lock(&lock_buffer);
    }
    pthread_mutex_unlock(&lock_buffer);
    cout << "WORKER DONE" << endl;
    pthread_exit(NULL);
}

int hashSerial(string palavra, int a, int b,int index)
{
    char ch[1000];
    strcpy(ch, palavra.c_str());

    int saida,c;

    unsigned long hash = 5381;
    for (int i=0; i < palavra.length(); i++)
    {
        c = ch[i];
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    saida = ((a*hash + b) % 2147483647) % (sketches.CMS[index]->width);
    return saida;
}

void *sumTime(void *threadsum)
{
    int index1,index2,pos;

    sumTime_t *args;
    args = (sumTime_t *) threadsum;

    index1=args->index1;
    index2=args->index2;
    pos=args->pos;

    for(int i=0; i<sketches.CMS[index1]->depth; i++)
    {
        for(int j=0; j<sketches.CMS[index1]->width; j++)
        {
            sketches.CMS[index1]->filter[pos][i][j].fetch_add(sketches.CMS[index2]->filter[pos][i][j]);
        }
    }
}

void timeAgreggation(int index1, int index2)
{
    pthread_t threads[NUMTHREADS];
    pthread_attr_t attr;

    sumTime_t sumargs [NUMTHREADS];

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(int j=0; j<NUMTHREADS; j++)
    {
        sumargs[j].index1=index1;
        sumargs[j].index2=index2;
        sumargs[j].pos=j;


        pthread_create(&threads[j],&attr,sumTime,(void *) &sumargs[j]);
    }

    for(int i=0; i<NUMTHREADS; i++)
    {
        pthread_join(threads[i],NULL);
    }

    deleteFilter(index2);
}

void *sumItem(void *threadsum)
{
    int index,pos;

    sumItem_t *args;
    args = (sumItem_t *) threadsum;

    index=args->index;
    pos=args->pos;

    for(int i=0; i<sketches.CMS[index]->depth; i++)
    {
        for(int j=0; j<sketches.CMS[index]->width/2; j++)
        {
            sketches.CMS[index]->filter[pos][i][j].fetch_add(sketches.CMS[index]->filter[pos][i][sketches.CMS[index]->width-j]);
        }
    }
}

void itemAgregation(int index)
{
    int new_filter_id;
    pthread_t threads[NUMTHREADS];
    pthread_attr_t attr;

    sumItem_t sumargs [NUMTHREADS];

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(int j=0; j<NUMTHREADS; j++)
    {
        sumargs[j].index=index;
        sumargs[j].pos=j;

        pthread_create(&threads[j],&attr,sumItem,(void *) &sumargs[j]);
    }

    for(int i=0; i<NUMTHREADS; i++)
    {
        pthread_join(threads[i],NULL);
    }

    new_filter_id=newFilter(sketches.CMS[index]->depth,sketches.CMS[index]->width/2,index);
    copyFilter(new_filter_id,index);
    deleteFilter(index);
}

void startBuffer(ifstream *file, list<Buffertype> *buffer, int buffer_size){
    Buffertype temp;
    int cnt=0;
    temp.cnt = 0;
    while(cnt<buffer_size && !file->eof())
    {
        *file >> temp.palavra;
        buffer->push_back(temp);
        cnt++;
    }
    cout << "BUFFER CRIADO" << endl;
}

void *feedBuffer(void *threadbuffer){
    
    ifstream *file;
    list <Buffertype> *buffer;
    Buffertype temp;
    atomic_int *buffer_current_size;
    long unsigned int buffer_size;

    feedBuffer_t *args;
    args = (feedBuffer_t *) threadbuffer;

    buffer = args->buffer;
    file = args->file;
    buffer_current_size=args->buffer_current_size;
    buffer_size=args->buffer_size;
    
    temp.cnt = 0;
    
    while(!file->eof())
    {   
        pthread_mutex_lock(&mutex);
        
        while(*buffer_current_size == buffer_size) { // full
            // wait until some elements are consumed
            pthread_cond_wait(&can_produce, &mutex);
        }
        
        *file >> temp.palavra;
        
        pthread_mutex_lock(&lock_buffer); //LENDO DE PALAVRA EM PALAVRA (descobrir tamanho ideal)
            buffer->push_back(temp);
            (*buffer_current_size)++;
            cout << "Tam buffer(p): " << *buffer_current_size << endl;
        pthread_mutex_unlock(&lock_buffer);
        
        
        pthread_cond_broadcast(&can_consume);
        pthread_mutex_unlock(&mutex);
        
    }
    file->close();
    
    pthread_cond_broadcast(&can_consume);
    
    pthread_mutex_lock(&lock_buffer);
        buffer_current_size->store(-1); //SINALIZAR QUE A LEITURA ACABOU, EIVTANDO O LOCK
    pthread_mutex_unlock(&lock_buffer);
    cout << "FIM DA LEITURA" << endl;
    pthread_exit(NULL);
}