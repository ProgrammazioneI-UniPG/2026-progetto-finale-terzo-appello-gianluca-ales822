#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "gamelib.h"
#include "colori.h"

#define MAX_GIOCATORI 4
#define AZIONI_PER_TURNO 2
#define NUM_ZONE 15
#define MAX_VINCITORI 3
static char ultimi_vincitori[MAX_VINCITORI][50] = {{0}};
static int indice_vincitore = 0;
static int partita_finita = 0;

/*
0 = partita in corso
1 = sconfitta
2 = vittoria
*/


/* ===== VARIABILI GLOBALI ===== */

static const char *nome_nemico(Tipo_nemico n);
static void avvisa_nemico_stanza(Giocatore *g);
static void inserisci_zona(int pos);
static void stampa_mappa(void);

/* ===== PROTOTIPI STATIC ===== */
static void combatti(Giocatore *g);
void raccogli_oggetto(Giocatore *g);
void utilizza_oggetto(Giocatore *g, int indice);
static void avvisa_oggetto_stanza(Giocatore *g);
static const char *nome_oggetto(Tipo_oggetto o);
static void mostra_zaino(Giocatore *g);



static Giocatore *giocatori[MAX_GIOCATORI];
static int num_giocatori = 0;
static int gioco_impostato = 0;
static int mappa_chiusa = 0;  // 0 = mappa non chiusa, 1 = mappa chiusa
static int demotorzone_vivo = 1;

static Zona_mondoreale *prima_zona_mondoreale = NULL;
static Zona_soprasotto *prima_zona_soprasotto = NULL;

/* ===== PROTOTIPI STATIC ===== */

static void crea_mappa(void);
static Tipo_zona genera_zona_casuale(void) {
    return rand() % 5; /* bosco, scuola, laboratorio, caverna, strada */
}

static Tipo_oggetto genera_oggetto_casuale(void) {
    if (rand() % 100 < 40) /* 40% oggetto */
        return rand() % 4 + 1;
    return nessun_oggetto;
}

static Tipo_nemico genera_nemico_mr(void) {
    int r = rand() % 100;
    if (r < 40) return nessun_nemico;
    if (r < 70) return democane;
    return billi;
}

static Tipo_nemico genera_nemico_ss(void) {
    int r = rand() % 100;
    if (r < 50) return nessun_nemico;
    return democane;
}

static void libera_mappa(void);
static void libera_giocatori(void);
static void reset_azioni_giocatori(void);
static void stabilisci_ordine_giocatori(void);

static void vai_avanti(Giocatore *g);
static void vai_indietro(Giocatore *g);
static void cambia_mondo(Giocatore *g);
static void dove_si_trova(Giocatore *g);

static int lancia_d20(void);
static int lancia_d4(void);

/* ===== CREAZIONE ZONE ===== */

static Zona_mondoreale *crea_zona_mr(Tipo_zona t) {
    Zona_mondoreale *z = malloc(sizeof(Zona_mondoreale));
    z->tipo = t;
    z->nemico = nessun_nemico;
    z->oggetto = nessun_oggetto;
    z->avanti = z->indietro = NULL;
    z->link_soprasotto = NULL;
    return z;
}

static Zona_soprasotto *crea_zona_ss(Tipo_zona t) {
    Zona_soprasotto *z = malloc(sizeof(Zona_soprasotto));
    z->tipo = t;
    z->nemico = nessun_nemico;
    z->avanti = z->indietro = NULL;
    z->link_mondoreale = NULL;
    return z;
}

/* ===== MAPPA ===== */

