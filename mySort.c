#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

struct Sort_Parameters {
    char **words;
    int start;
    int end;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int ThreadCount = 0;

int ThreadStatus();
void Merge(char **words, int start, int middle, int end);
void MergeSort(char **words, int start, int end);
void *ParallelMergeSort(void *args);
void QuickSort(char **words, int start, int end);
void *ParallelQuickSort(void *args);
int Partition(char **words, int start, int end);
void Swap(char **words, int i, int j);

int main(int argc, char *argv[]) {

    struct timeval start, end;
    double execution_time;

    gettimeofday(&start, NULL);

    if (argc != 5) {
        printf("\nMissing arguments\n");
        printf("\nFormat: <InputFile.txt> <OutputFile.txt> <Thread Count> <Sorting Algorithm (merge, quick)>\n\n");
        return 1;
    }

    char *InputFile = argv[1];
    char *OutputFile = argv[2];
    ThreadCount = atoi(argv[3]);

    if (ThreadCount < 0) {
        printf("\nInvalid Thread Count!\n");
        return 1;
    }

    char *SortingAlgorithm = argv[4];

    printf("\nInput File: %s\n", InputFile);
    printf("Output File: %s\n", OutputFile);
    printf("Thread Count: %d\n", ThreadCount);
    printf("Sorting Algorithm: %s\n", SortingAlgorithm);

    FILE *input_file;

    input_file = fopen(InputFile, "r");
    if (input_file == NULL) {
        printf("\nFailed to open Input File: %s\n", InputFile);
        return 1;
    }

    char **words = NULL;
    char word[100];
    int word_count = 0;
    while (fscanf(input_file, "%s", word) != EOF) {
        words = (char **)realloc(words, (word_count + 1) * sizeof(char *));
        if (words == NULL) {
            printf("\nMemory allocation failed!\n");
            return 1;
        }
        words[word_count] = strdup(word);
        word_count++;
    }

    fclose(input_file);

    struct Sort_Parameters parameters;
    parameters.words = words;
    parameters.start = 0;
    parameters.end = word_count - 1;

    if (strcmp(SortingAlgorithm, "merge") == 0) {
        if (ThreadCount > 0) {
            ParallelMergeSort(&parameters);
        } else {
            MergeSort(parameters.words, parameters.start, parameters.end);
        }
    } else if (strcmp(SortingAlgorithm, "quick") == 0) {
        if (ThreadCount > 0) {
            ParallelQuickSort(&parameters);
        } else {
            QuickSort(parameters.words, parameters.start, parameters.end);
        }
    } else {
        printf("\nInvalid algorithm. Available algorithms: merge, quick\n\n");
        return 1;
    }

    FILE *output_file;
    int i;

    output_file = fopen(OutputFile, "w");
    if (output_file == NULL) {
        printf("\nFailed to open Output File: %s\n", OutputFile);
        return 1;
    }

    for (i = 0; i < word_count; i++) {
        fprintf(output_file, "%s\n", words[i]);
        free(words[i]);
    }

    fclose(output_file);
    free(words);

    printf("\nSorting Completed.\n");

    gettimeofday(&end, NULL);

    execution_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("\nProgram execution time: %.6f seconds\n\n", execution_time);

    return 0;
}

int ThreadStatus() {
    pthread_mutex_lock(&mutex);
    if (ThreadCount > 0) {
        ThreadCount -= 1;
        pthread_mutex_unlock(&mutex);
        return 1;
    } else {
        pthread_mutex_unlock(&mutex);
        return 0;
    }
}

void *ParallelMergeSort(void *args) {
    struct Sort_Parameters *p = (struct Sort_Parameters *)args;
    if (p->start < p->end) {
        struct Sort_Parameters *params_1 = malloc(sizeof(struct Sort_Parameters));
        struct Sort_Parameters *params_2 = malloc(sizeof(struct Sort_Parameters));
        if (params_1 == NULL || params_2 == NULL) {
            printf("\nMemory allocation failed!\n");
            return NULL;
        }
        params_1->words = p->words;
        params_1->start = p->start;
        params_1->end = p->end;
        params_2->words = p->words;
        params_2->start = p->start;
        params_2->end = p->end;

        int middle = (p->start + p->end) / 2;
        params_1->end = middle;
        params_2->start = middle + 1;

        pthread_t thread_1;
        pthread_t thread_2;

        if (ThreadStatus()) {
            pthread_create(&thread_1, NULL, ParallelMergeSort, params_1);
        } else {
            MergeSort(params_1->words, params_1->start, params_1->end);
        }

        if (ThreadStatus()) {
            pthread_create(&thread_2, NULL, ParallelMergeSort, params_2);
        } else {
            MergeSort(params_2->words, params_2->start, params_2->end);
        }

        pthread_join(thread_1, NULL);
        pthread_join(thread_2, NULL);

        Merge(params_1->words, params_1->start, middle, params_2->end);

        free(params_1);
        free(params_2);

        return NULL;
    }
}

void MergeSort(char **words, int start, int end) {
    if (start < end) {
        int middle = (start + end) / 2;
        MergeSort(words, start, middle);
        MergeSort(words, middle + 1, end);
        Merge(words, start, middle, end);
    }
}

void Merge(char **words, int start, int middle, int end) {
    int i, j, k;
    int n1 = middle - start + 1;
    int n2 = end - middle;

    char **Left = (char **)malloc(n1 * sizeof(char *));
    char **Right = (char **)malloc(n2 * sizeof(char *));

    if (Left == NULL || Right == NULL) {
        printf("\nMemory allocation failed!\n");
        exit(1);
    }

    for (i = 0; i < n1; i++) {
        Left[i] = words[start + i];
    }

    for (j = 0; j < n2; j++) {
        Right[j] = words[middle + 1 + j];
    }

    i = 0;
    j = 0;
    k = start;
    while (i < n1 && j < n2) {
        if (strcasecmp(Left[i], Right[j]) <= 0) {
            words[k] = Left[i];
            i++;
        } else {
            words[k] = Right[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        words[k] = Left[i];
        i++;
        k++;
    }

    while (j < n2) {
        words[k] = Right[j];
        j++;
        k++;
    }

    free(Left);
    free(Right);
}

void *ParallelQuickSort(void *args) {
    struct Sort_Parameters *p = (struct Sort_Parameters *)args;
    if (p->start < p->end) {
        int i = Partition(p->words, p->start, p->end);
        struct Sort_Parameters *params_1 = malloc(sizeof(struct Sort_Parameters));
        struct Sort_Parameters *params_2 = malloc(sizeof(struct Sort_Parameters));
        if (params_1 == NULL || params_2 == NULL) {
            printf("\nMemory allocation failed!\n");
            return NULL;
        }

        params_1->words = p->words;
        params_1->start = p->start;
        params_1->end = i - 1;
        params_2->words = p->words;
        params_2->start = i + 1;
        params_2->end = p->end;

        pthread_t thread_1;
        pthread_t thread_2;

        if (ThreadStatus()) {
            pthread_create(&thread_1, NULL, ParallelQuickSort, params_1);
        } else {
            QuickSort(params_1->words, params_1->start, params_1->end);
        }

        if (ThreadStatus()) {
            pthread_create(&thread_2, NULL, ParallelQuickSort, params_2);
        } else {
            QuickSort(params_2->words, params_2->start, params_2->end);
        }

        pthread_join(thread_1, NULL);
        pthread_join(thread_2, NULL);

        free(params_1);
        free(params_2);
    }

    return NULL;
}

void QuickSort(char **words, int start, int end) {
    if (start < end) {
        int i = Partition(words, start, end);
        QuickSort(words, start, i - 1);
        QuickSort(words, i + 1, end);
    }
}

int Partition(char **words, int start, int end) {
    char *pivot = words[end];
    int i = (start - 1);
    for (int j = start; j <= end; j++) {
        if (strcasecmp(words[j], pivot) < 0) {
            i++;
            Swap(words, i, j);
        }
    }
    Swap(words, i + 1, end);
    return (i + 1);
}

void Swap(char **words, int i, int j) {
    char *temp = words[i];
    words[i] = words[j];
    words[j] = temp;
}
