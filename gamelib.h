#ifndef GAMELIB_H
#define GAMELIB_H

/* ===== ENUM ===== */

typedef enum {
    bosco,
    scuola,
    laboratorio,
    caverna,
    strada,
    giardino,
    supermercato,
    centrale_elettrica,
    deposito_abbandonato,
    stazione_polizia
} Tipo_zona;

typedef enum {
    nessun_nemico,
    billi,
    democane,
    demotorzone
} Tipo_nemico;

typedef enum {
    nessun_oggetto,
    bicicletta,
    maglietta_fuocoinferno,
    bussola,
    schitarrata_metallica
} Tipo_oggetto;

/* ===== STRUCT ===== */

struct Zona_soprasotto;

typedef struct Zona_mondoreale {
    Tipo_zona tipo;
    Tipo_nemico nemico;
    Tipo_oggetto oggetto;

    struct Zona_mondoreale *avanti;
    struct Zona_mondoreale *indietro;
    struct Zona_soprasotto *link_soprasotto;
} Zona_mondoreale;

typedef struct Zona_soprasotto {
    Tipo_zona tipo;
    Tipo_nemico nemico;
    Tipo_oggetto oggetto;

    struct Zona_soprasotto *avanti;
    struct Zona_soprasotto *indietro;
    Zona_mondoreale *link_mondoreale;
} Zona_soprasotto;

typedef struct {
    char nome[50];
    int mondo; /* 0 = mondo reale, 1 = soprasotto */

    Zona_mondoreale *pos_mondoreale;
    Zona_soprasotto *pos_soprasotto;

    int attacco_pischico;
    int difesa_pischica;
    int fortuna;
    int num_azioni;
    
    // EFFETTI OGGETTI //
    int bypass_nemico;          // bici: ignora nemico nella prossima stanza
    int bonus_difesa;           // maglietta: +3 DEF temporaneo
    int dimezza_danno;          // fortuna + maglietta
    int doppio_danno;           // chitarra


    int iniziativa;
    Tipo_oggetto zaino[3];
} Giocatore;

/* ===== FUNZIONI PUBBLICHE ===== */
int gioco_essere_impostato(void);
void imposta_gioco(void);
void gioca(void);
void termina_gioco(void);
void crediti(void);
void print_giocatori(void);

#endif