static void crea_mappa(void) {
    Zona_mondoreale *mr_prec = NULL;
    Zona_soprasotto *ss_prec = NULL;

    Zona_mondoreale *mr_array[NUM_ZONE];
    Zona_soprasotto *ss_array[NUM_ZONE];

    /* CREAZIONE ZONE */
    for (int i = 0; i < NUM_ZONE; i++) {
        Tipo_zona t = genera_zona_casuale();

        mr_array[i] = crea_zona_mr(t);
        ss_array[i] = crea_zona_ss(t);

        if (i == 0) {
            prima_zona_mondoreale = mr_array[i];
            prima_zona_soprasotto = ss_array[i];
        } else {
            mr_prec->avanti = mr_array[i];
            mr_array[i]->indietro = mr_prec;

            ss_prec->avanti = ss_array[i];
            ss_array[i]->indietro = ss_prec;
        }

        mr_array[i]->link_soprasotto = ss_array[i];
        ss_array[i]->link_mondoreale = mr_array[i];

        mr_prec = mr_array[i];
        ss_prec = ss_array[i];
    }

    /* ASSEGNA NEMICI E OGGETTI */
    for (int i = 0; i < NUM_ZONE; i++) {
        mr_array[i]->nemico = genera_nemico_mr();
        mr_array[i]->oggetto = genera_oggetto_casuale();

        ss_array[i]->nemico = genera_nemico_ss();
    }

    /* FORZA UN DEMOTORZONE NEL SOPRASOTTO */
    int idx = rand() % NUM_ZONE;
    ss_array[idx]->nemico = demotorzone;
}

    /* INSERISCI ZONA A PIACERE */
    static void inserisci_zona(int pos) {
    if (pos < 0 || pos >= NUM_ZONE) {
        printf("Posizione non valida\n");
        return;
    }

    Tipo_zona t = genera_zona_casuale();

    Zona_mondoreale *nuova_mr = crea_zona_mr(t);
    Zona_soprasotto *nuova_ss = crea_zona_ss(t);

    /* ===== SCELTA NEMICO MR ===== */
    printf("Nemico Mondo Reale (0=nessuno, 1=democane, 2=billi): ");
    scanf("%d", (int *)&nuova_mr->nemico);
    if (nuova_mr->nemico < nessun_nemico || nuova_mr->nemico > billi)
        nuova_mr->nemico = nessun_nemico;

    /* ===== SCELTA OGGETTO MR ===== */
    printf("Oggetto MR (0=nessuno, 1=bicicletta, 2=maglietta, 3=bussola, 4=schitarrata): ");
    scanf("%d", (int *)&nuova_mr->oggetto);
    if (nuova_mr->oggetto < nessun_oggetto || nuova_mr->oggetto > schitarrata_metallica)
        nuova_mr->oggetto = nessun_oggetto;

    /* ===== SCELTA NEMICO SS ===== */
    printf("Nemico Soprasotto (0=nessuno, 1=democane, 2=demotorzone): ");
    scanf("%d", (int *)&nuova_ss->nemico);
    if (nuova_ss->nemico < nessun_nemico || nuova_ss->nemico > demotorzone)
        nuova_ss->nemico = nessun_nemico;

    /* ===== INSERIMENTO LISTA ===== */
    Zona_mondoreale *mr = prima_zona_mondoreale;
    Zona_soprasotto *ss = prima_zona_soprasotto;

    for (int i = 0; i < pos - 1 && mr && ss; i++) {
        mr = mr->avanti;
        ss = ss->avanti;
    }

    nuova_mr->avanti = mr->avanti;
    if (mr->avanti) mr->avanti->indietro = nuova_mr;
    mr->avanti = nuova_mr;
    nuova_mr->indietro = mr;

    nuova_ss->avanti = ss->avanti;
    if (ss->avanti) ss->avanti->indietro = nuova_ss;
    ss->avanti = nuova_ss;
    nuova_ss->indietro = ss;

    nuova_mr->link_soprasotto = nuova_ss;
    nuova_ss->link_mondoreale = nuova_mr;

    printf("Zona inserita correttamente in posizione %d\n", pos);
}

static void cancella_zona(void) {
    int pos;
    printf("Posizione da cancellare: ");
    scanf("%d", &pos);

    Zona_mondoreale *mr = prima_zona_mondoreale;
    Zona_soprasotto *ss = prima_zona_soprasotto;

    for (int i = 0; i < pos && mr; i++) {
        mr = mr->avanti;
        ss = ss->avanti;
    }

    if (!mr || !ss) return;

    if (mr->indietro) mr->indietro->avanti = mr->avanti;
    if (mr->avanti) mr->avanti->indietro = mr->indietro;
    if (mr == prima_zona_mondoreale) prima_zona_mondoreale = mr->avanti;

    if (ss->indietro) ss->indietro->avanti = ss->avanti;
    if (ss->avanti) ss->avanti->indietro = ss->indietro;
    if (ss == prima_zona_soprasotto) prima_zona_soprasotto = ss->avanti;

    free(mr);
    free(ss);
}

