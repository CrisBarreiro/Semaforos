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

int shared, i;
int *buffer, *value;

  	value = (int*) malloc(sizeof(int));

	
		srand(time(NULL));
		
		/*Creación de los semáforos*/
		
		/*El productor crea e inicializa los semáforos. En caso de que se especifique como flag O_CREATE, si ya existe un semáforo creado con ese mismo 
		nombre, los parámetros mode y value son ignorados. Es decir, que si lanzamos varios productores, solo el primero creará los semáforos y les dará
		un valor, el resto, al estar ya creados, no lo modificará, de modo que utilizarán los semáforos con los valores que tienen en el kernel.
		El productor primero crea e inicializa los semáforos, mientras que procesos sucesivos sólo los abrirán.*/
		
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
		
		/*Definición de un área de memoria compartida*/
		
		/*El productor primero crea la zona de memoria, mientras que los demás procesos acceden a ella.
		Como el productor primero es quien crea el área de memoria compartida, el resto de productores sólo tienen que acceder a ella con permisos
		de lectura y escritura.*/
		
		shared = open ("buffer", O_RDWR);
		if (shared == -1){
			perror ("Error en el open\n");
			exit(1);
		}
		/*Se proyecta el archivo en el mapa de memoria del proceso, indicando que la proyección puede ser escrita, y que la proyección será compartida,
		es decir, que las actualizaciones que se produzcan en la proyección serán vistas por el resto de procesos que proyecten el archivo.*/
		buffer = (int*) mmap(NULL, N*sizeof(int), PROT_WRITE, MAP_SHARED, shared, 0); 
		if (*buffer == -1){
			perror("Error en la función mmap\n");
			exit(1);
		}
		
		/*****************************************************************/
		
		/*Escritura en el buffer*/
		
		/*Para asegurarnos de que los semáforos serán cerrados, para permitir que el programa funcione correctamente, el productor ha de entrar en
		el bucle un número finito de veces. En este bloque, el productor escribe en el buffer un número determinado de veces. Para poder acceder a
		la escritura en el buffer, necesita hacer un down de empty y de mutex antes de entrar a la región crítica. En caso de que el buffer esté lleno
		(empty = 0), o de que el otro proceso esté accediendo a la región crítica en ese momento y por lo tanto no se pueda garantizar exclusión mutua,
		(mutex = 0), el proceso se bloquerá hasta que un consimidor lo desbloquee al haber consumido un elemento del buffer y haber salido de la región
		crítica. El proceso escribe un valor en la posición del buffer correspondiente al valor del semáforo FULL y a continuación realiza un up de mutex
		y full, para despertar al consumidor en caso de que éste se haya bloqueado por no poder acceder a la región crítica bien porque ésta estuviese
		ocupada, o bien porque no hubiese elementos que consumir en el buffer, y para indicar que existe un elemento más en el buffer según el 
		planteamiento de los semáforos explicado al inicio.*/
		
		for (i = 0; i < 10*N; i++){
			sleep(rand()%5);
			sem_wait(empty);
			sem_wait(mutex);
			sem_getvalue(full, value);
			buffer[*value] = i;
			printf("El productor ha introducido el valor %d en la posición %d\n", i, *value);
			fflush(stdout);
			sem_post(mutex);
			sem_post(full);
		}
		
		printf("Fin de la producción\n");
		
		/*****************************************************************/
		
		/*Cierre de la proyección, el archivo y los semáforos*/
		
		munmap(buffer, N*sizeof(int));
		close(shared);
		sem_close(empty);
		sem_close(full);
		sem_close(mutex);
		
		/*****************************************************************/
	return (0);
}
	
