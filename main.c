#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gamelib.h"
#include "colori.h"

int main(void) {
    char input[16];
    int scelta;
    time_t t;

    srand((unsigned) time(&t));

    do {
        printf(BRED BLINK"\n=== MENU ===\n"RESET);
        printf("1) Imposta gioco\n");
        printf("2) Gioca\n");
        printf("3) Esci\n");
        printf("4) Crediti\n");
        printf("5) Print Giocatori (Solo Debug)\n");
        printf("Scelta: ");

        scanf("%15s", input);

        if (input[0] == '\033') {
            printf("Errore: input non valido (freccia direzionale)\n");
            continue;
        }

        if (strlen(input) != 1 || input[0] < '1' || input[0] > '5') {
            printf("Errore: comando non valido\n");
            continue;
        }

        scelta = input[0] - '0';

        switch (scelta) {
            case 1:
                imposta_gioco();
                break;

            case 2:
                if (!gioco_essere_impostato()) {
                    printf("Devi prima impostare il gioco!\n");
                } else {
                    gioca();
                }
                break;

            case 3:
                printf("Uscita dal gioco.\n");
                break;

            case 4:
                crediti();
                break;

            case 5:
                print_giocatori();
                break;

            default:
                printf("Errore: comando non valido\n");
                break;
        }

    } while (scelta != 3);

    return 0;
}
