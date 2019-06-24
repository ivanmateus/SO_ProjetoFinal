#include <gmp.h>
#include <math.h>
#include <mpfr.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "rand_bm.h"

#define NUMTHREADS 12

typedef struct {
    int fromidx, length;
} thread_arg, *ptr_thread_arg;

pthread_t threads[NUMTHREADS];
thread_arg arguments[NUMTHREADS];

mpf_t S, E, r, sig, T, total;
mpf_t *trials;
int M;

void proccess(int index, mpf_t tmp_a, mpf_t tmp_b, mpf_t t, mpfr_t tmp_r,
              struct BoxMullerState state) {
    // a = r - (sigma²)/2
    mpf_mul(tmp_a, sig, sig);
    mpf_div_ui(tmp_a, tmp_a, 2);
    mpf_sub(tmp_a, r, tmp_a);
    mpf_mul(tmp_a, tmp_a, T);

    // b = sigma * sqrt(T)
    mpf_sqrt(tmp_b, T);
    mpf_mul(tmp_b, tmp_b, sig);

    // a = a + b * randomNumber
    double random = boxMullerRandom(&state);
    mpf_t rand_num;
    mpf_init_set_d(rand_num, random);
    mpf_mul(tmp_b, tmp_b, rand_num);
    mpf_add(tmp_a, tmp_a, tmp_b);

    // a = exp(a)
    mpfr_set_f(tmp_r, tmp_a, MPFR_RNDN);
    mpfr_exp(tmp_r, tmp_r, MPFR_RNDN);
    mpfr_get_f(tmp_a, tmp_r, MPFR_RNDN);

    // t = S * a
    mpf_mul(t, S, tmp_a);

    // a = exp(-r * T)
    mpf_neg(tmp_a, r);
    mpf_mul(tmp_a, tmp_a, T);
    mpfr_set_f(tmp_r, tmp_a, MPFR_RNDN);
    mpfr_exp(tmp_r, tmp_r, MPFR_RNDN);
    mpfr_get_f(tmp_a, tmp_r, MPFR_RNDN);

    // a = a * max{t - E, 0}
    mpf_sub(tmp_b, t, E);
    mpf_cmp_ui(tmp_b, 0) > 0 ? mpf_mul(tmp_a, tmp_a, tmp_b) : mpf_mul_ui(tmp_a, tmp_a, 0);

    mpf_init_set(trials[index], tmp_a);
    // mpf_add(total, total, tmp_a);
}

void *thread_func(void *arg) {
    ptr_thread_arg argument = (ptr_thread_arg)arg;
    int endix = argument->fromidx + argument->length;

    mpf_t t, tmp_a, tmp_b;
    mpfr_t tmp_r;
    mpf_init(tmp_a);
    mpf_init(tmp_b);
    mpf_init(t);
    mpfr_init(tmp_r);

    struct BoxMullerState state;
    initBoxMullerState(&state);

    printf("thread\n");

    for (int i = argument->fromidx; i < endix; i++) {
        proccess(i, tmp_a, tmp_b, t, tmp_r, state);
    }

    mpf_clear(tmp_a);
    mpf_clear(tmp_b);
    mpf_clear(t);
    mpfr_clear(tmp_r);
};

int main(void) {
    clock_t start, end;
    start = clock();
    srand(time(0));

    mpf_init(S);
    mpf_init(E);
    mpf_init(r);
    mpf_init(sig);
    mpf_init(T);
    // mpf_init_set_si(total, 0);

    gmp_scanf("%Ff", &S);
    gmp_scanf("%Ff", &E);
    gmp_scanf("%Ff", &r);
    gmp_scanf("%Ff", &sig);
    gmp_scanf("%Ff", &T);
    scanf("%d", &M);

    trials = (mpf_t *)malloc(M * sizeof(mpf_t));

    int length = M / NUMTHREADS;
    int rem = M % NUMTHREADS;

    for (int i = 0; i < NUMTHREADS; i++) {
        arguments[i].fromidx = i * length;
        arguments[i].length = length;
        if (i == (NUMTHREADS - 1)) arguments[i].length += rem;
        pthread_create(&(threads[i]), NULL, thread_func, &(arguments[i]));
    }
    for (int i = 0; i < NUMTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // for (int i = 0; i < M; i++) {
    //     proccess(i);
    // }

    // mean = a = total / M
    mpf_t total, tmp_a, tmp_b;
    mpf_init(tmp_a);
    mpf_init(tmp_b);
    mpf_init_set_ui(total, 0);
    mpf_div_ui(tmp_a, total, M);

    mpf_t mean, stddev, confwidth;
    mpf_init(confwidth);
    mpf_init_set(mean, tmp_a);
    mpf_init_set_ui(stddev, 0);
    for (int i = 0; i < M; i++) {
        mpf_sub(tmp_a, trials[i], mean);
        mpf_mul(tmp_a, tmp_a, tmp_a);
        mpf_add(stddev, stddev, tmp_a);
    }
    mpf_div_ui(stddev, stddev, M);
    mpf_sqrt(stddev, stddev);

    mpf_set_ui(tmp_b, M);
    mpf_sqrt(tmp_b, tmp_b);
    mpf_div(tmp_a, stddev, tmp_b);
    mpf_set_d(tmp_b, 1.96);
    mpf_mul(confwidth, tmp_a, tmp_b);

    mpf_sub(tmp_a, mean, confwidth);
    mpf_add(tmp_b, mean, confwidth);

    // Printing entry variables
    gmp_printf("%Ff\n%Ff\n%Ff\n%Ff\n%Ff\n", S, E, r, sig, T);
    printf("%d\n", M);

    end = clock();
    printf("%lfs\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    // confmin
    gmp_printf("%Ff\n", tmp_a);

    // confmax
    gmp_printf("%Ff\n", tmp_b);

    return 0;
}