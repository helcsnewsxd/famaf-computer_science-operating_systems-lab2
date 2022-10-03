# **Laboratorio número 2 de Sistemas Operativos 2022 - Grupo 12 | FaMAF UNC**

## **Integrantes del grupo:**

- Lautaro Bachmann (lautaro.bachmann@mi.unc.edu.ar)
- Juan Bratti (juanbratti@mi.unc.edu.ar)
- Gonzalo Canavesio (gonzalo.canavesio@mi.unc.edu.ar)
- Emanuel Herrador (emanuel.nicolas.herrador@unc.edu.ar)


## **Índice**


## **Introducción al proyecto**
Implementamos un administrador de recursos o semáforo a través del uso de un arreglo, teniendo así en cada posición un semáforo distinto. Para cada semáforo definimos una estructura con dos campos, uno para guardar el valor y otro para el spinlock, el cual está encargado de regular el acceso correcto a los recursos por parte de los procesos.

Luego, en la función ping pong usamos esta implementación para imprimir por pantalla de forma organizada y alterna la palabra "ping" seguida de "pong"; se utilizó la syscall fork para controlar la ejecución de los procesos con la implementación propuesta de los semáforos.

## **Modularización**

### **Syscalls utilizadas**
#### **Introducción al funcionamiento de *acquire* y *release***
Las funciones **acquire** y **release** protegen secciones compartidas entre varios procesos, para evitar que se sobreescriban datos por error.
La estructura del código es la siguiente

**acquire()**

- Sección critica del código donde se accede a un recurso compartido

**release()**

Para evitar deadlocks ("puntos muertos" donde ningún proceso puede seguir su ejecución), se deshabilitan las interrupciones al ejecutar **acquire()** y se rehabilitan al ejecutar **release()**, es decir, no se interrumpe la ejecución en la CPU del proceso mientras se está ejecutando la sección crítica del código. (Al ejecutarse varios **acquire()** anidados, para rehabilitar las interrupciones se necesita hacer la misma cantidad de **release()**, eso se gestiona mediante la variable *noff* del *struct cpu*)

#### **acquire(*struct spinlock \*lk*)**
Primero deshabilita las interrupciones del procesador.

Luego toma un puntero a un spinlock *lk*, revisa que no este bloqueado ya por la misma CPU que la esta llamando (en ese caso da un error) y ejecuta un bucle del que se sale solo cuando se desbloquea el spinlock. 

En ese ciclo se realiza un swap atómico (en una sola instrucción) de *&lk->locked*, escribiendole un 1 y guardando el valor que tenia anteriormente ese espacio de memoria, si el valor anterior era 0 (el spinlock estaba desbloqueado) termina el ciclo y si era diferente de 0 (Un "1") se sigue ejecutando hasta que se libere el spinlock.

Una vez ya se libero el spinlock, se guarda en *lk->cpu* un puntero a la estructura de la CPU que esta ejecutando esta función

#### **release(*struct spinlock \*lk*)**
Toma un puntero a un spinlock *lk*, revisa que no esté desbloqueado ya (en ese caso da un error) y si no lo esta, desbloquea el spinlock *lk*, asignando 0 a *lk->cpu* (porque ninguna CPU está bloqueando el spinlock) y asignando 0 a *&lk->locked*

Luego habilita nuevamente las interrupciones del procesador.

#### **sleep(*void \*chan, struct spinlock \*lk*)**
Bloquea la tabla del proceso que ejecutó la función, libera el spinlock *lk*, asigna en *p->chan* el argumento  *chan* y devuelve el control del CPU hasta que el proceso es despertado por **wakeup**.

Una vez es despertado, modifica el valor *p->chan* a, desbloquea la tabla del proceso y vuelve a bloquear el spinlock *lk* (Volviendo el spinlock al estado inicial antes de que fuera llamado el sleep)

#### **wakeup(*void \*chan*)**
Revisa todos los procesos excepto el suyo, bloqueando la tabla del proceso y revisando si el proceso está dormido y esperando por *chan*, si se cumple esa condición entonces despierta al proceso (Cambia su estado de *durmiendo* a *listo para ejecutarse*)

Una vez deja de revisar la tabla del proceso, la libera.

#### **argint(*int n, int \*ip*)**
Obtiene el argumento *n*-ésimo insertado en la pila de usuario por el código de usuario antes de que el usuario solicite una llamada al sistema y lo escribe en *ip*.

### **Implementación semaforo**

#### **Manejo de secciones críticas**
En todas aquellas secciones donde se utilizan recursos compartidos, como por ejemplo al modificar el valor del semaforo, se utilizan las funciones **acquire** y **release** para asegurarnos de que no se sobreescriban los datos y logrando así un acceso síncrono a los recursos por parte de los procesos. 


