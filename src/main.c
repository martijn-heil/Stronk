#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
//#include "mcpr.h"
#include "pthread.h"


void *doFancyStuffs(void *arg) {
    printf("Aye ish doink fancy stuffs rite now! Like zis!\n");
    printf("Oh ghai comrad!\n");

    return NULL;
}

int main(void) {
    setlocale(LC_ALL, ""); // important

    char *test = u8"♥";
    puts(test);
    printf("%zu\n", strlen(test));

    pthread_t thread1;
    int  iret1;

    /* Create independent threads each of which will execute function */

    iret1 = pthread_create(&thread1, NULL, doFancyStuffs, NULL);
    if(iret1)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
    }

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */

    pthread_join(thread1, NULL);

    exit(EXIT_SUCCESS);
}
