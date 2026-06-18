#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Block {
    int start;           /* dirección de inicio (bytes) */
    int size;            /* tamaño del bloque (bytes)   */
    int is_free;         /* 1 = libre, 0 = ocupado      */
    char pid[32];        /* ID del proceso (si ocupado) */
    struct Block *next;
} Block;

static Block *head     = NULL;
static int    MEM_SIZE = 0;
static int alloc_count = 0;

static Block *new_block(int start, int size, int is_free, const char *pid) {
    Block *b = (Block *)malloc(sizeof(Block));
    if (!b) { fprintf(stderr, "Error: malloc falló\n"); exit(1); }
    b->start   = start;
    b->size    = size;
    b->is_free = is_free;
    b->next    = NULL;
    strncpy(b->pid, pid ? pid : "", sizeof(b->pid) - 1);
    b->pid[sizeof(b->pid) - 1] = '\0';
    return b;
}

static void assign_block(Block *blk, int size, const char *pid) {
    int leftover = blk->size - size;

    blk->is_free = 0;
    blk->size    = size;
    strncpy(blk->pid, pid, sizeof(blk->pid) - 1);
    blk->pid[sizeof(blk->pid) - 1] = '\0';

    if (leftover > 0) {
        Block *free_blk = new_block(blk->start + size, leftover, 1, NULL);
        free_blk->next  = blk->next;
        blk->next       = free_blk;
    }
}