#### **Estructuras del semáforo**
```c
// Estructura del semáforo
struct sem{
  int value;
  struct spinlock lock;
}; 

// Arreglo de semáforos
struct sem semaphore_counter[MAXCNTSEM];
```
#### **Funciones para el manejo de semaforos y detalles de implementación**
Para inicializar el semáforo deseado se utiliza sem_open colocando como argumentos el ID del semaforo y el valor inicial del semaforo. El ID del semáforo va a ser el indice del arreglo de semaforos al cual accederemos, y en caso de ya estar siendo utilizado se le informa al usuario mediante un error. Se inicializa el *lock* del semaforo y se lo utiliza para bloquearlo mientras se le asigna al semaforo su valor inicial.

Para administrar el uso de recursos entre procesos se usan las funciones **sem_up** y **sem_down**. Dentro de estas funciones se utilizaron las syscalls sleep y wakeup para regular el acceso a los recursos y administrar los procesos dormidos, cumpliendo con la consigna de bloquer los procesos cuando el valor del semaforo es 0 al utilizar **sem_down** y desbloquearlos cuando el valor del semaforo es 0 al utilizar **sem_up**

Para definir cuando un semaforo esta "bloqueado", asignamos al valor del semaforo el entero -1. En el arranque del sistema operativo ejecutamos la función **seminit** que se encarga de inicializar el valor de todos los semaforos en -1 para asi poder desbloquearlos luego mientras los vamos necesitando utilizar. La función **sem_close** despierta todos los procesos que hayan sido mandados a dormir por el semaforo y establece el valor del semaforo en -1.

Se detallan a continuación algunas de las implementaciones interesantes.


### **Implementaciones Interesantes**
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

Relacionado a la función ping pong, lo que hicimos fue inicializar dos semáforos, uno para controlar el proceso que imprime por pantalla "ping" y otro para el proceso que imprime "pong".
El semáforo 0 se inicializa en 1 y el semáforo 1 se inicializa en 0 para luego a través de las funciones sem_up y sem_down intercalar la ejecución de cada proceso.
```c
  // abrimos un semáforo para el ping
  error = sem_open(0,1);
  exit_error(error);
  // abrimos un semáforo para el pong
  error = sem_open(1,0); 
  exit_error(error);
```
Usando la syscall fork, intercalamos entre el proceso hijo y padre cada print. Es importante notar que gracias a que usamos dos semáforos es que pudimos controlar la ejecución de cada proceso en cierto momento, logrando imprimir "ping pong" de forma alterna.
En esta parte del código, el proceso hijo disminuye a cero el contador del semáforo 0 y aumenta a 1 el contador del semáforo 1, generando que el proceso hijo se bloquee dejandole lugar al proceso padre.
```c
 // proceso hijo
 if(pid == 0){
    for(unsigned int i = 0; i < N; i++){
      error = sem_down(0);
      exit_error(error);
      
      printf("ping\n");

      error = sem_up(1);
      exit_error(error);
    }

  }
```
En el proceso padre, se siguen los mismos pasos pero de forma opuesta: aumenta en uno el contador cero (dejándolo en valor 1) y disminuye el 1 el contador uno (dejándolo en cero). Por lo tanto, una vez impreso el "pong", el proceso hijo tiene lugar a imprimir nuevamente "ping".

```c

   // proceso padre
   else{
    for(unsigned int i = 0; i < N; i++){
      error = sem_down(1);
      exit_error(error);

      printf("pong\n");

      error = sem_up(0);
      exit_error(error);
    }

    // Cerramos semáforo
    error = sem_close(0); 
    exit_error(error);
    // Cerramos semáforo
    error = sem_close(1);
    exit_error(error);
  }
  
```
 
 Los ciclos for de ambos procesos (hijo y padre) sirven para que se realice cada print N veces, haciendo referencia a la cantidad que pasa el usuario por terminal. Se destaca que en el proceso padre se cierran los semáforos porque "pong" es lo último que se imprime, por lo tanto, el padre es el encargado de cerrarlos.

### **Implementaciones en XV6**


## **Técnicas de Programación**


## **Herramientas de Programación**


## **Desarrollo**


## **Conclusiones**
Al implementar semaforos en XV6, aprendimos sobre condiciones de carrera, locks, mutex y sobre operaciones atómicas y como administrar la memoria en sistemas operativos para evitar sobreescrituras, todo con el objetivo de permitir a los hilos intercambiar información de forma segura. 

Probablemente todo este aprendizaje cobre mucho más sentido cuando lleguemos a la sección de concurrencia en el teórico, pero creemos que este proyecto nos sirvió de alguna forma como introducción a ese tema y nos va a ser más fácil cuando tengamos que verlo desde la teoria.

También aprendimos sobre XV6, como separa sus espacios de kernel y de usuario y como hace la comunicación entre ellos. 

## **Webgrafía**
- https://github.com/mit-pdos/xv6-book
- https://github.com/YehudaShapira/xv6-explained