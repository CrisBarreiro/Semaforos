#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define N 10 //tamaño del buffer

/*Declaración de los semáforos*/

sem_t *empty;
sem_t *full;
sem_t *mutex;

/******************************/

main(){

int  shared, i;
int *buffer, *value;

  	value = (int*) malloc(sizeof(int));
  	
  		/*Apertura de los semáforos*/
  	
  		/*Dado que el consumidor ha de ser lanzado después del productor, los semáforos ya habrán sido creados e inicializados, por lo que el consumidor 
  		sólo debe abrilos pero sin inicializarlos.*/
	
		srand(time(NULL));
		
		/*EMPTY se inicializa con valor N, y se irá decrementando su valor a medida que el productor introduzca elementos en el mismo. En caso de que 
		se llene el buffer, tomará valor 0, y el productor se bloqueará. Del mismo modo, cada vez que el consumidor consuma un elemento, incrementará su
		valor para que el productor pueda seguir produciendo.*/
		if ((empty = sem_open ("EMPTY", N)) == SEM_FAILED){
			perror("Error en la creación de EMPTY\n");
		}
		
		/*FULL se inicializa con valor 0, y se irá incrementando su valor a medida que el productor introduzca elementos en el mismo. Del mismo modo,
		cada vez que el consumidor consuma un elemento, decrementará su valor. En caso de que se vacíe el buffer, tomará valor 0, por lo que el consumidor
		se bloqueará en espera de que el productor produzca un elemento.*/
		if ((full = sem_open ("FULL", 0)) == SEM_FAILED){
			perror("Error en la creación de FULL\n");
		}
		/*MUTEX está pensado para garantizar exclusión mutua, de modo que para que un proceso pueda acceder a la región crítica, este semáforo ha de tener 
		valor 1, y hacerse un down del mismo, para impedir que otro proceso acceda al mismo tiempo. Al salir de la región crítica, se realizará un up
		del mismo para desbloquear a aquellos procesos que estuviesen esperando para entrar.*/
		if ((mutex = sem_open ("MUTEX", 1)) == SEM_FAILED){
			perror("Error en la creación de MUTEX\n");
		}
		
		/*****************************************************************/
		
		/*Apertura de la zona de memoria compartida*/
		
		/*Al igual que sucede con los semáforos, es el primer productor que se lanza quien crea el área de memoria compartida, de modo que los
		consumidores sólo tienen que abrirla y proyectarla en memoria. Dado que el consumidor no escribe en dicha área, deberá abrir el archivo
		con permisos de sólo lectura, y realizará la proyección con protección de escritura y mapeado compartido.*/
		
		shared = open ("buffer", O_RDONLY);
		if (shared == -1){
			perror ("Error en el open\n");
			exit(1);
		}
		buffer = (int*) mmap(NULL, N*sizeof(int), PROT_READ, MAP_SHARED, shared, 0);
		if (*buffer == -1){
			perror("Error en la función mmap\n");
			exit(1);
		}
		
		/*****************************************************************/
		
		/*Lectura del buffer*/
		
		/*Para asegurarnos de que los semáforos serán cerrados y deslinkados al final de la ejecución del programa, debemos asegurar que el consumidor
		entre en el bucle de consumo un número finito de veces pues, en caso contrario, deberíamos abortar manualmente la ejecución del programa y no
		permituiríamos la ejecución íntegra del código. Esto generaría un problema ya que, al no deslinkar los semáforos al comienzo del productor, 
		por los motivos descritos en el código del mismo, cuando el productor los quisiese crear éstos ya existiesen en el kernel y no se les asignase
		el valor deseado, como se explica también en el código del productor.
		
		Para poder acceder a la región crítica, debe de existir algún elemento en el buffer (full > 0), y se debe garantizar la exclusión mutua 
		(mutex > 0). En caso de que no se cumpla uno de estos requisitos, el proceso se bloqueará esperando a ser desbloqueado por un productor.
		Después de haber consumido un elemento en la posición del buffer igual al valor de FULL, el consumidor hará un up de mutex, para desbloquear 
		a aquellos procesos que deseasen entrar a la región crítica, y un up de EMPTY, para desbloquar a aquellos procesos que estuviesen esperando para 
		entrar en la región crítica, de haberlos, y para indicar que se ha consumido un elemento del buffer, según el planteamiento de los 
		semáforos, explicado al inicio.*/
		
		for (i = 0; i < 10*N;i++){
			sem_wait(full);
			sem_wait(mutex);
			sem_getvalue(full, value);
			printf("El consumidor ha sacado el valor %d de la posición %d\n", i, *value);
			fflush(stdout);
			sleep(rand()%5);
			sem_post(mutex);
			sem_post(empty);
		}
		
		/*****************************************************************/
		
		printf("Fin del consumo\n");
		
		/*Cierre de la proyección, el archivo y los semáforos*/
		
		munmap(buffer, N*sizeof(int));
		close(shared);
		sem_close(empty);
		sem_close(full);
		sem_close(mutex);
		
		/*****************************************************************/
		
	return (0);
}
	
