#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;

//nr random la adaugare/vanzare produse
//random la ok
//numarare threaduri care cumpara, vand si verificare daca se mai pot vinde in continuare

int nrProducts = 50;
int nr_thrs_sell_done = 0;
int nr_thrs_add_done = 0;
int nr_thrs;
int nr_thrs_sell, nr_thrs_add;

void sellProduct(int products, int *tid)
{
    pthread_mutex_lock(&lock);
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    t.tv_sec += 5;

    printf("Thread-ul %d vrea vanzarea a %d produse, mai sunt %d threaduri care adauga\n", *tid, products, nr_thrs_add-nr_thrs_add_done);
    int ret = pthread_cond_timedwait(&full,&lock, &t);                  //se va astepta pana conditia full e deblocata sau pana cand expira timpul t +  mutexul lock va fi deblocat
    if (ret ==  ETIMEDOUT && nrProducts<products)                       //ret a returnat ETTIMEDOUT => timpul dat a expirat si nu mai sunt produse suficiente pt a se face o vanzare
    {
        printf("Nu s-au putut vinde cele %d produse deoarece nu mai sunt suficiente produse in stoc si nici threaduri care adauga produse\n", products);
        pthread_mutex_unlock(&lock);
        return;                                                         //pentru a iesi in punctul acesta din program
    }
    nrProducts -= products;                                             //se face vanzare
    printf("--->Threadul %d a vandut %d produse\n", *tid, products);
    printf("\tAu ramas %d produse\n", nrProducts);
    nr_thrs_sell_done++;

    pthread_mutex_unlock(&lock);
}

void addProduct(int products, int *tid)
{
    pthread_mutex_lock(&lock);

    nrProducts += products;
    printf("--->Threadul %d a adaugat %d produse\n", *tid, products);
    printf("\tAvem %d produse\n", nrProducts);
    nr_thrs_add_done++;

    pthread_cond_signal(&full);                                            //trimite semnalul catre conditia full ca exista elemente in stoc, deci se pot vinde
    pthread_mutex_unlock(&lock);

}

void *functhr1(void *arg)
{
	int *tid = (int *)arg;
    int *random_number = (int*)malloc(1*sizeof(int));
    srand(random_number);                                      //nr random din zona aceea de memorie? pt a fi diferit de fiecare data deoarece pointerii sunt distincti
	*random_number = (rand()%5 + 1)*10;                        //10,20,30,40,50
	sellProduct(*random_number, tid);
	free(tid);
    return NULL;
}

void *functhr2(void *arg)
{
	int *tid = (int *)arg;
	int *random_number = (int*)malloc(1*sizeof(int));
	srand(random_number);                                       //nr random din zona aceea de memorie? pt a fi diferit de fiecare data deoarece pointerii sunt distincti
	*random_number = (rand()%5 + 1)*10;                         //10,20,30,40,50
	addProduct(*random_number, tid);
	free(tid);
    return NULL;
}


int main(int argc, char *argv[])
{

    srand(time(NULL));

    printf("Dati numarul de thread-uri \n");
	scanf("%d", &nr_thrs);

    pthread_t th[100];
	int i;

	if(pthread_mutex_init(&lock, NULL))     //returneaza 0, initializeaza mutex                //initializarea threadurilor
    {
		perror(NULL);
		return errno;
	}

    nr_thrs_sell = rand()%nr_thrs;                          //un nr random de threaduri care sa faca vanzari
    nr_thrs_add = nr_thrs - nr_thrs_sell;
    printf("Nr threaduri care vand = %d\n", nr_thrs_sell);
    printf("Nr threaduri care adauga = %d\n", nr_thrs_add);

	for(i = 0; i < nr_thrs_sell; i++)
    {
		int *k = (int *)malloc(1*sizeof(int));              //se aloca memorie pt un int de dimensiune 1 ca sa fie pointer
		*k = i;
        if(pthread_create(&th[i], NULL, functhr1, k))       //se apeleaza rutina functhr1 unde este SELL thi creaza thread ul, null atributul implicit, k = argumentu rutinei
        {
            perror(NULL);
            return errno;
        }
	}

	for(i = nr_thrs_sell; i < nr_thrs; i++)
    {
		int *k = (int *)malloc(1*sizeof(int));              //se aloca memorie pt un int de dimensiune 1
		*k = i;
		if(pthread_create(&th[i], NULL, functhr2, k))       //se apeleaza rutina functhr2 unde este ADD
        {
			perror(NULL);
			return errno;
		}
	}

    for(i = 0; i < nr_thrs ; i++)
    {
        if(pthread_join(th[i],NULL))                    //asteapta finalizarea executiei unui thread,
        {
            perror(NULL);
            return errno;
        }
    }

	printf("\nAu terminat %d threaduri\n", nr_thrs_sell_done + nr_thrs_add_done);       //nr threadurilor care au reusit sa finalizeze ce aveau de facut
	printf("Sold final: %d\n", nrProducts);

    return 0;

}