void chiudi_mappa(void) {
    // Controllo numero zone MR
    int count = 0;
    Zona_mondoreale *mr = prima_zona_mondoreale;
    while (mr) {
        count++;
        mr = mr->avanti;
    }

    if (count < NUM_ZONE) {
        printf("Non puoi chiudere la mappa: meno di %d zone presenti.\n", NUM_ZONE);
        return;
    }

    // Controllo presenza esatta di un solo Demotorzone
    int demotorzone_count = 0;
    Zona_soprasotto *ss = prima_zona_soprasotto;
    while (ss) {
        if (ss->nemico == demotorzone) demotorzone_count++;
        ss = ss->avanti;
    }

    if (demotorzone_count != 1) {
        printf("Non puoi chiudere la mappa: deve esserci un solo Demotorzone (trovati %d).\n", demotorzone_count);
        return;
    }

    mappa_chiusa = 1;
    printf("Mappa chiusa correttamente. Il gioco può iniziare.\n");
}




/* ===== NEMICI CASUALI ===== */

static Tipo_nemico genera_nemico_mondoreale(void) {
    if (rand() % 2 == 0)
        return nessun_nemico;

    int r = rand() % 2;
    if (r == 0) return billi;
    return democane;
}

static Tipo_nemico genera_nemico_soprasotto(void) {
    if (rand() % 2 == 0)
        return nessun_nemico;

    int r = rand() % 3;
    if (r == 0) return billi;
    if (r == 1) return democane;
    return demotorzone;
}

static void assegna_nemici_mappa(void) {

    /* ===== MONDO REALE: NO DEMOTORZONE ===== */
    Zona_mondoreale *mr = prima_zona_mondoreale;
    while (mr) {
        mr->nemico = genera_nemico_mondoreale();
        mr = mr->avanti;
    }

    /* ===== SOPRASOTTO: UN SOLO DEMOTORZONE ===== */
    Zona_soprasotto *ss = prima_zona_soprasotto;
    int demotorzone_piazzato = 0;

    while (ss) {

        Tipo_nemico n = genera_nemico_soprasotto();

        if (n == demotorzone) {
            if (demotorzone_piazzato) {
                ss->nemico = nessun_nemico;
            } else {
                ss->nemico = demotorzone;
                demotorzone_piazzato = 1;
            }
        } else {
            ss->nemico = n;
        }

        ss = ss->avanti;
    }

    /* Se non c'è il demotorzone ne mette uno */
    if (!demotorzone_piazzato) {
    int idx = rand() % NUM_ZONE;
    Zona_soprasotto *z = prima_zona_soprasotto;

    for (int i = 0; i < idx && z; i++)
        z = z->avanti;

    if (z)
        z->nemico = demotorzone;
}

}


/* ===== UTIL ===== */

static void libera_mappa(void) {
    while (prima_zona_mondoreale) {
        Zona_mondoreale *t = prima_zona_mondoreale;
        prima_zona_mondoreale = t->avanti;
        free(t);
    }

    while (prima_zona_soprasotto) {
        Zona_soprasotto *t = prima_zona_soprasotto;
        prima_zona_soprasotto = t->avanti;
        free(t);
    }

    //  QUESTO È FONDAMENTALE
    prima_zona_mondoreale = NULL;
    prima_zona_soprasotto = NULL;
}


static void libera_giocatori(void) {
    for (int i = 0; i < num_giocatori; i++) {
        free(giocatori[i]);
        giocatori[i] = NULL;
    }
    num_giocatori = 0;
}

static void reset_azioni_giocatori(void) {
    if (num_giocatori <= 0) return;

    for (int i = 0; i < num_giocatori; i++) {
        if (giocatori[i] != NULL) {
            giocatori[i]->num_azioni = AZIONI_PER_TURNO;
        } else {
            printf("WARNING: giocatori[%d] non allocato!\n", i);
        }
    }
}



/* ===== MOVIMENTO ===== */

