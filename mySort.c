#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>


struct Sort_SiralamaParametreleri {
    char **kelimeler;
    int baslangic;
    int son;
};


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


int ThreadSayisi = 0;


int ThreadDurum();
void Merge(char **kelimeler, int baslangic, int ortanca, int son);
void MergeSort(char **kelimeler, int baslangic, int son);
void *ParalelMergeSort(void *args);
void QuickSort(char **kelimeler, int baslangic, int son);
void *ParalelQuickSort(void *args);
int Bolumlendir(char **kelimeler, int baslangic, int son);
void Degistir(char **kelimeler, int i, int j);


int main(int argc, char *argv[]){

    struct timeval start, end;
    double islem_suresi;

    gettimeofday(&start, NULL);

    if (argc != 5) {
        printf("\nEksik arguman\n");
        printf("\nFormat: <GirdiDosyasi.txt> <CiktiDosyasi.txt> <Thread Sayisi> <Siralama Algoritmasi (merge, quick)>\n\n");
        return 1;
    }

    char *GirdiDosyasi = argv[1];
    char *CiktiDosyasi = argv[2];
    ThreadSayisi = atoi(argv[3]);

    if(ThreadSayisi < 0){

        printf("/nGecersiz Thread Sayisi!\n");
        return 1;
    }

    char *SiralamaAlgoritmasi = argv[4];

    printf("\nGirdi Dosyasi: %s\n", GirdiDosyasi);
    printf("Cikti Dosyasi: %s\n", CiktiDosyasi);
    printf("Thread Sayisi: %d\n", ThreadSayisi);
    printf("Siralama Algoritmasi: %s\n", SiralamaAlgoritmasi);

    FILE *girdi_dosyasi;

    girdi_dosyasi = fopen(GirdiDosyasi, "r");
    if (girdi_dosyasi == NULL) {
        printf("\nGirdi Dosyasi Acilamadi!, %s\n", girdi_dosyasi);
        return 1;
    }

    char **kelimeler = NULL;
    char kelime[100];
    int kelime_sayaci = 0;
    while (fscanf(girdi_dosyasi, "%s", kelime) != EOF) {

        kelimeler = (char **)realloc(kelimeler, (kelime_sayaci + 1) * sizeof(char *));

        if(kelimeler == NULL){

            printf("\nBellek tahsisi gerceklestirilemedi!\n");
            return 1;
        }

        kelimeler[kelime_sayaci] = strdup(kelime);
        kelime_sayaci++;
    }

    fclose(girdi_dosyasi);

    struct Sort_SiralamaParametreleri parametreler;
    parametreler.kelimeler = kelimeler;
    parametreler.baslangic = 0;
    parametreler.son = kelime_sayaci - 1;

    if(strcmp(SiralamaAlgoritmasi, "merge") == 0) {

        if(ThreadSayisi > 0){

            ParalelMergeSort(&parametreler);

        } else {

            MergeSort(parametreler.kelimeler, parametreler.baslangic, parametreler.son);
        }

    } else if (strcmp(SiralamaAlgoritmasi, "quick") == 0) {

        if(ThreadSayisi > 0){

            ParalelQuickSort(&parametreler);

        } else {

            QuickSort(parametreler.kelimeler, parametreler.baslangic, parametreler.son);
        }

    } else {

        printf("\nGecersiz algoritma. Kullanilabilecek algoritmalar: merge, quick\n\n");
        return 1;
    }

    FILE *cikti_dosyasi;
    int i;

    cikti_dosyasi = fopen(CiktiDosyasi, "w");
    if (cikti_dosyasi == NULL) {

        printf("\nCikti Dosyasi Acilamadi!, %s\n", CiktiDosyasi);
        return 1;
    }

    for (i = 0; i < kelime_sayaci; i++) {

        fprintf(cikti_dosyasi, "%s\n", kelimeler[i]);
        free(kelimeler[i]);
    }

    fclose(cikti_dosyasi);
    free(kelimeler);

    printf("\nSiralama Tamamlandi.\n");


    gettimeofday(&end, NULL);

    islem_suresi = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("\nProgramin calisma suresi: %.6f saniye\n\n", islem_suresi);

    return 0;
}

int ThreadDurum(){

    pthread_mutex_lock(&mutex);

    if(ThreadSayisi > 0){

        ThreadSayisi -= 1;

        pthread_mutex_unlock(&mutex);
        return 1;

    } else {

        pthread_mutex_unlock(&mutex);
        return 0;
    }
}


