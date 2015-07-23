#include <iostream>
#include <atomic>
#include <deque>

using namespace std;

typedef struct CMSketch
{
    int depth;
    int width;
    int time_length;
    atomic_int ***filter;
} CMSketch;

typedef struct Hokusai
{
    int numOfSketches;
    deque <CMSketch*> CMS;
} Hokusai;

typedef struct
{
    string palavra;
    int pos;
    int index;
} hashParallel_t;

typedef struct
{
    string palavra;
    int index;
} update_t;

typedef struct
{
    int index1,index2,pos;
} sumTime_t;

typedef struct
{
    int index,pos;
} sumItem_t;

int newFilter(int profundidade, int largura,int insert_pos);
void deleteFilter(int index);
void copyFilter(int index1,int index2);
void printFilter(int index);

void inicializa(int altura, int profundidade);

void update(string palavra,int index);
int estimate(string palavra,int index);

int hashSerial(string palavra, int a, int b,int index); //serial
void *hashParallel(void *threadhash); //paralelo



void timeAgreggation(int index1, int index2);
void *sumTime(void *threadsum);

void itemAgregation(int index);
void *sumItem(void *threadsum);