static void vai_avanti(Giocatore *g) {
    if (!g) return;

    Tipo_nemico n = (g->mondo == 0) ? g->pos_mondoreale->nemico
                                     : g->pos_soprasotto->nemico;

    if (n != nessun_nemico && !g->bypass_nemico) {
        printf("Non puoi avanzare: un nemico blocca il passaggio! Usa la bicicletta.\n");
        return;
    }

    if (g->mondo == 0 && g->pos_mondoreale->avanti) {
        g->pos_mondoreale = g->pos_mondoreale->avanti;
        printf("Avanzi nel mondo reale\n");
    } else if (g->mondo == 1 && g->pos_soprasotto->avanti) {
        g->pos_soprasotto = g->pos_soprasotto->avanti;
        printf("Avanzi nel soprasotto\n");
    } else {
        printf("Non puoi avanzare\n");
        return;
    }

    g->num_azioni--;
    g->bypass_nemico = 0; // reset del bypass
}

static void vai_indietro(Giocatore *g) {
    if (!g) return;

    Tipo_nemico n = (g->mondo == 0) ? g->pos_mondoreale->nemico
                                     : g->pos_soprasotto->nemico;

    if (n != nessun_nemico && !g->bypass_nemico) {
        printf("Non puoi tornare indietro: un nemico blocca il passaggio! Usa la bicicletta.\n");
        return;
    }

    if (g->mondo == 0 && g->pos_mondoreale->indietro) {
        g->pos_mondoreale = g->pos_mondoreale->indietro;
        printf("Torni indietro nel mondo reale\n");
    } else if (g->mondo == 1 && g->pos_soprasotto->indietro) {
        g->pos_soprasotto = g->pos_soprasotto->indietro;
        printf("Torni indietro nel soprasotto\n");
    } else {
        printf("Non puoi tornare indietro\n");
        return;
    }

    g->num_azioni--;
    g->bypass_nemico = 0; // reset del bypass
}



static void cambia_mondo(Giocatore *g) {
    if (g->mondo == 0) {
        g->pos_soprasotto = g->pos_mondoreale->link_soprasotto;
        g->mondo = 1;
        avvisa_nemico_stanza(g);
        avvisa_oggetto_stanza(g);
        printf("Entri nel SOPRASOTTO\n");
    } else {
        g->pos_mondoreale = g->pos_soprasotto->link_mondoreale;
        g->mondo = 0;
        avvisa_nemico_stanza(g);
        avvisa_oggetto_stanza(g);
        printf("Torni nel MONDO REALE\n");
    }
    g->num_azioni--;
}
/* ===== FUNZIONE HELPER ===== */
static Zona_mondoreale *zona_corrente_mondoreale(Giocatore *g) {
    return g->pos_mondoreale;
}

static Zona_soprasotto *zona_corrente_soprasotto(Giocatore *g) {
    return g->pos_soprasotto;
}


static void dove_si_trova(Giocatore *g) {
    if (g->mondo == 0)
        printf("Mondo reale, zona %d\n", g->pos_mondoreale->tipo);
    else
        printf("Soprasotto, zona %d\n", g->pos_soprasotto->tipo);
}

/* ===== ORDINE ===== */

static void stabilisci_ordine_giocatori(void) {
    for (int i = 0; i < num_giocatori; i++)
        giocatori[i]->iniziativa = lancia_d4();
}

/* ===== FUNZIONI PUBBLICHE ===== */

