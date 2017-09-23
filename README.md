# Tarea 1.2
A continuación se presenta la documentación de la entrega de la tarea 1.2.

---
## Funciones Principales

La función más importante es la función ``main()`` en donde se ejecutan los threads del proceso central o casa matriz del banco. Es aquí donde se generan los procesos de sucursales y manejo de comandos.

Al iniciar el programa, el proceso padre inicia un thread que ejecuta la función ``asyncTransactionBroadcast()`` esta función se encarga de reenviar los mensajes generados por las distintas sucursales hacia el resto. Funciona en segundo plano para no bloquear la posibilidad de ingresar comandos mientras se está haciendo broadcast.

Cuando se crea una nueva sucursal, ocurren tres eventos de interés. En primer lugar se crean dos pipes (uno para comunicación padre -> hijo y otro hijo -> padre). En segundo lugar se crea un thread para que la sucursal pueda generar mensajes que sean reenviados por el padre hacia el resto de sucursales utilizando la función ``asyncPostTransaction()``. Finalmente, se crea un segundo thread que permite a la sucursal escuchar activamente al padre para recibir los mensajes enviados por el resto de sucursales, para ello se utiliza la función ``asyncListenTransactions()``.

Cada sucursal a medida que escucha mensajes del padre, revisa si el paquete está destinado hacia él y de ser así, ejecuta la función ``useMessage()`` para ejecutar las acciones que el mensaje indique.

---
## Problemas Encontrados
A lo largo del desarrollo del sistema de bancos se presentan una serie de dificultades que se explican brevemente a continuación.

- Creación de sucursales: en un principio cuandos se crea una sucursal copia la información existente en el minuto del estado del banco y con ello los pids existentes de otras sucursales. De esta forma, la primera sucursal no conoce otras y no tiene a quien mandarle transacciones. Esto se soluciona enviando un mensaje broadcast anunciando la creacion de una nueva sucursal.
- Kill de sucursal: De forma análoga al problema de creación, al eliminar una sucursal, las otras no se enteran y siguen intentando comunicarse con ella. Se soluciona con un broadcast.
- Procesos y memoria del sistema: Al crear los forks es necesario que tengan la misma información general. Esto se dificulta en el momento que se crea una nueva sucursal. En este caso el banco sabe que existe una nueva y la almacena, pero las sucursales antiguas quedan desactualizadas.
Una forma de solucionar ésto fue mandar la información por los pipes de las sucursales como Broadcast.
- Random: Existen varios datos que se requieren datos aleatorios, como es el caso de los saldos de las cuentas y las transacciones. El comando random de la librería ``<stdlib.h>`` toma en consideración el tiempo actual de la máquina. El problema surge cuando se necesita generar cien valores aleatorios. La máquina es tan veloz que genera el mismo número cien veces.
Las formas utilizadas para solucionar ésto fueron en unos casos usar la función ``sleep()`` para que cambie el valor del reloj. En otros casos modificar en cada iteración el parámetro recibido por el creador de ``rand()`` y así obtener valores distintos en cada ejecución.

- La implementación de la comunicación mediante pipes entre procesos fue bastante desafiante por el manejo de recursos compartidos. Todos fueron resueltos con la excepción del pipe que se utiliza para enviar mensajes desde los comandos ``DUMP`` hacia el broadcast. Reutilizamos el ``pipe 0`` del arreglo y en unas cuantas iteraciones éste se llenaba por lo que las sucursales dejan de responder a estos comandos ya que nunca reciben el mensaje.



---
## Funciones del sistema sin implementar
Logramos una buena cobertura funcionalidades, sin embargo algunas quedaron con bugs o parcialmente implementadas. Entre ellas están:
- Comando DUMP: Si bien el comando genera el archivo por sucursal con datos de ésta, no el contenido del archivo no cumple con lo solicitado. Faltó recolectar información del sistema como la cuenta que hace la petición y otros.
- Comando DUMP_ERRS: Nuevamente el comando genera el archivo pero con otros datos por la misma razón de arriba.
- No fue posible lograr un funcionamiento estable de estas funciones, en algunos casos el comando genera un archivo vacío.
