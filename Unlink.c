#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

main(){
	sem_unlink("EMPTY");
	sem_unlink("FULL");
	sem_unlink("MUTEX");
	
	return (0);
}

/*Dado que ni los productores ni los consumidores eliminan los semáforos del kernel, dado que para ello deberían esperar a la finalización del resto
de procesos, debemos eliminar manualmente los semáforos mediante la ejecución de este código al finalizar la ejecución de los demás procesos.*/