void imposta_gioco(void) {
    libera_mappa();
    libera_giocatori();
    srand(time(NULL));

    int scelta;
    int setup_finito = 0;


        /* ===== SETUP MAPPA ===== */

    do {
        printf("\n=== SETUP MAPPA ===\n");
        printf("0) Genera mappa casuale\n");
        printf("1) Inserisci zona manualmente\n");
        printf("2) Cancella zona\n");
        printf("3) Stampa mappa\n");
        printf("4) Continua con il gioco\n");
        printf("5) Chiudi mappa\n");
        printf("Scelta: ");
        scanf("%d", &scelta);

        switch (scelta) {
            case 0:{
                libera_mappa();
                crea_mappa();
                printf("Mappa rigenerata\n");
            break;
            }
            case 1: {
                int pos;
                printf("In che posizione inserire la zona? ");
                scanf("%d", &pos);
                inserisci_zona(pos);
            break;
            }

            case 2:{
                cancella_zona();
            break;
            }

            case 3:{
                stampa_mappa();
            break;
            }

            case 4:{
                if (!mappa_chiusa) {
                    printf("Devi chiudere la mappa prima di continuare!\n");
                    break;
                }
                setup_finito = 1;
            break;
            }

            case 5:{
                chiudi_mappa(); 
            break;
            }

            default:{
                printf("Scelta non valida, riprova\n");
            }
    }

    } while (!setup_finito);

     /* ===== CREAZIONE GIOCATORI ===== */
    printf("Numero giocatori (1-4): ");
    scanf("%d", &num_giocatori);

    if (num_giocatori < 1 || num_giocatori > MAX_GIOCATORI) {
        printf("Numero non valido\n");
        num_giocatori = 0;
        return;
    }

    for (int i = 0; i < num_giocatori; i++) {
        giocatori[i] = malloc(sizeof(Giocatore)); //sotto a forma allocazione

    printf("Nome giocatore %d: ", i + 1);
    scanf(" %49s", giocatori[i]->nome);

    int atk  = lancia_d20();
    int def  = lancia_d20();
    int fort = lancia_d20();

    /* ===== STAMPA STATISTICHE INIZIALI ===== */
    printf("\nStatistiche iniziali di %s:\n", giocatori[i]->nome);
    printf("ATK: %d | DEF: %d | FORT: %d\n", atk, def, fort);

    /* ===== BONUS SPECIALE ===== */
    if (strcmp(giocatori[i]->nome, "UndiciVirgolaCinque") == 0) {
        atk += 3;
        def += 3;
        fort /= 2;
        if (fort < 1) fort = 1;
        printf("Bonus speciale UndiciVirgolaCinque applicato!\n");
    }
    else if (strcmp(giocatori[i]->nome, "Gianluca") == 0) {
        atk += 50;
        def += 50;
        fort += 50;
        printf("Bonus speciale Gianluca applicato!\n");
    }

    /* ===== SCELTA MODIFICA STATISTICHE ===== */
    else {
        int scelta;
        do {
            printf("\nScegli modifica statistiche:\n");
            printf("0) Nessuna Modifica\n");
            printf("1) +3 ATK / -3 DEF\n");
            printf("2) -3 ATK / +3 DEF\n");
            scanf("%d", &scelta);
        } while (scelta != 0 && scelta != 1 && scelta != 2);

        if (scelta == 1) {
            atk += 3;
            def -= 3;
        } else if (scelta == 2) {
            atk -= 3;
            def += 3;
        }
    }

    /* ===== BLOCCO VALORI NEGATIVI ===== */
    if (atk < 0) atk = 0;
    if (def < 0) def = 0;
    if (fort < 0) fort = 0;

    /* ===== ASSEGNAZIONE AL GIOCATORE ===== */
    giocatori[i]->attacco_pischico = atk;
    giocatori[i]->difesa_pischica  = def;
    giocatori[i]->fortuna          = fort;
    giocatori[i]->mondo = 0;
    giocatori[i]->num_azioni = AZIONI_PER_TURNO;
    for (int j = 0; j < 3; j++)
        giocatori[i]->zaino[j] = nessun_oggetto;

    /* ===== STAMPA STATISTICHE DOPO MODIFICA ===== */
    printf("\nStatistiche finali di %s:\n", giocatori[i]->nome);
    printf("ATK: %d | DEF: %d | FORT: %d\n", atk, def, fort);
    }
    crea_mappa();
    assegna_nemici_mappa();


    for (int i = 0; i < num_giocatori; i++) {
        giocatori[i]->pos_mondoreale = prima_zona_mondoreale;
        giocatori[i]->pos_soprasotto = prima_zona_soprasotto;
    }

    stabilisci_ordine_giocatori();
    gioco_impostato = 1;
}


    void gioca(void) {

    if (!gioco_impostato || !mappa_chiusa) {
        printf("Errore: gioco non impostato\n");
        return;
    }


    while (demotorzone_vivo) {
        reset_azioni_giocatori();

        for (int i = 0; i < num_giocatori; i++) {
            Giocatore *g = giocatori[i];

            // Skip i giocatori già "morti" (difesa <= 0)
            if (g->difesa_pischica <= 0)
                continue;

            while (g->num_azioni > 0) {
                int s;
                printf("\nGiocatore: %s | Azioni rimaste: %d\n", g->nome, g->num_azioni);
                dove_si_trova(g);
                avvisa_nemico_stanza(g);
                avvisa_oggetto_stanza(g);
                printf("\n");
                printf("1) Avanti\n");
                printf("2) Indietro\n");
                printf("3) Cambia mondo\n");
                printf("4) Passa Turno\n");
                printf("5) Combatti\n");
                printf("6) Raccogli oggetto\n");
                printf("7) Usa oggetto\n");
                printf("8) Mostra Zaino\n");
                printf("9) Abbandona gioco\n");
                printf("Scelta: ");
                scanf("%d", &s);

                switch (s) {
                    case 1: { // Avanti
                        if ((g->mondo == 0 && g->pos_mondoreale->nemico != nessun_nemico && !g->bypass_nemico) ||
                            (g->mondo == 1 && g->pos_soprasotto->nemico != nessun_nemico && !g->bypass_nemico)) {
                            printf("Non puoi avanzare: nemico presente! Usa la bicicletta per bypass.\n");
                        } else {
                            vai_avanti(g);
                        }
                    break;}
                    case 2:{  // Indietro
                        if ((g->mondo == 0 && g->pos_mondoreale->nemico != nessun_nemico && !g->bypass_nemico) ||
                            (g->mondo == 1 && g->pos_soprasotto->nemico != nessun_nemico && !g->bypass_nemico)) {
                            printf("Non puoi tornare indietro: nemico presente! Usa la bicicletta per bypass.\n");
                        } else {
                            vai_indietro(g);
                        }
                    break;}
                    case 3:{ cambia_mondo(g); break;}
                    case 4:{ g->num_azioni = 0; break;}
                    case 5:{
                            combatti(g);

                            if (partita_finita) {
                                termina_gioco();
                                partita_finita = 0;
                                return;
                            }
                            break;
}

                    case 6:{ raccogli_oggetto(g); break;}
                    case 7:{
                        int slot;
                        mostra_zaino(g);
                        printf("Slot oggetto da usare (0-2): ");
                        scanf("%d", &slot);
                        utilizza_oggetto(g, slot);
                        break;}
                    case 8:{ mostra_zaino(g); break;}
                    case 9: {
                        printf("Abbandoni il gioco.\n");
                        demotorzone_vivo = 0;
                    return;
}

                    default:{ printf("Comando non valido\n"); break;}
                }
                g->bypass_nemico = 0;
            }
        }
    }

}


