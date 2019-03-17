#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "coro_jmp.h"

static int coro_count;
static struct coro *coros;

void merge(int *l, int *r, int size)
{
    coro_this()->l = l, coro_this()->r = r, coro_this()->sz = size, coro_this()->i = 0,
        coro_this()->p1 = coro_this()->l, coro_this()->p2 = coro_this()->r, coro_this()->c = calloc(coro_this()->sz, sizeof(int));
    coro_yield();
    while (coro_this()->i < coro_this()->sz)
    {
        if (coro_this()->p1 >= coro_this()->r)
        {
            coro_this()->c[coro_this()->i++] = *(coro_this()->p2)++;
        }
        else if (coro_this()->p2 - coro_this()->l >= coro_this()->sz)
        {
            coro_this()->c[coro_this()->i++] = *(coro_this()->p1)++;
        }
        else if (*coro_this()->p1 < *coro_this()->p2)
        {
            coro_this()->c[coro_this()->i++] = *(coro_this()->p1)++;
        }
        else
        {
            coro_this()->c[coro_this()->i++] = *(coro_this()->p2)++;
        }
        coro_yield();
    }
    for(coro_this()->cnt = 0; coro_this()->cnt < coro_this()->sz; ++(coro_this()->cnt))
    {
        coro_this()->l[coro_this()->cnt] = coro_this()->c[coro_this()->cnt];
        coro_yield();
    }
    free(coro_this()->c);
    coro_yield();
    coro_return();
}

void merge_sort(int *arr, int size)
{
    coro_this()->arr = arr, coro_this()->size = size;
    coro_yield();
    if (coro_this()->size <= 1)
    {
        coro_return();
    }
    coro_yield();
    coro_call(merge_sort, coro_this()->arr, coro_this()->size / 2);
    coro_yield();
    coro_call(merge_sort, coro_this()->arr + coro_this()->size / 2, coro_this()->size - coro_this()->size / 2);
    coro_yield();
    coro_call(merge, coro_this()->arr, coro_this()->arr + coro_this()->size / 2, coro_this()->size);
    coro_return();
}

void sort_file(const char *filename)
{
    coro_this()->filename = filename, coro_this()->f = fopen(coro_this()->filename, "r"), coro_this()->dummy = 0, coro_this()->siz = 0;
    for (; fscanf(coro_this()->f, "%d", &coro_this()->dummy) == 1; ++coro_this()->siz) {
        coro_yield();
    }
    coro_this()->arr = calloc(coro_this()->siz, sizeof(int));
    coro_yield();
    fseek(coro_this()->f, 0, SEEK_SET);
    coro_yield();
    for (coro_this()->dummy = 0; fscanf(coro_this()->f, "%d", &coro_this()->arr[coro_this()->dummy]) == 1; ++coro_this()->dummy) {
        coro_yield();
    }
    coro_call(merge_sort, coro_this()->arr, coro_this()->siz);
    coro_yield();
    fclose(coro_this()->f);
    coro_yield();
    coro_this()->f = fopen(coro_this()->filename, "w");
    for (coro_this()->dummy = 0; coro_this()->dummy < coro_this()->siz; ++coro_this()->dummy) {
        coro_yield();
        fprintf(coro_this()->f, "%d ", coro_this()->arr[coro_this()->dummy]);
    }
    coro_yield();
    free(coro_this()->arr);
    coro_yield();
    fclose(coro_this()->f);
    coro_return();
}

void fmerge(char *file1, char *file2) {
    int x, y, is_x = 0, is_y = 0;
    const char *tmp = "__tmp.txt";
    FILE *f1 = fopen(file1, "r");
    FILE *f2 = fopen(file2, "r");
    FILE *newf = fopen(tmp, "w");
    while (1) {
        if (!is_x) {
            if (fscanf(f1, "%d", &x) == 1)
                is_x = 1;
        }
        if (!is_y) {
            if (fscanf(f2, "%d", &y) == 1)
                is_y = 1;
        }
        if (!is_x && !is_y) {
            break;
        }
        if (!is_y || is_x && x < y) {
            fprintf(newf, "%d ", x);
            is_x = 0;
        } else {
            fprintf(newf, "%d ", y);
            is_y = 0;
        }
    }
    fclose(f1);
    fclose(f2);
    fclose(newf);
    remove(file1);
    remove(file2);
    rename(tmp, file1);
}

void fmerge_sort(char *files[], int n)
{
    if (n <= 1) {
        return;
    }
    fmerge_sort(files, n / 2);
    fmerge_sort(files + n / 2, n - n / 2);
    fmerge(files[0], files[n / 2]);
}

int main(int argc, char *argv[])
{
    coro_count = argc - 1;
    coros = calloc(coro_count, sizeof(struct coro));
    for (int i = 0; i < coro_count; ++i) {
        coro_init(&coros[i]);
        if (i >= coro_count) {
            break;
        }
    }
    coro_call(sort_file, argv[curr_coro_i+1]);
    coro_finish();
    coro_wait_all();
    fmerge_sort(argv + 1, argc - 1);
    rename(argv[1], "BIG.txt");
    return 0;
}
