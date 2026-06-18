# memsim — Simulador de Políticas de Asignación de Memoria

Simulador por consola que modela un gestor de memoria con particiones dinámicas.
Soporta tres políticas de asignación: **First-Fit**, **Best-Fit** y **Worst-Fit**.

## Compilación

```bash
gcc -Wall -Wextra -std=c17 -o memsim memsim.c
```

O usando el Makefile:

```bash
make
```

## Uso

```
./memsim <tamaño_bytes> <política> <archivo_traza>
```

| Parámetro       | Descripción                                      |
|-----------------|--------------------------------------------------|
| `tamaño_bytes`  | Tamaño total de la memoria simulada en bytes     |
| `política`      | `FIRST_FIT`, `BEST_FIT` o `WORST_FIT`           |
| `archivo_traza` | Ruta al archivo de texto con las operaciones     |

## Formato del Archivo de Traza

```
ALLOC <ID_Proceso> <SIZE>   # Asigna SIZE bytes al proceso ID_Proceso
FREE <ID_Proceso>            # Libera toda la memoria del proceso
COMPACT                      # Compacta la memoria eliminando fragmentación
```

## Ejemplo de Ejecución

**Archivo de traza (`traza1.txt`):**
```
ALLOC P1 200
ALLOC P2 300
ALLOC P3 100
FREE P2
ALLOC P4 150
FREE P1
COMPACT
ALLOC P5 400
```

**Comando:**
```bash
./memsim 1024 FIRST_FIT traza1.txt
```

**Salida esperada:**
```
Procesos asignados: 5
Memoria utilizada: 63.48%
Indice de Fragmentacion Externa: 0.0000
Estado final de la memoria:
[Ocupado P4 150] -> [Ocupado P3 100] -> [Ocupado P5 400] -> [Libre 374]
```

## Comportamiento

- **ALLOC exitoso:** El bloque candidato se divide si hay espacio sobrante.
- **FREE + Coalescencia:** Al liberar, se fusionan automáticamente los bloques libres adyacentes.
- **COMPACT:** Todos los bloques ocupados se mueven al inicio de la memoria; el espacio libre queda en un único bloque al final.
- **Fallo de ALLOC:** Si no hay bloque libre suficiente, se imprime el reporte final y el programa termina.

## Casos de Prueba Incluidos

| Archivo         | Descripción                                             |
|-----------------|---------------------------------------------------------|
| `traza1.txt`    | Flujo normal: alloc, free, compact, alloc posterior     |
| `traza_borde.txt` | Fragmentación extrema que requiere compactación       |
| `traza_frag.txt` | Tres huecos de tamaños distintos (250, 200, 400); cada política elige uno diferente, lo que produce índices de fragmentación distintos. Termina con un ALLOC que falla por fragmentación externa aunque hay memoria libre suficiente en total. |
| `traza_frag_compact.txt` | Igual al anterior pero con un `COMPACT` antes del ALLOC final, demostrando que la compactación revierte la falla. |
| `traza_multi_pid.txt` | Un mismo PID solicita memoria dos veces sin liberar entre medio; `FREE` debe liberar ambos bloques. |
| `traza_size_invalido.txt` | Una línea `ALLOC` con tamaño negativo; el simulador debe descartarla con una advertencia en vez de corromper la contabilidad. |