void termina_gioco(void) {
    printf("\nGrazie per aver giocato!\n");

    libera_mappa();
    libera_giocatori();

    mappa_chiusa = 0;
    gioco_impostato = 0;
    demotorzone_vivo = 0;

    printf("\nRitorno al menu principale...\n\n");
}





void crediti(void) {
    printf("\n=== CREDITS ===\n");
    printf("Creatore: Gianluca 391643\n");
    

    printf("\nUltimi vincitori:\n");
    for (int i = 0; i < MAX_VINCITORI; i++) {
        if (strlen(ultimi_vincitori[i]) > 0)
            printf("%d) %s\n", i + 1, ultimi_vincitori[i]);
        else
            printf("%d) Nessun vincitore\n", i + 1);
    }
}


void print_giocatori(void) {
    if (!gioco_essere_impostato()) {
        printf("Errore: gioco non impostato\n");
        return;
    }
    for (int i = 0; i < num_giocatori; i++) {
        Giocatore *g = giocatori[i];
        printf("\n%s | ATK %d | DEF %d | FORT %d | INIT %d\n",
               g->nome,
               g->attacco_pischico,
               g->difesa_pischica,
               g->fortuna,
               g->iniziativa);

               printf("Zaino: ");
        int vuoto = 1;
        for (int j = 0; j < 3; j++) {
            if (g->zaino[j] != nessun_oggetto) {
                printf("%d ", g->zaino[j]);
                vuoto = 0;
            }
        }
        if (vuoto) {
            printf("vuoto");
        }
        printf("\n");
    }
}
static int attacco_nemico(Tipo_nemico n) {
    switch (n) {
        case billi:        return 8;
        case democane:     return 12;
        case demotorzone:  return 16;
        default:           return 0;
    }
}

static int difesa_nemico(Tipo_nemico n) {
    switch (n) {
        case billi:        return 6;
        case democane:     return 10;
        case demotorzone:  return 14;
        default:           return 0;
    }
}

// GESTIONE NEMICI
static const char *nome_nemico(Tipo_nemico n) {
    switch (n) {
        case nessun_nemico: return "Nessun nemico";
        case billi: return "Billi";
        case democane: return "Democane";
        case demotorzone: return "Demotorzone";
        default: return "Sconosciuto";
    }
}