void *ParalelMergeSort(void *args) {

    struct Sort_SiralamaParametreleri *p = (struct Sort_SiralamaParametreleri *)args;

    if(p->baslangic < p->son){

        struct Sort_SiralamaParametreleri *params_1 = malloc(sizeof(struct Sort_SiralamaParametreleri));
        struct Sort_SiralamaParametreleri *params_2 = malloc(sizeof(struct Sort_SiralamaParametreleri));

        if(params_1 == NULL || params_2 == NULL){

            printf("\nBellek tahsisi gerceklestirilemedi!\n");
            return NULL;
        }

        params_1->kelimeler = p->kelimeler;
        params_1->baslangic = p->baslangic;
        params_1->son = p->son;
        params_2->kelimeler = p->kelimeler;
        params_2->baslangic = p->baslangic;
        params_2->son = p->son;

        int ortanca = (p->baslangic + p->son) / 2;

        params_1->son = ortanca;
        params_2->baslangic = ortanca + 1;

        pthread_t thread_1;
        pthread_t thread_2;

        if(ThreadDurum()){

            pthread_create(&thread_1, NULL, ParalelMergeSort, params_1);

        } else {

            MergeSort(params_1->kelimeler, params_1->baslangic, params_1->son);
        }

        if(ThreadDurum()){

            pthread_create(&thread_2, NULL, ParalelMergeSort, params_2);

        } else {

            MergeSort(params_2->kelimeler, params_2->baslangic, params_2->son);
        }

        pthread_join(thread_1, NULL);
        pthread_join(thread_2, NULL);

        Merge(params_1->kelimeler, params_1->baslangic, ortanca, params_2->son);

        free(params_1);
        free(params_2);

        return NULL;
    }
}


void MergeSort(char **kelimeler, int baslangic, int son){


    if(baslangic < son){

        int ortanca = (baslangic + son) / 2;

        MergeSort(kelimeler, baslangic, ortanca);
        MergeSort(kelimeler, ortanca + 1, son);

        Merge(kelimeler, baslangic, ortanca, son);
    }
}


void Merge(char **kelimeler, int baslangic, int ortanca, int son) {

    int i, j, k;
    int n1 = ortanca - baslangic + 1;
    int n2 = son - ortanca;

    char **Sol = (char **)malloc(n1 * sizeof(char *));
    char **Sag = (char **)malloc(n2 * sizeof(char *));

    if (Sol == NULL || Sag == NULL) {

        printf("\nBellek tahsisi gerceklestirilemedi!\n");
        exit(1);
    }

    for (i = 0; i < n1; i++){

        Sol[i] = kelimeler[baslangic + i];
    }

    for (j = 0; j < n2; j++) {

        Sag[j] = kelimeler[ortanca + 1 + j];
    }

    i = 0;
    j = 0;
    k = baslangic;
    while(i < n1 && j < n2){

        if(strcasecmp(Sol[i], Sag[j]) <= 0){

            kelimeler[k] = Sol[i];
            i++;

        } else {

            kelimeler[k] = Sag[j];
            j++;
        }

        k++;
    }

    while(i < n1){

        kelimeler[k] = Sol[i];
        i++;
        k++;
    }

    while(j < n2){

        kelimeler[k] = Sag[j];
        j++;
        k++;
    }

    free(Sol);
    free(Sag);
}


void *ParalelQuickSort(void *args) {

    struct Sort_SiralamaParametreleri *p = (struct Sort_SiralamaParametreleri *)args;

    if (p->baslangic < p->son) {

        int i = Bolumlendir(p->kelimeler, p->baslangic, p->son);

        struct Sort_SiralamaParametreleri *params_1 = malloc(sizeof(struct Sort_SiralamaParametreleri));
        struct Sort_SiralamaParametreleri *params_2 = malloc(sizeof(struct Sort_SiralamaParametreleri));

        if (params_1 == NULL || params_2 == NULL) {
            printf("\nBellek tahsisi gerceklestirilemedi!\n");
            return NULL;
        }

        params_1->kelimeler = p->kelimeler;
        params_1->baslangic = p->baslangic;
        params_1->son = i - 1;

        params_2->kelimeler = p->kelimeler;
        params_2->baslangic = i + 1;
        params_2->son = p->son;

        pthread_t thread_1;
        pthread_t thread_2;

        if (ThreadDurum()) {

            pthread_create(&thread_1, NULL, ParalelQuickSort, params_1);

        } else {

            QuickSort(params_1->kelimeler, params_1->baslangic, params_1->son);
        }

        if (ThreadDurum()) {

            pthread_create(&thread_2, NULL, ParalelQuickSort, params_2);

        } else {

            QuickSort(params_2->kelimeler, params_2->baslangic, params_2->son);
        }

        pthread_join(thread_1, NULL);
        pthread_join(thread_2, NULL);

        free(params_1);
        free(params_2);
    }

    return NULL;
}


void QuickSort(char **kelimeler, int baslangic, int son){

    if(baslangic < son){

        int i = Bolumlendir(kelimeler, baslangic, son);

        QuickSort(kelimeler, baslangic, i - 1);
        QuickSort(kelimeler, i + 1, son);
    }
}


int Bolumlendir(char **kelimeler, int baslangic, int son){

    char *pivot = kelimeler[son];

    int i = (baslangic - 1);

    for(int j = baslangic; j <= son; j++){

        if(strcasecmp(kelimeler[j], pivot) < 0){

            i++;
            Degistir(kelimeler, i, j);
        }
    }

    Degistir(kelimeler, i + 1, son);
    return(i + 1);
}


void Degistir(char **kelimeler, int i, int j){

    char *temp = kelimeler[i];
    kelimeler[i] = kelimeler[j];
    kelimeler[j] = temp;
}
