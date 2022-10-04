# Laboratorio número 2 de Sistemas Operativos 2022 - Grupo 12 | FaMAF UNC

## Integrantes del grupo:

- Lautaro Bachmann (lautaro.bachmann@mi.unc.edu.ar)
- Juan Bratti (juanbratti@mi.unc.edu.ar)
- Gonzalo Canavesio (gonzalo.canavesio@mi.unc.edu.ar)
- Emanuel Herrador (emanuel.nicolas.herrador@unc.edu.ar)


# Índice

[TOC]

# Introducción
Luego de ver en la primera parte del semestre la virtualización de los procesos y de la memoria, comenzamos a pensar acerca de los multiprogramas y los hilos. Comenzamos a ver más allá de un programa que ejecute un solo proceso, pasando a varios hilos en forma simultánea (lo cual permite gran eficiencia). Sin embargo, toda optimización tiene un costo y, en este caso, son los _problemas de concurrencia_.
Por ello mismo y como consecuencia de lo mencionado, en este laboratorio vamos a trabajar una de las formas de solucionar este conflicto: _la implementación de semáforos con spinlocks_.

# ¿Cómo correr el código?
## Instalación
1. Clonar repositorio: `git clone https://bitbucket.org/sistop-famaf/so22lab2g12.git`
2. Instalar qemu (ubuntu): `sudo apt-get install qemu-system-riscv64`
    - Para otras distribuciones ver este [link](https://pdos.csail.mit.edu/6.828/2019/tools.html).

## Compilación y ejecución
### Compilar y ejecutar XV6
Ejecutar en *so22lab1g12/* el siguiente comando: 
```sh
make qemu
```
### Ejecutar pingpong
 Para hacer uso de la función pingpong debemos escribir
```sh
pingpong N
```
donde N es un número natural.

# Implementación del proyecto

## Syscalls utilizadas
### Introducción al funcionamiento de *acquire* y *release*
Las funciones **acquire** y **release** protegen secciones compartidas entre varios procesos, para evitar que se sobreescriban datos por error.

Para evitar deadlocks, se deshabilitan las interrupciones al ejecutar **acquire()** y se rehabilitan al ejecutar **release()**, es decir, no se interrumpe la ejecución en la CPU del proceso mientras se está ejecutando la sección crítica del código. 

### acquire(*struct spinlock \*lk*)
Primero deshabilita las interrupciones del procesador.

Luego toma un puntero a un spinlock *lk*, revisa que no este bloqueado ya por la misma CPU que la esta llamando (en ese caso da un error) y ejecuta un bucle del que se sale solo cuando se desbloquea el spinlock. 

En ese ciclo se realiza un swap atómico (en una sola instrucción) de *&lk->locked*, escribiendole un 1 y guardando el valor que tenia anteriormente ese espacio de memoria, si el valor anterior era 0 (el spinlock estaba desbloqueado) termina el ciclo y si era diferente de 0 (Un "1") se sigue ejecutando hasta que se libere el spinlock.

Una vez ya se libero el spinlock, se guarda en *lk->cpu* un puntero a la estructura de la CPU que esta ejecutando esta función

### release(*struct spinlock \*lk*)
Toma un puntero a un spinlock *lk*, revisa que no esté desbloqueado ya (en ese caso da un error) y si no lo esta, desbloquea el spinlock *lk*, asignando 0 a *lk->cpu* (porque ninguna CPU está bloqueando el spinlock) y asignando 0 a *&lk->locked*

Luego habilita nuevamente las interrupciones del procesador.

### sleep(*void \*chan, struct spinlock \*lk*)
Bloquea la tabla del proceso que ejecutó la función, libera el spinlock *lk*, asigna en *p->chan* el argumento  *chan* y devuelve el control del CPU hasta que el proceso es despertado por **wakeup**.

Una vez es despertado, modifica el valor *p->chan* a, desbloquea la tabla del proceso y vuelve a bloquear el spinlock *lk* (Volviendo el spinlock al estado inicial antes de que fuera llamado el sleep)

### wakeup(*void \*chan*)
Revisa todos los procesos excepto el suyo, bloqueando la tabla del proceso y revisando si el proceso está dormido y esperando por *chan*, si se cumple esa condición entonces despierta al proceso (Cambia su estado de *durmiendo* a *listo para ejecutarse*)

Una vez deja de revisar la tabla del proceso, la libera.

### argint(*int n, int \*ip*)
Obtiene el argumento *n*-ésimo insertado en la pila de usuario por el código de usuario antes de que el usuario solicite una llamada al sistema y lo escribe en *ip*.

## Implementación semáforo

### Manejo de secciones críticas
En todas aquellas secciones donde se utilizan recursos compartidos, como por ejemplo al modificar el valor del semaforo, se utilizan las funciones **acquire** y **release** para asegurarnos de que no se sobreescriban los datos y logrando así un acceso síncrono a los recursos por parte de los procesos. 

### Estructuras del semáforo
```c
// Estructura del semáforo
struct sem{
  int value;
  struct spinlock lock;
}; 

// Arreglo de semáforos
struct sem semaphore_counter[MAXCNTSEM];
```
### sem_open
Para inicializar el semáforo deseado se utiliza sem_open colocando como argumentos el ID del semaforo y el valor inicial del semaforo. El ID del semáforo va a ser el indice del arreglo de semaforos al cual accederemos, y en caso de ya estar siendo utilizado se le informa al usuario mediante un error. Se inicializa el *lock* del semaforo y se lo utiliza para bloquearlo mientras se le asigna al semaforo su valor inicial.

### sem\_up y sem\_down
Para administrar el uso de recursos entre procesos se usan las funciones **sem_up** y **sem_down**. Dentro de estas funciones se utilizaron las syscalls sleep y wakeup para regular el acceso a los recursos y administrar los procesos dormidos, cumpliendo con la consigna de bloquear los procesos cuando el valor del semaforo es 0 al utilizar **sem_down** y desbloquearlos cuando el valor del semaforo es 0 al utilizar **sem_up**

### Semaforos libres para su uso
Para definir cuando un semaforo esta "libre", asignamos al valor de este al entero -1.

### Inicializacion del array de semaforos
En el arranque del sistema operativo ejecutamos la función **seminit** que se encarga de inicializar el valor de todos los semaforos en -1. 

### sem_close
La función **sem_close** despierta todos los procesos que hayan sido mandados a dormir por el semáforo y establece su valor en -1.

## Implementacion pingpong
Lo que hicimos fue inicializar dos semáforos, uno para controlar el proceso que imprime por pantalla "ping" y otro para el proceso que imprime "pong".
El semáforo del "ping" se inicializa en 1 y el semáforo del "pong" se inicializa en 0 para luego a través de las funciones sem_up y sem_down se intercale la ejecución de cada proceso.

En definitiva, usando la syscall fork, creamos el hijo y luego intercalamos entre el proceso hijo y padre cada print.

## Implementaciones en XV6
### Kernel
- **kernel/syscall.c y kernel/syscall.h:** se agregaron los prototipos de las llamadas al sistema a implementar junto con los números correspondientes a cada una para el correcto mapeo de las funciones.
- **kernel/sem.c:** se creó el archivo sem.c, en el mismo se implementaron las llamadas al sistemas pedidas.
- **kernel/printf.c:** implementar funcion para convertir un entero no signado en un string
- **kernel/string.c:** implementar funcion para concatenar strings
- **kernel/param.h:** se declararon las variables globales y el arreglo de semáforos para el posterior uso en la implementación.
- **kernel/defs.h:** Añadir prototipos para las siguientes funciones:
    - `uint_to_str`
    - `seminit`
    - `strcat`

### User
- **user/user.h:** se agregaron los prototipos de las 4 llamadas al sistema a implementar.
- **user/usys.pl:** se agregaron las llamadas a la función entry() para las 4 llamadas al sistema a implementar.
- **user/pingpong.c:** se creó el archivo pingpong.c para realizar la implementación del programa pingpong pedido.
- **user/pingpongpung.c:** variacion de pingpong que usa 3 hilos para testear si nuestra implementacion del semaforo es correcta.

### Otros
- **Makefile:** se enlazó el ejecutable de pingpong.c, pingpongpung.c y sem.c para la correcta ejecución del programa. Ademas se cambio el nombre del README de XV6 a README-XV6.

# Herramientas de Programación
Las principales herramientas utilizadas por el grupo en la implementación y división del proyecto fueron las siguientes:

## *Material teórico de estudio y preparación*

 - [**Operating Systems: Three Easy Pieces**: Process virtualization](https://pages.cs.wisc.edu/~remzi/OSTEP/), principalmente el capítulo número 5 (*Process API*) y los capitulos de la sección de Concurrencia, sobre todo el capítulo número 31 (*Semaphores*), junto con las secciones de *Homework Simulation* y *Homework Code* de cada uno de esos capitulos.
 - [**Documentación de XV6**](https://course.ccs.neu.edu/cs3650/unix-xv6/index.html)
 - [**Explicaciones sobre el funcionamiento de XV6**](https://github.com/YehudaShapira/xv6-explained)
 - [**Repositorio XV6**](https://github.com/mit-pdos/xv6-book) 

### Conceptos teóricos utilizados
- **Race conditions:** Esto se da cuando un resultado depende de procesos que se ejecutan en un orden arbitrario y trabajan sobre el mismo recurso compartido, los procesos corren una "carrera" para acceder al recurso compartido y se puede producir un error cuando dichos procesos no llegan (se ejecutan) en el orden que se esperaba.
- **Locks:** Son un mecanismo de sincronización que limita el acceso a un recurso compartido por varios procesos o hilos en un ambiente de ejecución concurrente, permitiendo así la exclusión mutua. Cada proceso/hilo para tener acceso a un elemento del conjunto, deberá bloquear, con lo que se convierte en su dueño, esa es la única forma de ganar acceso. Al terminar de usarlo, el dueño debe desbloquear, para permitir que otro proceso/hilo pueda tomarlo a su vez.
- **Mutex:** (exclusión mutua) Es el requisito de que un hilo de ejecución nunca entre en una sección crítica mientras un hilo de ejecución concurrente ya está accediendo a dicha sección crítica. Se crea para prevenir condiciones de carrera. 
- **Deadlocks:** Es cualquier situación en la cual ningún proceso puede proceder con su ejecución porque cada uno de los procesos del grupo está esperando que otro tome la acción.
- **Zona crítica:** Porción de código en la que se accede a un recurso compartido que no debe ser accedido por más de un proceso o hilo en ejecución. Se necesita un mecanismo de sincronización en la entrada y salida de la sección crítica para asegurar la utilización en exclusiva del recurso.
- **Semáforo:** Es una variable (o TAD) que se utiliza para administrar el acceso a un recurso compartido en un entorno de multiprocesamiento y evitar problemas en las secciones críticas del sistema.

## *Desarrollo*

 - [Visual Studio Code](https://code.visualstudio.com/), editor de código

## *Compilación*

- [GNU Make](https://www.gnu.org/software/make/)

## *Debugging*

- [GDB](https://sourceware.org/gdb/), depurador estándar para el compilador GNU.

# Desarrollo del proyecto
Comenzamos haciendo una investigación sobre el funcionamiento de XV6 y conceptos como race conditions, el funcionamiento de los locks y de los semáforos. Primero realizamos los prototipos de las funciones en XV6 y luego desarrollamos gran parte de la implementación del semáforo todos juntos durante el laboratorio. Posteriormente, corregimos detalles y agregando nuevas funcionalidades. El proyecto en general, al ser tan corto, no tuvo mucha separación u organización interna en subgrupos, más bien trabajamos en las partes necesarias para avanzar en conjunto. Decidimos hacerlo de esta forma debido a su longitud.

## Problemas
Con el objetivo de corroborar el correcto funcionamiento de los semáforos respecto a las race conditions, se tuvo la idea de implementar una función denominada prodsumadiv, la cual recibia 4 argumentos.

Siendo *arg_i* el argumento *i* de la función, lo que hacía era *arg_4* veces la operación `x = (x * arg_1 + arg_2)/arg_3`, usando 3 procesos, uno por argumento.

El problema fue que al crear los procesos con **fork** no vimos forma de compartir memoria entre padres e hijos (se genera una copia de la misma para cada proceso). Funciones como **mmap** resuelven este problema, pero no están implementadas en XV6. También se podrían crear las syscalls y el ambiente en XV6 para los threads y utilizarlos en vez de usar **fork**, pero consideramos que no es el objetivo del presente laboratorio.

## *Comunicación*
La comunicación se basó fuertemente en plataformas como [Discord](https://discord.com/), donde la comunicación es más organizada y se pueden hacer llamadas de voz, y [Telegram](https://telegram.org/), donde conseguimos una comunicación más veloz e informal. 

## *Workflow de desarrollo*
### *Branches*
Nuestro workflow se apoyó fuertemente en el uso de branchs dentro del repositorio de bitbucket.

En cada parte del trabajo, se crearon branchs para: adición de los prototipos a los archivos complementarios de XV6, realización del informe, implementación de las syscalls y testeo de las mismas.

## *Pruebas utilizadas*
Además de los tests pedidos por la cátedra (pingpong.c), se realizaron pruebas caseras. Uno de los tests realizados fue pingpongpung (mismo comportamiento que pingpong pero con 3 hilos).

# Conclusiones
Al implementar semaforos en XV6, aprendimos sobre condiciones de carrera, locks, mutex y sobre operaciones atómicas y como administrar la memoria en sistemas operativos para evitar sobreescrituras, todo con el objetivo de permitir a los hilos intercambiar información de forma segura. 

Todo este aprendizaje, junto con el conocimiento de la estructura del SO XV6, nos ha ayudado para poder entender y comprender de mejor modo los conceptos que en posteriores clases comenzaremos a ver desde la parte teórica de la materia. Nos ha sido provechoso para poder tener más conocimiento de cómo funciona un sistema operativo, cómo se estructura, cuáles son sus partes fundamentales y cuáles son algunos de los problemas que hay a la hora de implementarlo.