static void avvisa_nemico_stanza(Giocatore *g) {
    Tipo_nemico n;

    if (g->mondo == 0)
        n = g->pos_mondoreale->nemico;
    else
        n = g->pos_soprasotto->nemico;

    if (n == nessun_nemico) {
        printf("Non ci sono nemici in questa stanza\n");
    } else {
        printf("Attenzione! Nemico presente: %s\n", nome_nemico(n));
        printf("Statistiche nemico -> ATK: %d | DEF: %d\n",
               attacco_nemico(n),
               difesa_nemico(n));
    }
}
// GESTIONE OGGETTI
static const char *nome_oggetto(Tipo_oggetto o) {
    switch (o) {
        case bicicletta: return "Bicicletta";
        case maglietta_fuocoinferno: return "Maglietta del Fuoco Infernale";
        case bussola: return "Bussola";
        case schitarrata_metallica: return "Schitarrata Metallica";
        default: return "Nessun oggetto";
    }
}

static void mostra_zaino(Giocatore *g) {
    printf("\nOggetti nello zaino:\n");

    for (int i = 0; i < 3; i++) {
        printf("[%d] ", i);

        if (g->zaino[i] == nessun_oggetto)
            printf("Vuoto\n");
        else
            printf("%s\n", nome_oggetto(g->zaino[i]));
    }
}


static void avvisa_oggetto_stanza(Giocatore *g) {
    Tipo_oggetto o;

    if (g->mondo == 0)
        o = g->pos_mondoreale->oggetto;
    else
        o = g->pos_soprasotto->oggetto;

    if (o == nessun_oggetto) {
        printf("Non ci sono oggetti in questa stanza.\n");
    } else {
        printf("Oggetto presente nella stanza: %s\n", nome_oggetto(o));
    }
}


static void combatti(Giocatore *g) {
    if (!g) return;

    Tipo_nemico n = (g->mondo == 0) ? g->pos_mondoreale->nemico
                                     : g->pos_soprasotto->nemico;

    if (n == nessun_nemico) {
        printf("Non ci sono nemici qui!\n");
        return;
    }

    int atk_g = g->attacco_pischico;
    int def_g = g->difesa_pischica;
    int atk_n = attacco_nemico(n);
    int def_n = difesa_nemico(n);

    def_g += g->bonus_difesa;

    printf("\n--- INIZIO COMBATTIMENTO ---\n");
    printf("Nemico: %s | ATK %d | DEF %d\n", nome_nemico(n), atk_n, def_n);

    while (def_g > 0 && def_n > 0) {
        // Mostra sempre statistiche giocatore
        printf("\n--- STATISTICHE GIOCATORE ---\n");
        printf("%s | ATK: %d | DEF: %d | FORT: %d\n", g->nome, g->attacco_pischico, g->difesa_pischica, g->fortuna);
        printf("Bonus difesa: %d | Danno doppio: %d | Bypass nemico: %d | Dimezza danno: %d\n",
               g->bonus_difesa, g->doppio_danno, g->bypass_nemico, g->dimezza_danno);

        // Attacco giocatore
        int danno_g = atk_g;
        if (g->doppio_danno) danno_g *= 2;
        def_n -= danno_g;
        if (def_n < 0) def_n = 0;
        printf("%s attacca il nemico, difesa nemico: %d\n", g->nome, def_n);
        if (def_n == 0) break;

        // Attacco nemico
        int danno_n = atk_n;
        if (g->dimezza_danno) danno_n /= 2;
        def_g -= danno_n;
        if (def_g < 0) def_g = 0;
        printf("Il nemico attacca, tua difesa: %d\n", def_g);
    }

    // Risultati
    if (def_g == 0) {
        printf("\nSei stato sconfitto!\n");
        demotorzone_vivo = 0;
        partita_finita = 1;
        return;
    } else {
        printf("\nHai sconfitto il nemico!\n");
        if (g->mondo == 0) g->pos_mondoreale->nemico = nessun_nemico;
        else g->pos_soprasotto->nemico = nessun_nemico;

        if (n == demotorzone) {
            printf("Hai sconfitto il Demotorzone e vinto il gioco!\n");
            demotorzone_vivo = 0;

            strncpy(ultimi_vincitori[indice_vincitore % MAX_VINCITORI], g->nome, 49);
            ultimi_vincitori[indice_vincitore % MAX_VINCITORI][49] = '\0';
            indice_vincitore++;

            partita_finita = 2;
    return;
}

    }

    // Reset bonus temporanei
    g->bonus_difesa = 0;
    g->doppio_danno = 0;
    g->dimezza_danno = 0;
    g->bypass_nemico = 0;

    printf("--- COMBATTIMENTO TERMINATO ---\n");
}



