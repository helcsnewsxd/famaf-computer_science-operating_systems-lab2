# Introducción al proyecto
Implementamos un administrador de recursos o semáforo a través del uso de un arreglo, teniendo así en cada posición un semáforo distinto. Para cada semáforo definimos una estructura con dos campos, uno para guardar el valor y otro para el spinlock, el cual está encargado de regular el acceso correcto a los recursos por parte de los procesos.

```c
// Estructura del semáforo
struct sem{
  int value;
  struct spinlock lock;
}; 

// Arreglo de semáforos
struct sem semaphore_counter[MAXCNTSEM];
```

Lo que hicimos fue inicializar el semáforo deseado con sem_open, y administrar el uso de recursos entre procesos usando las funciones sem_up y sem_down y los respectivos spinlocks de cada semáforo para evitar las interrupciones del procesador o que los datos se sobreescriban, logrando así un acceso síncrono a los recursos por parte de los procesos. 

Para ello, se hizo uso de las syscalls acquire y release a la hora de realizar modificaciones seguras a los valores del semáforo, y se utilizaron las syscalls sleep y wakeup para regular el acceso a los recursos de los procesos dormidos.

Se detallan al final del informe algunas de las implementaciones interesantes.

# Explicación syscalls utilizadas

## Introducción al funcionamiento de **acquire** y **release**
Las funciones **acquire** y **release** protegen secciones compartidas entre varios procesos, para evitar que se sobreescriban datos por error.
La estructura del código es la siguiente

**acquire()**
Sección critica del código donde se accede a un recurso compartido
**release()**

Para evitar deadlocks ("puntos muertos" donde ningún proceso puede seguir su ejecución), se deshabilitan las interrupciones al ejecutar **acquire()** y se rehabilitan al ejecutar **release()**, es decir, no se interrumpe la ejecución en la CPU del proceso mientras se está ejecutando la sección crítica del código. (Al ejecutarse varios **acquire()** anidados, para rehabilitar las interrupciones se necesita hacer la misma cantidad de **release()**, eso se gestiona mediante la variable *noff* del *struct cpu*)


## acquire(*struct spinlock \*lk*)
Primero deshabilita las interrupciones del procesador.

Luego toma un puntero a un spinlock *lk*, revisa que no este bloqueado ya por la misma CPU que la esta llamando (en ese caso da un error) y ejecuta un bucle del que se sale solo cuando se desbloquea el spinlock. 

En ese ciclo se realiza un swap atómico (en una sola instrucción) de *&lk->locked*, escribiendole un 1 y guardando el valor que tenia anteriormente ese espacio de memoria, si el valor anterior era 0 (el spinlock estaba desbloqueado) termina el ciclo y si era diferente de 0 (Un "1") se sigue ejecutando hasta que se libere el spinlock.

Una vez ya se libero el spinlock, se guarda en *lk->cpu* un puntero a la estructura de la CPU que esta ejecutando esta función

## release(*struct spinlock \*lk*): 
Toma un puntero a un spinlock *lk*, revisa que no esté desbloqueado ya (en ese caso da un error) y si no lo esta, desbloquea el spinlock *lk*, asignando 0 a *lk->cpu* (porque ninguna CPU está bloqueando el spinlock) y asignando 0 a *&lk->locked*

Luego habilita nuevamente las interrupciones del procesador.

## sleep(*void \*chan, struct spinlock \*lk*)
Bloquea la tabla del proceso que ejecutó la función, libera el spinlock *lk*, asigna en *p->chan* el argumento  *chan* y devuelve el control del CPU hasta que el proceso es despertado por **wakeup**.

Una vez es despertado, modifica el valor *p->chan* a, desbloquea la tabla del proceso y vuelve a bloquear el spinlock *lk* (Volviendo el spinlock al estado inicial antes de que fuera llamado el sleep)

## wakeup(*void \*chan*): 
Revisa todos los procesos excepto el suyo, bloqueando la tabla del proceso y revisando si el proceso está dormido y esperando por *chan*, si se cumple esa condición entonces despierta al proceso (Cambia su estado de *durmiendo* a *listo para ejecutarse*)

Una vez deja de revisar la tabla del proceso, la libera.

## argint(*int n, int \*ip*)
Obtiene el argumento *n*-ésimo insertado en la pila de usuario por el código de usuario antes de que el usuario solicite una llamada al sistema y lo escribe en *ip*.
# Implementaciones Interesantes
Relacionado al funcionamiento de los spinlocks y al acceso de los recursos por parte de los procesos, alguna de las implementaciones destacables son las soluciones a los siguientes problemas:

1. El valor del semáforo disminuye utilizando la función sem_down, y un proceso quiere acceder a un recurso.

```c
	// Inicio de zona crítica para evitar que datos se sobreescriban

	// Se utiliza acquire para realizar un lock de procesos y evitar que el procesador genere una interrupción o se sobreescriban datos.
    acquire(&semaphore_counter[sem_id].lock);

	// Si el contador está en cero, los procesos que quieran acceder a los recursos deberán pasar a estar "dormidos" ya que el contador < 1.
    while(semaphore_counter[sem_id].value == 0){                               
    sleep(&semaphore_counter[sem_id], &semaphore_counter[sem_id].lock); // Duerme aquellos procesos que necesiten el recurso en el caso de que el contador esté en cero. Se despertarán cuando un proceso ejecute sem_up
    }                                                                          
    semaphore_counter[sem_id].value--;  // Disminuye el valor del semáforo en caso de que el contador > 0                                
                                        
    release(&semaphore_counter[sem_id].lock); // Se Utiliza release para liberar el spinlock y vuelve a habilitar las interrupciones del procesador. 

    // Fin de zona crítica
```

2. El valor del semáforo aumenta utilizando la función sem_up, y un proceso quiere acceder a un recurso.

```c
	// Zona crítica para evitar que datos se sobreescriban

	// Se utiliza acquire para realizar un lock de procesos y evitar que el procesador genere una interrupción o se sobreescriban datos.
  acquire(&semaphore_counter[sem_id].lock); 

	// Si el semáforo tenía inicialmente valor cero, tendrán prioridad para acceder los recursos aquellos procesos dormidos. Estos procesos dormidos son "despertados" por la syscall wakeup.
    if(semaphore_counter[sem_id].value == 0){
    wakeup(&semaphore_counter[sem_id]); 
    }

    semaphore_counter[sem_id].value++; // Aumenta el valor del semáforo

    release(&semaphore_counter[sem_id].lock); // Se Utiliza release para liberar el spinlock y vuelve a habilitar las interrupciones del procesador. 
  
    // Fin de zona crítica.
```