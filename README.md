# Laboratorio número 2 de Sistemas Operativos 2022 - Grupo 12 | FaMAF UNC

## Integrantes del grupo:

- Lautaro Bachmann (lautaro.bachmann@mi.unc.edu.ar)
- Juan Bratti (juanbratti@mi.unc.edu.ar)
- Gonzalo Canavesio (gonzalo.canavesio@mi.unc.edu.ar)
- Emanuel Herrador (emanuel.nicolas.herrador@unc.edu.ar)


## Índice

 - [Introducción](#markdown-header-introduccion)
 - [¿Cómo correr el código?](#markdown-header-como-correr-el-codigo)
 - [Modularización](#markdown-header-modularizacion)
    - [Syscalls utilizadas](#markdown-header-syscalls-utilizadas)
    - [Implementación semaforo](#markdown-header-implementacion-semaforo)
    - [Implementaciones interesantes](#markdown-header-implementaciones-interesantes)
    - [Implementaciones en XV6](#markdown-header-implementaciones-en-xv6)
 - [Herramientas de Programación](#markdown-header-herramientas-de-programacion)
 - [Desarrollo del proyecto](#markdown-header-desarrollo-del-proyecto)
 - [Conclusiones](#markdown-header-conclusiones)
 - [Webgrafía](#markdown-header-webgrafia)


## Introducción
Implementamos semáforo a través del uso de un arreglo, teniendo así en cada posición un semáforo distinto. Para cada semáforo definimos una estructura con dos campos, uno para guardar el valor y otro para el spinlock, el cual está encargado de regular el acceso correcto a los recursos por parte de los procesos.

Luego, en la función ping pong usamos esta implementación para imprimir por pantalla de forma organizada y alterna la palabra "ping" seguida de "pong"; se utilizó la syscall fork para controlar la ejecución de los procesos con la implementación propuesta de los semáforos.

## ¿Cómo correr el código?
### Instalación
1. Clonar repositorio: `git clone https://bitbucket.org/sistop-famaf/so22lab2g12.git`
2. Instalar qemu (ubuntu): `sudo apt-get install qemu-system-riscv64`
    - Para otras distribuciones ver este [link](https://pdos.csail.mit.edu/6.828/2019/tools.html).

### Compilación y ejecución
#### Compilar y ejecutar XV6
Ejecutar en *so22lab1g12/* el siguiente comando: 
```sh
make qemu
```
#### Ejecutar pingpong
 Para hacer uso de la función pingpong debemos escribir
```sh
pingpong N
```
donde N es un número natural.

## Modularización
Para realizar el trabajo, no nos basamos en una fuerte modularización individual o en subgrupos, más bien trabajamos en las partes necesarias para avanzar en conjunto. Decidimos hacerlo de esta forma debido a la longitud del laboratorio. Esta división de partes fue principalmente en el trabajo sobre las syscalls, implementación del semáforo y las implementaciones en XV6 necesarias.

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

## Implementaciones
### Implementación semáforo

#### Manejo de secciones críticas
En todas aquellas secciones donde se utilizan recursos compartidos, como por ejemplo al modificar el valor del semaforo, se utilizan las funciones **acquire** y **release** para asegurarnos de que no se sobreescriban los datos y logrando así un acceso síncrono a los recursos por parte de los procesos. 

#### Estructuras del semáforo
```c
// Estructura del semáforo
struct sem{
  int value;
  struct spinlock lock;
}; 

// Arreglo de semáforos
struct sem semaphore_counter[MAXCNTSEM];
```
#### sem_open
Para inicializar el semáforo deseado se utiliza sem_open colocando como argumentos el ID del semaforo y el valor inicial del semaforo. El ID del semáforo va a ser el indice del arreglo de semaforos al cual accederemos, y en caso de ya estar siendo utilizado se le informa al usuario mediante un error. Se inicializa el *lock* del semaforo y se lo utiliza para bloquearlo mientras se le asigna al semaforo su valor inicial.

#### sem\_up y sem\_down
Para administrar el uso de recursos entre procesos se usan las funciones **sem_up** y **sem_down**. Dentro de estas funciones se utilizaron las syscalls sleep y wakeup para regular el acceso a los recursos y administrar los procesos dormidos, cumpliendo con la consigna de bloquer los procesos cuando el valor del semaforo es 0 al utilizar **sem_down** y desbloquearlos cuando el valor del semaforo es 0 al utilizar **sem_up**

#### Semaforos bloqueados
Para definir cuando un semaforo esta "bloqueado", asignamos al valor del semaforo el entero -1.

#### Inicializacion del array de semaforos
En el arranque del sistema operativo ejecutamos la función **seminit** que se encarga de inicializar el valor de todos los semaforos en -1. 

#### sem_close
La función **sem_close** despierta todos los procesos que hayan sido mandados a dormir por el semaforo y establece el valor del semaforo en -1.

### Implementacion pingpong
Relacionado a la función ping pong, lo que hicimos fue inicializar dos semáforos, uno para controlar el proceso que imprime por pantalla "ping" y otro para el proceso que imprime "pong".
El semáforo del "ping" se inicializa en 1 y el semáforo del "pong" se inicializa en 0 para luego a través de las funciones sem_up y sem_down intercalar la ejecución de cada proceso.

Usando la syscall fork, intercalamos entre el proceso hijo y padre cada print.

### Implementaciones en XV6
Se modificaron los siguientes archivos:

- user/user.h: se agregaron los prototipos de las 4 llamadas al sistema a implementar.
- user/usys.pl: se agregaron las llamadas a la función entry() para las 4 llamadas al sistema a implementar.
- user/pingpong.c: se creó el archivo pingpong.c para realizar la implementación del programa pingpong pedido.
- kernel/syscall.c y kernel/syscall.h: se agregaron los prototipos de las llamadas al sistema a implementar junto con los números correspondientes a cada una para el correcto mapeo de las funciones.
- kernel/sem.c: se creó el archivo sem.c para la implementación de las llamadas al sistema con el código del grupo.
- kernel/param.h: se declararon las variables globales y el arreglo de semáforos para el posterior uso en la implementación.
- Makefile: se enlazó el ejecutable de pingpong.c y sem.c para la correcta ejecución del programa.

## Herramientas de Programación
Las principales herramientas utilizadas por el grupo en la implementación y división del proyecto fueron las siguientes:

### *Material teórico de estudio y preparación*

 - [**Operating Systems: Three Easy Pieces**: Process virtualization](https://pages.cs.wisc.edu/~remzi/OSTEP/), principalmente el capítulo número 5 (*Process API*) junto con su sección de *Homework Simulation* y *Homework Code*
 - [**Documentación de XV6**](https://course.ccs.neu.edu/cs3650/unix-xv6/index.html)
 - [**Explicaciones sobre el funcionamiento de XV6**](https://github.com/YehudaShapira/xv6-explained)
 - [**Repositorio XV6**](https://github.com/mit-pdos/xv6-book) 
### *Desarrollo*

 - [Visual Studio Code](https://code.visualstudio.com/), editor de código
 - [ClangFormat](https://clang.llvm.org/docs/ClangFormat.html), formateador de código
    - Se generó una configuración basada en la guía de estilo de GNU y se subió al repositorio para que el editor de texto de todos los integrantes del grupo se comporte de forma similar.
### *Compilacion*

- [GNU Make](https://www.gnu.org/software/make/)

### *Debugging*

- [GDB](https://sourceware.org/gdb/), depurador estándar para el compilador GNU.

## Desarrollo del proyecto
### *Comunicación*
La comunicación se basó fuertemente en plataformas como [Discord](https://discord.com/) y [Telegram](https://telegram.org/).

### *Workflow de desarrollo*
#### *Branches*
Nuestro workflow se apoyó fuertemente en el uso de branchs dentro del repositorio de bitbucket.

En cada parte del trabajo, se crearon branchs para: adición de los prototipos a los archivos complementarios de XV6, realización del informe, implementación de las syscalls y testeo de las mismas.

### *Pruebas utilizadas*
Además de los tests brindados por la cátedra, se realizaron pruebas caseras. Uno de los tests realizados fue pingpongpung (mismo comportamiento que pingpong pero con 3 hilos).

## Conclusiones
Al implementar semaforos en XV6, aprendimos sobre condiciones de carrera, locks, mutex y sobre operaciones atómicas y como administrar la memoria en sistemas operativos para evitar sobreescrituras, todo con el objetivo de permitir a los hilos intercambiar información de forma segura. 

Probablemente todo este aprendizaje cobre mucho más sentido cuando lleguemos a la sección de concurrencia en el teórico, pero creemos que este proyecto nos sirvió de alguna forma como introducción a ese tema y nos va a ser más fácil cuando tengamos que verlo desde la teoría.

También aprendimos sobre XV6, como separa sus espacios de kernel y de usuario y como hace la comunicación entre ellos. 

## Webgrafía
- https://github.com/mit-pdos/xv6-book
- https://github.com/YehudaShapira/xv6-explained