static int alloc_first_fit(const char *pid, int size) {
    Block *cur = head;
    while (cur) {
        if (cur->is_free && cur->size >= size) {
            assign_block(cur, size, pid);
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}

static int alloc_best_fit(const char *pid, int size) {
    Block *best = NULL;
    Block *cur  = head;
    while (cur) {
        if (cur->is_free && cur->size >= size) {
            if (!best || cur->size < best->size)
                best = cur;
        }
        cur = cur->next;
    }
    if (!best) return 0;
    assign_block(best, size, pid);
    return 1;
}

static int alloc_worst_fit(const char *pid, int size) {
    Block *worst = NULL;
    Block *cur   = head;
    while (cur) {
        if (cur->is_free && cur->size >= size) {
            if (!worst || cur->size > worst->size)
                worst = cur;
        }
        cur = cur->next;
    }
    if (!worst) return 0;
    assign_block(worst, size, pid);
    return 1;
}

static void free_process(const char *pid) {
    /* Recorre TODA la lista liberando cada bloque que pertenezca al proceso,
       ya que un mismo PID puede tener mas de un bloque asignado si hizo
       varios ALLOC sin liberar entre medio. */
    Block *prev  = NULL;
    Block *cur   = head;
    int    found = 0;

    while (cur) {
        Block *after = cur->next; /* siguiente nodo a visitar, guardado antes de mutar la lista */

        if (!cur->is_free && strcmp(cur->pid, pid) == 0) {
            found = 1;
            cur->is_free = 1;
            cur->pid[0]  = '\0';

            /* Coalescencia con el bloque siguiente */
            if (cur->next && cur->next->is_free) {
                Block *nxt = cur->next;
                cur->size += nxt->size;
                cur->next  = nxt->next;
                if (after == nxt) after = cur->next; /* 'after' apuntaba a nxt, que se libera */
                free(nxt);
            }

            /* Coalescencia con el bloque anterior */
            if (prev && prev->is_free) {
                prev->size += cur->size;
                prev->next  = cur->next;
                free(cur);
                cur = prev; /* el bloque fusionado pasa a ser 'cur' para que prev quede consistente */
            }
        }

        prev = cur;
        cur  = after;
    }

    if (!found) {
        fprintf(stderr, "Advertencia: proceso %s no encontrado para liberar\n", pid);
    }
}


static void compact(void) {
    int occupied_end = 0; 
    Block *cur = head;

    while (cur) {
        if (!cur->is_free) {
            cur->start  = occupied_end;
            occupied_end += cur->size;
        }
        cur = cur->next;
    }

    Block *prev_node = NULL;
    cur = head;
    while (cur) {
        if (cur->is_free) {
            Block *to_del = cur;
            if (prev_node)
                prev_node->next = cur->next;
            else
                head = cur->next;
            cur = cur->next;
            free(to_del);
        } else {
            prev_node = cur;
            cur = cur->next;
        }
    }

    int free_space = MEM_SIZE - occupied_end;
    if (free_space > 0) {
        Block *free_blk = new_block(occupied_end, free_space, 1, NULL);
        if (!head) {
            head = free_blk;
        } else {
            Block *tail = head;
            while (tail->next) tail = tail->next;
            tail->next = free_blk;
        }
    }
}

/*Reporte Final*/

static void print_report(void) {
    int used_bytes = 0;
    int free_bytes = 0;
    int bmax       = 0; /* bloque libre más grande */

    Block *cur = head;
    while (cur) {
        if (cur->is_free) {
            free_bytes += cur->size;
            if (cur->size > bmax) bmax = cur->size;
        } else {
            used_bytes += cur->size;
        }
        cur = cur->next;
    }

    double used_pct = (MEM_SIZE > 0) ? (100.0 * used_bytes / MEM_SIZE) : 0.0;

    /* Fragmentación externa: F = 1 - Bmax/Mlibre  (0 si no hay bloques libres) */
    double frag = 0.0;
    if (free_bytes > 0)
        frag = 1.0 - ((double)bmax / (double)free_bytes);

    printf("Procesos asignados: %d\n", alloc_count);
    printf("Memoria utilizada: %.2f%%\n", used_pct);
    printf("Indice de Fragmentacion Externa: %.4f\n", frag);
    printf("Estado final de la memoria:\n");

    cur = head;
    int first = 1;
    while (cur) {
        if (!first) printf(" -> ");
        if (cur->is_free)
            printf("[Libre %d]", cur->size);
        else
            printf("[Ocupado %s %d]", cur->pid, cur->size);
        first = 0;
        cur   = cur->next;
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <tamaño> <FIRST_FIT|BEST_FIT|WORST_FIT> <archivo_traza>\n", argv[0]);
        return 1;
    }

    MEM_SIZE = atoi(argv[1]);
    if (MEM_SIZE <= 0) {
        fprintf(stderr, "Error: tamaño de memoria inválido\n");
        return 1;
    }

    int policy = -1;
    if (strcmp(argv[2], "FIRST_FIT") == 0) policy = 0;
    else if (strcmp(argv[2], "BEST_FIT")  == 0) policy = 1;
    else if (strcmp(argv[2], "WORST_FIT") == 0) policy = 2;
    else {
        fprintf(stderr, "Error: política '%s' desconocida\n", argv[2]);
        return 1;
    }

    FILE *fp = fopen(argv[3], "r");
    if (!fp) {
        fprintf(stderr, "Error: no se puede abrir '%s'\n", argv[3]);
        return 1;
    }

    head = new_block(0, MEM_SIZE, 1, NULL);

    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0') continue;

        char op[16];
        if (sscanf(line, "%15s", op) != 1) continue;

        if (strcmp(op, "ALLOC") == 0) {
            char pid[32];
            int  size;
            if (sscanf(line, "%*s %31s %d", pid, &size) != 2) {
                fprintf(stderr, "Advertencia: línea ALLOC mal formateada: %s\n", line);
                continue;
            }

            if (size <= 0) {
                fprintf(stderr, "Advertencia: tamaño invalido en ALLOC %s (%d): se ignora la linea\n", pid, size);
                continue;
            }

            int ok = 0;
            if      (policy == 0) ok = alloc_first_fit(pid, size);
            else if (policy == 1) ok = alloc_best_fit(pid, size);
            else                  ok = alloc_worst_fit(pid, size);

            if (ok) {
                alloc_count++;
            } else {
                fprintf(stderr, "ALLOC fallido para %s (%d bytes): fragmentacion externa\n", pid, size);
                break;
            }

        } else if (strcmp(op, "FREE") == 0) {
            char pid[32];
            if (sscanf(line, "%*s %31s", pid) != 1) {
                fprintf(stderr, "Advertencia: línea FREE mal formateada: %s\n", line);
                continue;
            }
            free_process(pid);

        } else if (strcmp(op, "COMPACT") == 0) {
            compact();

        } else {
            fprintf(stderr, "Advertencia: operación desconocida '%s'\n", op);
        }
    }

    fclose(fp);
    print_report();

    Block *cur = head;
    while (cur) {
        Block *tmp = cur->next;
        free(cur);
        cur = tmp;
    }

    return 0;
}
