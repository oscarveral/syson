/* --- Boletín 1. Ejercicio 1. --- */

#include "types.h"
#include "user.h"
#include "date.h"

int
main(int argc, char **argv)
{
    struct rtcdate r;

    if (date(&r) < 0) {
        printf(2, "Fallo de la llamada al sistema \"date\"\n");
        exit(EXIT_FAILURE);
    }
    
    printf(1, "Fecha actual %d/%d/%d %d:%d:%d\n", r.day, r.month, r.year, r.hour, r.minute, r.second);

    exit(EXIT_SUCCESS);
}