void raccogli_oggetto(Giocatore *g) {

    /* === ZONA CORRENTE === */
    Tipo_oggetto *oggetto_zona = NULL;

    if (g->mondo == 0) {
        Zona_mondoreale *z = zona_corrente_mondoreale(g);
        oggetto_zona = &z->oggetto;

        if (z->nemico != nessun_nemico) {
            printf("Devi prima sconfiggere il nemico!\n");
            return;
        }
    } else {
        Zona_soprasotto *z = zona_corrente_soprasotto(g);
        oggetto_zona = &z->oggetto;

        if (z->nemico != nessun_nemico) {
            printf("Devi prima sconfiggere il nemico!\n");
            return;
        }
    }

    /* === CONTROLLO OGGETTO === */
    if (*oggetto_zona == nessun_oggetto) {
        printf("Non ci sono oggetti qui.\n");
        return;
    }

    /* === ZAINO === */
    for (int i = 0; i < 3; i++) {
        if (g->zaino[i] == nessun_oggetto) {
            g->zaino[i] = *oggetto_zona;
            printf("Oggetto raccolto!\n");
            *oggetto_zona = nessun_oggetto;
            return;
        }
    }

    printf("Zaino pieno!\n");
}




void utilizza_oggetto(Giocatore *g, int slot) {
    if (!g) return;

    if (slot < 0 || slot >= 3) {
        printf("Slot non valido\n");
        return;
    }

    Tipo_oggetto obj = g->zaino[slot];
    if (obj == nessun_oggetto) {
        printf("Slot vuoto\n");
        return;
    }

    switch (obj) {
        case bicicletta:{
    printf("%s usa la bicicletta! Puoi passare alla prossima stanza senza combattere.\n", g->nome);
    g->bypass_nemico = 1;
    break;}

case maglietta_fuocoinferno:{
    printf("%s indossa la maglietta fuoco inferno! +3 DEF.\n", g->nome);
    g->bonus_difesa = 3;

    if (lancia_d20() <= g->fortuna) {
        g->dimezza_danno = 1;
        printf("La fortuna ti protegge! Subirai metà danni.\n");
    }
    break;}

case bussola: {
    printf("%s usa la bussola...\n", g->nome);
    if (lancia_d20() <= g->fortuna) {
        g->num_azioni++;
        printf("La bussola ti guida! +1 azione\n");
    } else {
        printf("La bussola gira a vuoto...\n");
    }
    break;
}

case schitarrata_metallica: {
    printf("%s La melodia della chitarra rende debole il nemico! Riceverà doppio danno.\n", g->nome);
    g->doppio_danno = 1;
    break;
}

default: {break; }
    }

    g->zaino[slot] = nessun_oggetto;
}

static void stampa_mappa(void) {
    int scelta;
    printf("\nStampa mappa:\n");
    printf("0) Mondo Reale\n");
    printf("1) Soprasotto\n");
    scanf("%d", &scelta);

    if (scelta == 0) {
        Zona_mondoreale *z = prima_zona_mondoreale;
        int i = 0;
        while (z) {
            printf("\nZona %d | Tipo: %d | Nemico: %s | Oggetto: %d",
                   i,
                   z->tipo,
                   nome_nemico(z->nemico),
                   z->oggetto);
            z = z->avanti;
            i++;
        }
    } 
    else if (scelta == 1) {
        Zona_soprasotto *z = prima_zona_soprasotto;
        int i = 0;
        while (z) {
            printf("\nZona %d | Tipo: %d | Nemico: %s",
                   i,
                   z->tipo,
                   nome_nemico(z->nemico));
            z = z->avanti;
            i++;
        }
    } 
    else {
        printf("Scelta non valida\n");
    }

    printf("\n");
}

int gioco_essere_impostato(void) {
    return gioco_impostato;
}


/* ===== DADI ===== */

static int lancia_d20(void) {
    return rand() % 20 + 1;
}

static int lancia_d4(void) {
    return rand() % 4 + 1;
}
