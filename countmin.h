#include <iostream>
#include <atomic>
#include <deque>
#include <list>
#include <fstream>

using namespace std;

struct CMSketch
{
    int depth;
    int width;
    int time_length;
    atomic_int ***filter;
};

struct Hokusai
{
    int numOfSketches;
    deque <CMSketch*> CMS;
};

struct Buffertype
{
    string palavra;
    unsigned short cnt;
};

struct hashParallel_t
{
    list<Buffertype> *buffer;
    int pos;
    int index;
};

struct update_t
{
    list<Buffertype> *buffer; 
    int index;
};

struct sumTime_t
{
    int index1,index2,pos;
};

struct sumItem_t
{
    int index,pos;
};

struct feedBuffer_t
{
    ifstream *file;
    list <Buffertype> *buffer;
};

int newFilter(int profundidade, int largura,int insert_pos);
void deleteFilter(int index);
void copyFilter(int index1,int index2);
void printFilter(int index);

void inicializa(int altura, int profundidade);

void *update(void *threadupdate);
int estimate(string palavra,int index);

int hashSerial(string palavra, int a, int b,int index); //serial
void *hashParallel(void *threadhash); //paralelo



void timeAgreggation(int index1, int index2);
void *sumTime(void *threadsum);

void itemAgregation(int index);
void *sumItem(void *threadsum);

void *feedBuffer(void *threadbuffer);
void startBuffer(ifstream *file, list<Buffertype> *buffer, int buffer_size);

