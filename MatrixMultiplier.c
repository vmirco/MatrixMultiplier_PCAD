#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

pthread_barrier_t barriera;

//Struttura base matrice
struct matrix {
    double* next; //Puntatore elementi matrice
    int row;
    int col;
};
typedef struct matrix* matrice;

//Struct argomenti da passare per la funzione product_matrix
struct args {
    struct matrix A;
    struct matrix B;
    struct matrix R;
    struct matrix C;
    struct matrix FINAL;
    int thread; //Numero di thread
    int block; //Numero di blocchi
};
typedef struct args* argomenti;


//Dichiarazione funzioni utili per manipolare le matrici
void create_matrix (matrice m, int rows, int cols); //Inizializza matrice
void randomize_matrix (matrice m); //Popola casualmente la matrice
void print_m(matrice m);
void* product_matrix(void* args);

int main(int argc, char** argv) {
    srand(time(NULL)); 
    int threads, block_size, block;

    if (argc != 3){
        printf( "ERROR: ./... [block] [block_size]\n");
        exit(1);
    }
    block_size = atoi(argv[1]);
    block = atoi(argv[2]);
    if (block > block_size){
        printf( "ERROR: block > block_size\n");
        exit(1);
    }
    if (block_size % block == 0) 
        threads = block_size / block;
    else    
        threads = block_size / block + 1;

    int M = block_size;
    int N = block_size / 2;
    int P = M;

    //MATRICI INIZIALI
    struct matrix m_A;
    struct matrix m_B;
    create_matrix(&m_A, M, N);
    randomize_matrix(&m_A);
    create_matrix(&m_B, N, P);
    randomize_matrix(&m_B);

    //MATRICE RISULTATO INTERMEDIA
    struct matrix m_R;
    create_matrix(&m_R, M, P);

    //TERZA MATRICE DA MOLTIPLICARE
    struct matrix m_C;
    create_matrix(&m_C, P, M);
    randomize_matrix(&m_C); 

    //MATRICE FINALE
    struct matrix m_FINAL;
    create_matrix(&m_FINAL, P, P);

    pthread_t* thread_ptr = (pthread_t*)malloc(threads * sizeof(pthread_t)); //"Array" per i thread
    struct args* f_arguments = (struct args*)malloc(threads * sizeof(struct args)); //"Array" per gli argomenti da passare alla funzione matrix_product
    pthread_barrier_init(&barriera, NULL, threads);

    clock_t start = clock(); //PRENDIAMO IL TEMPO

    //CREO E SETTO OGNI THREAD
    for (int i = 0; i < threads; i++){
        f_arguments[i].A = m_A;
        f_arguments[i].B = m_B;
        f_arguments[i].R = m_R;
        f_arguments[i].C = m_C;
        f_arguments[i].FINAL = m_FINAL;
        f_arguments[i].thread = i; //Numero identificativo per il thread
        f_arguments[i].block = block;

        if (pthread_create(&thread_ptr[i], NULL, product_matrix, &f_arguments[i]) != 0){
            printf("ERROR CREATING THREAD");
            exit(-1);
        }
    }

    //SINCRONIZZO I THREAD
    void* code;
    for (int i = 0; i < threads; ++i){
        if (pthread_join(thread_ptr[i], &code) != 0){
            printf("ERROR WITH PTHREAD_JOIN");
            exit(-1);
        }
    }


    free(m_A.next);
    free(m_B.next);
    free(m_R.next);
    free(m_C.next);
    free(m_FINAL.next);
    free(thread_ptr);
    free(f_arguments);
    pthread_barrier_destroy(&barriera);

    printf("NUMERO DI THREAD: %i - TEMPO IMPIEGATO: %f secondi\n", threads, ((double)(clock() - start) / CLOCKS_PER_SEC));
}

//FUNZIONI GESTIONE MATRICI
void create_matrix(matrice m, int rows, int cols) {
    m->next = (double*)malloc(rows * cols * sizeof(double)); //Alloco spazio

    if (m->next == NULL) {
        printf("create_matrix: MALLOC ERROR");
        exit(-1);
    }
    m->row = rows;
    m->col = cols;

    for (int i = 0; i < m->row * m->col; ++i) m->next[i] = 0; //Inizializzo a 0
}


void randomize_matrix(matrice m) {
    for (int i = 0; i < m->row * m->col; ++i)
        m->next[i] = (float)(rand() % 100) / 10; //Inserisco numero random da 1 a 99, poi fratto 10
}

void print_m(matrice m) {
    for (int y = 0; y < m->row; ++y) {
        for (int x = 0; x < m->col; ++x)
            printf(" %f ", m->next[y + x]);
        printf("\n");
    }
     printf("\n");
}

void* product_matrix(void* args) {
    argomenti arguments = args;
    double value;
    int i, j, n;

    for (i = arguments->thread * arguments->block; i < ((arguments->thread * arguments->block) + arguments->block) && i < (arguments->R.row); i++) {
        for (j = 0; j < arguments->R.col; j++) {
            value = 0;
            for (n = 0; n < arguments->A.col; n++)
                value += arguments->A.next[j * arguments->A.col + n] * arguments->B.next[n * arguments->B.col + i];
             arguments->R.next[j * arguments->R.col + i] = value;
        }
    }

    //Aspetto che tutti i thread abbiano concluso i loro calcoli poichè altrimenti avrei un R incompleta
    pthread_barrier_wait(&barriera);

    for (i = arguments->thread * arguments->block; i < ((arguments->thread * arguments->block) + arguments->block) && i < (arguments->FINAL.row); i++) {
        for (j = 0; j < arguments->FINAL.col; j++) {
            value = 0;
            for (n = 0; n < arguments->R.col; n++)
                value += arguments->R.next[j * arguments->R.col + n] * arguments->C.next[n * arguments->C.col + i];
             arguments->FINAL.next[j * arguments->FINAL.col + i] = value;
        }
    }

    //Aspetto di avere la matrice finale completa
    pthread_barrier_wait(&barriera);
}