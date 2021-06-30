#define _DEFAULT_SOURCE /* Pour pas que le usleep ne provoque de warning. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <MLV/MLV_all.h>
#include <unistd.h>
#include <assert.h>
#include "structures.h"

#define HAUTEUR 800 
#define LARGEUR 800



/***************STRUCTURES*****************/

typedef struct coordonnees_t {
/* Permet de repérer une case dans le labyrinthe. */
    int x;
    int y;
} coordonnees_t;

typedef struct case_t {
    int murEst; /* Présence ou non d'un mur à l'Est de la case */
    int murSud; /* Présence ou non d'un mur au Sud de la case */
    coordonnees_t pere; /* Coordonnées du père de la case */
    int rang; /* Hauteur potentielle de l'arbre */
} case_t;

typedef struct laby_t {
    coordonnees_t taille; /* Taille maximale du labyrinthe (lignes, colonnes) */
    case_t **cases; /* Tableau a deux dimensions des cases qui composent le labyrinthe. */
    int nb_classes; /* Au début autant que le nombre de cases du labyrinthe, puis se réduit. */
} laby_t;

typedef enum position {
/* Position d'un mur de type cibleMur */
    est,
    sud
} position;

typedef struct cibleMur {
    position pos; /* Position du mur pour les coordonnées indiquées */
    coordonnees_t coor; /* Coordonnées (x, y) de la case */
} cibleMur;



/**********************FONCTIONS***********************/

laby_t initLaby_t (coordonnees_t taille_laby){
/* Initialise une variable de type laby_t. */
    laby_t laby;
    int i, j;

    laby.taille.x = taille_laby.x; /* Nombre de lignes */
    laby.taille.y = taille_laby.y; /* Nombre de colonnes */
    laby.cases = NULL; /* Initialisation du tableau à double entrée */

    laby.nb_classes = taille_laby.x * taille_laby.y; /* Correspond au nombre total de cases au début de la partie, puis se réduit. */

    laby.cases = malloc(sizeof(case_t *) * laby.taille.x); /* Attribution de la place mémoire du tableau à double entrée qui représente les cases du labyrinthe. */
    assert(laby.cases != NULL);

    for (i = 0; i < laby.taille.x; i++){ /* Initialisation des variables de type case_t qui sont contenues dans le tableau dynamique cases. */
        laby.cases[i] = malloc(sizeof(case_t) * laby.taille.y);
        assert(laby.cases[i] != NULL);
        for (j = 0; j < laby.taille.y; j++){
            /* 1 si le mur est présent, 0 s'il est détruit. */
            laby.cases[i][j].murEst = 1; 
            laby.cases[i][j].murSud = 1;
            /* Au début chaque cases est son propre père. */
            laby.cases[i][j].pere.x = i; 
            laby.cases[i][j].pere.y = j;
            /* Comme pour un arbre où il n'y aurait que la racine la hauteur de l'arbre est 0. */
            laby.cases[i][j].rang = 0;
        }
    }

    return laby;
}

void libererLaby_t(laby_t *laby){
/* Désalloue l'espace mémoire utilisé par le tableau cases, de la variable de type laby_t. */
    int i;
    for (i = 0; i < laby->taille.x; i++)
        free(laby->cases[i]);
    free(laby->cases); 
    laby->cases = NULL;   
}

coordonnees_t TrouveCompresse(laby_t *laby, coordonnees_t coord){
/* Trouve le représentant de la classe où se trouve la case de coordonnées coord.
 * Pour cela, on compare le père de la case en question, et la case elle même. Si la case en argument
 * n'est pas son propre père, on définit récursivement comme nouveau père de la case, le résultat de la fonction TrouveCompresse
 * sur son père. Résultat, la case mise en argument ainsi que toutes le cases de la branche ont pour nouveau père le représentant de la classe. */
    if (coord.x != laby->cases[coord.x][coord.y].pere.x || 
        coord.y != laby->cases[coord.x][coord.y].pere.y)
        /* La case n'est pas le représentant de sa classe. */
        laby->cases[coord.x][coord.y].pere = TrouveCompresse(laby, laby->cases[coord.x][coord.y].pere); /*On lui affilie comme nouveau père le représentant de la classe. */
    
    return laby->cases[coord.x][coord.y].pere; /* On renvoie le représentant de la classe. */
}

void FusionCoord(laby_t *laby, coordonnees_t coord1, coordonnees_t coord2){
/* Cherche le représentant de la classe respective des deux coordonnées mises en paramètre.
 * Compare leur rang et fait du représentant au rang le plus élevé le père de l'autre.
 * En cas d'égalité, le premier représentant devient le père du deuxième par defaut, et augmente son rang de 1.*/
    coordonnees_t representant1 = TrouveCompresse(laby, coord1);
    coordonnees_t representant2 = TrouveCompresse(laby, coord2); /* On récupère pour chaque coordonnées mises en argument le représentant de la classe de chacune. */

    /* On compare leurs rangs. Le représentant au plus petit rang a pour nouveau père l'autre. */
    if(laby->cases[representant2.x][representant2.y].rang > laby->cases[representant1.x][representant1.y].rang){
        laby->cases[representant1.x][representant1.y].pere = laby->cases[representant2.x][representant2.y].pere;
    }
    else {
        laby->cases[representant2.x][representant2.y].pere = laby->cases[representant1.x][representant1.y].pere;
        /* Si les rangs sont égaux, on incrémente le rang du nouveau représentant de 1. */
        if (laby->cases[representant2.x][representant2.y].rang == laby->cases[representant1.x][representant1.y].rang)
            laby->cases[representant1.x][representant1.y].rang ++;
    }    

}


coordonnees_t aleaCoord(laby_t laby){
/* Renvoie des coordonnées aleatoire du labyrinthe. */
    coordonnees_t coord;
    coord.x = rand() % laby.taille.x;
    coord.y = rand() % laby.taille.y;
    return coord;
}

void supprime_mur_naif(laby_t *laby){
/* Supprime un mur du labyrinthe. */
    coordonnees_t coord, coord2;
    int mur;

    while(1){
        coord = aleaCoord(*laby); /* Tire une coordonnée du labyrinthe aléatoirement. */
        mur = rand () % 2; /* Choix entre mur Est et mur Sud. */
        
        if (!mur){/* Destruction du mur Est de la case qui a pour coordonnées coord. */
            if (laby->cases[coord.x][coord.y].murEst && coord.y < laby->taille.y - 1){ /* On vérifie si le mur n'est pas déjà détruit et s'ilest interne (murs externes indestructibles). */
                laby->cases[coord.x][coord.y].murEst = 0;/* Destruction du mur */
                coord2.x = coord.x;
                coord2.y = coord.y + 1;
                /* On vérifie si la case à droite est dans la même classe. */
                if (TrouveCompresse(laby, coord).x != TrouveCompresse(laby, coord2).x || 
                    TrouveCompresse(laby, coord).y != TrouveCompresse(laby, coord2).y){
                    /* Si oui on décrémente le nombre de classe. */
                    laby->nb_classes --;
                    /* Et on les met dans la même classe. */
                    FusionCoord(laby, coord, coord2);
                }
                break;
            }
        }
        
        else{/* Destruction du mur Sud de la case qui a pour coordonnées coord. */
            if (laby->cases[coord.x][coord.y].murSud && coord.x < laby->taille.x - 1){
                laby->cases[coord.x][coord.y].murSud = 0;
                coord2.x = coord.x + 1;
                coord2.y = coord.y;
                if (TrouveCompresse(laby, coord).x != TrouveCompresse(laby, coord2).x || 
                    TrouveCompresse(laby, coord).y != TrouveCompresse(laby, coord2).y){
                    laby->nb_classes --;
                    FusionCoord(laby, coord, coord2);
                }
                break;
            }
        }
    }
}

void supprime_mur_unique(laby_t *laby){
/*Supprime un mur du labyrinthe seulement s'il sépare 2 cases qui ne sont pas dans la même classe.*/
    coordonnees_t coord, coord2;
    int mur;

    while(1){
        coord = aleaCoord(*laby);
        mur = rand () % 2;
        if (!mur){
            if (laby->cases[coord.x][coord.y].murEst && coord.y < laby->taille.y - 1){
                coord2.x = coord.x;
                coord2.y = coord.y + 1;
                if (TrouveCompresse(laby, coord).x != TrouveCompresse(laby, coord2).x || 
                    TrouveCompresse(laby, coord).y != TrouveCompresse(laby, coord2).y){
                    laby->cases[coord.x][coord.y].murEst = 0;/* On ne détruit le mur que si les cases qu'il sépare ne sont pas déjà reliées. */
                    laby->nb_classes --;
                    FusionCoord(laby, coord, coord2);
                    break;
                }
            }
        }
        else{
            if (laby->cases[coord.x][coord.y].murSud && coord.x < laby->taille.x - 1){
                coord2.x = coord.x + 1;
                coord2.y = coord.y;
                if (TrouveCompresse(laby, coord).x != TrouveCompresse(laby, coord2).x || 
                    TrouveCompresse(laby, coord).y != TrouveCompresse(laby, coord2).y){
                    laby->cases[coord.x][coord.y].murSud = 0;/* On ne détruit le mur que si les cases qu'il sépare ne sont pas déjà reliées. */
                    laby->nb_classes --;
                    FusionCoord(laby, coord, coord2);
                    break;
                }
            }
        }
    }
}

void supprime_mur(laby_t *laby, int unique){
/*Appelle l'une des deux fonctions précédentes en fonction du paramètre unique, s'il est activé ou non.*/
    if (unique) 
        supprime_mur_unique(laby);
    else
        supprime_mur_naif(laby);
}

int init_taille_tableau_mur(laby_t laby){
/* Renvoie la taille que dois faire le tableau des murs du labyrinthe. */
    int tailleTableau = (((laby.taille.x - 1) * (laby.taille.y - 1))) * 2 + (laby.taille.x - 1) + (laby.taille.y - 1);
    return tailleTableau;
}

void remplit_tableau_mur(laby_t laby, cibleMur *tableauMur){
/* Remplit le tableau des murs avec les valeurs appropriées. */
    int i, j, k;
    k = 0;
    
    for(i = 0; i < laby.taille.x; i++){
        
        for(j = 0; j < laby.taille.y; j++){

            if(j < laby.taille.y - 1){
                tableauMur[k].coor.x = i;
                tableauMur[k].coor.y = j;
                tableauMur[k].pos = est;
                k ++;
            }
            
            if(i < laby.taille.x - 1){
                tableauMur[k].coor.x = i;
                tableauMur[k].coor.y = j;
                tableauMur[k].pos = sud;
                k ++;
            }  
        }
    }
}

void echange_valeur(cibleMur *a, cibleMur *b){
/* Echange la valeur de deux cases du tableau des murs. */
    cibleMur temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

void melange_tableau_mur(cibleMur *tableauMur, int taille){
/* Mélange les cases du tableau des murs dans un ordre aléatoire. */
    int i, j;
    for(i = 1; i < taille; i++){
        j = rand() % i;
        echange_valeur(&tableauMur[i], &tableauMur[j]);
    }
}

int supprime_mur_par_tableau(laby_t *laby, cibleMur tableauMur[], int indice){
/* Supprime un mur du labyrinthe en parcourant le tableau des murs. On renvoie l'indice incrémenté pour accéder à la case suivante. */
    coordonnees_t coor1, coor2;
    
    coor1 = tableauMur[indice].coor;
    switch(tableauMur[indice].pos){
        case est:
            coor2.x = coor1.x;
            coor2.y = coor1.y + 1;
            laby->cases[coor1.x][coor1.y].murEst = 0;
            if (TrouveCompresse(laby, coor1).x != TrouveCompresse(laby, coor2).x || 
            TrouveCompresse(laby, coor1).y != TrouveCompresse(laby, coor2).y){
                laby->nb_classes --;
                FusionCoord(laby, coor1, coor2);
            }
            break;
        case sud:
            coor2.x = coor1.x + 1;
            coor2.y = coor1.y;
            laby->cases[coor1.x][coor1.y].murSud = 0;
            if (TrouveCompresse(laby, coor1).x != TrouveCompresse(laby, coor2).x || 
            TrouveCompresse(laby, coor1).y != TrouveCompresse(laby, coor2).y){
                laby->nb_classes --;
                FusionCoord(laby, coor1, coor2);
            }
            break;
    }
    return indice + 1 ;
}

int supprime_mur_par_tableau_unique(laby_t *laby, cibleMur tableauMur[], int indice){
/*Supprime un mur du labyrinthe en parcourant le tableau des murs seulement si ce mur sépare 2 cases qui n'ont pas la même classe.
* On renvoie l'indice incrémenté pour accéder à la case suivante.*/ 
    coordonnees_t coor1, coor2;

    /*On fait une boucle pour être sÛr qu'un mur sera supprimé.*/
    int mur_supprime = 0;
    
    while (!mur_supprime){
        coor1 = tableauMur[indice].coor;
        switch(tableauMur[indice].pos){
            case est:
                coor2.x = coor1.x;
                coor2.y = coor1.y + 1;
                if (TrouveCompresse(laby, coor1).x != TrouveCompresse(laby, coor2).x || 
                TrouveCompresse(laby, coor1).y != TrouveCompresse(laby, coor2).y){
                    laby->nb_classes --;
                    laby->cases[coor1.x][coor1.y].murEst = 0;/* On ne détruit le mur que si les cases qu'il sépare ne sont pas déjà reliées. */
                    FusionCoord(laby, coor1, coor2);
                    mur_supprime = 1;
                }
                break;
            case sud:
                coor2.x = coor1.x + 1;
                coor2.y = coor1.y;
                if (TrouveCompresse(laby, coor1).x != TrouveCompresse(laby, coor2).x || 
                TrouveCompresse(laby, coor1).y != TrouveCompresse(laby, coor2).y){
                    laby->nb_classes --;
                    laby->cases[coor1.x][coor1.y].murSud = 0;/* On ne détruit le mur que si les cases qu'il sépare ne sont pas déjà reliées. */
                    FusionCoord(laby, coor1, coor2);
                    mur_supprime = 1;
                }
                break;
        }
        indice ++;
    }
    return indice;
}


int supprime_mur_fast(laby_t *laby, cibleMur tableauMur[], int indice, int unique){
/* Appelle l'une des deux fonctions précédentes en fonction du paramètre unique, s'il est activé ou non. */
    if(unique) 
        return supprime_mur_par_tableau_unique(laby, tableauMur, indice);
    else
        return supprime_mur_par_tableau(laby, tableauMur, indice);
}


int labyValide(laby_t laby){
/* Renvoie 1 s'il existe un chemin entre le départ et l'arrivée, c'est-à-dire si les coordonnées (0,0) et (laby.taille.x, laby.taille.y) sont dans la même classe. 
 * Renvoie 0 sinon. */
    coordonnees_t coord, coord2;
    coord.x = 0;
    coord.y = 0;
    coord2.x = laby.taille.x - 1;
    coord2.y = laby.taille.y - 1;
    return (TrouveCompresse(&laby, coord).x == TrouveCompresse(&laby, coord2).x &&
            TrouveCompresse(&laby, coord).y == TrouveCompresse(&laby, coord2).y);

}

int labyAccessible(laby_t laby){
/* Renvoie 1 si le labyrinthe est accessible, c'est-à-dire si toutes les cases sont reliées entre elles, donc qu'il ne reste qu'une classe. Renvoie 0 sinon. */
    assert(laby.nb_classes > 0);
    if (laby.nb_classes == 1)
        return 1;
    return 0;
}

int bonLaby(laby_t laby, int acces){
    /* Adapte les conditions d'arrêt du labyrinthe. Si le labyrinthe est valide, 
     * observe si l'option --acces est activée. Si oui, vérifie également s'il est accessible. 
     * Renvoie 1 si toutes les conditions d'arrêt demandées sont remplies, 0 sinon. */
    if (labyValide(laby)){
        if (!acces)
            return 1;
        if (labyAccessible(laby))
            return 1;
    }
    return 0;
}

void afficherLaby(laby_t laby){
/*Affiche un labyrinthe dans la console.*/
    int i, j;

    printf("\n\n\n");

    for(j = 0; j < laby.taille.y; j++)
        printf("+--");
    printf("+\n");

    for (i = 0; i < laby.taille.x; i++){

        if (i != 0) printf("|");
        else printf(" ");

        for (j = 0; j < laby.taille.y; j++){
            printf("  ");
            if (laby.cases[i][j].murEst && (i != laby.taille.x - 1 || j != laby.taille.y - 1)) printf("|");
            else printf(" ");
        }

        printf("\n");

        for(j = 0; j < laby.taille.y; j++){
            if (!laby.cases[i][j].murSud){
                if (j - 1 >= 0){
                    if (!laby.cases[i][j - 1].murSud && !laby.cases[i][j - 1].murEst){
                        if (i + 1 < laby.taille.x){
                            if (!laby.cases[i + 1][j - 1].murEst)
                                printf(" ");
                            else printf("+");
                        }
                        else printf("+");
                    }
                    else printf("+");
                }
                else printf("+");
            }
            else printf("+");
            if (laby.cases[i][j].murSud) printf("--");
            else printf("  ");
        }
        printf("+\n");
    }
    printf("\n\n\n");
}

void afficherLabyUTF8(laby_t laby){
    /* Affiche un labyrinthe dans la console avec des caractères utf-8. */
    
    int i, j;
    
    printf("\n\n\n");
    
    /*Murs externes en haut.*/
    for(j = 0; j < laby.taille.y; j++){
        if(j == 0){
            /* */
            printf("%s", intersections[0][0][0][0]);
        }
        /*─*/
        printf("%s", intersections[0][0][1][1]);
        
        if(j == laby.taille.y - 1){
            /*┐*/
            printf("%s\n", intersections[1][0][1][0]);
        } 
        else{
            if(laby.cases[0][j].murEst == 0){
                /*─*/
                printf("%s", intersections[0][0][1][1]);

            }
            else{
                /*┬*/
                printf("%s", intersections[1][0][1][1]);
            }
        }
    }
    for(i = 0; i < laby.taille.x; i++){
        if(i != 0){
            /*│*/
            printf("%s", intersections[1][1][0][0]);
        }
        else{
            /* */
            printf("%s", intersections[0][0][0][0]);
        }
        for(j = 0; j < laby.taille.y - 1; j++){
            /* */
            printf("%s", intersections[0][0][0][0]);

            if(laby.cases[i][j].murEst){
                /*│*/
                printf("%s", intersections[1][1][0][0]);
            }
            else{
                /* */
                printf("%s", intersections[0][0][0][0]);
            }
        }
        if(i < laby.taille.x - 1){
            /* */
            printf("%s", intersections[0][0][0][0]);
            /*│*/
            printf("%s\n", intersections[1][1][0][0]);
        }
        else{
            printf("\n");
        }
        if(i < laby.taille.x - 1){
            
            if(i == 0){     
                if(laby.cases[i][0].murSud){
                    /*┌*/
                    printf("%s", intersections[1][0][0][1]);
                }
                else{
                    /*╷*/
                    printf("%s", intersections[1][0][0][0]);
                }
            }
            else{
                if(laby.cases[i][0].murSud){
                    /*├*/
                    printf("%s", intersections[1][1][0][1]);
                }
                else{
                    /*│*/
                    printf("%s", intersections[1][1][0][0]);
                }
            }
            for(j = 0; j < laby.taille.y; j++){
                if(laby.cases[i][j].murSud){
                    /*─*/
                    printf("%s", intersections[0][0][1][1]);
                }
                else{
                    /* */
                    printf("%s", intersections[0][0][0][0]);  
                }
                if(j == laby.taille.y - 1){
                    if(i == laby.taille.x - 2){
                        if(laby.cases[i][j].murSud){
                            /*┘*/
                            printf("%s", intersections[0][1][1][0]);
                        }
                        else{
                            /*╵*/
                            printf("%s", intersections[0][1][0][0]);
                        }
                    }
                    else{
                        if(laby.cases[i][j].murSud){
                            /*┤*/
                            printf("%s", intersections[1][1][1][0]);
                        }
                        else{
                            /*│*/
                            printf("%s", intersections[1][1][0][0]);
                        }
                        
                    }   
                }
                else{
                    if(laby.cases[i][j].murSud && laby.cases[i][j].murEst && laby.cases[i][j + 1].murSud && laby.cases[i + 1][j].murEst){
                        /*┼*/
                        printf("%s", intersections[1][1][1][1]);
                    }
                    else if(!(laby.cases[i][j].murSud) && laby.cases[i][j].murEst && laby.cases[i][j + 1].murSud && laby.cases[i + 1][j].murEst){
                        /*├*/
                        printf("%s", intersections[1][1][0][1]);
                    }
                    else if(laby.cases[i][j].murSud && !(laby.cases[i][j].murEst) && laby.cases[i][j + 1].murSud && laby.cases[i + 1][j].murEst){
                        /*┬*/
                        printf("%s", intersections[1][0][1][1]);
                    }
                    else if(laby.cases[i][j].murSud && laby.cases[i][j].murEst && !(laby.cases[i][j + 1].murSud) && laby.cases[i + 1][j].murEst){
                        /*┤*/
                        printf("%s", intersections[1][1][1][0]);
                    }
                    else if(laby.cases[i][j].murSud && laby.cases[i][j].murEst && laby.cases[i][j + 1].murSud && !(laby.cases[i + 1][j].murEst)){
                        /*┴*/
                        printf("%s", intersections[0][1][1][1]);
                    }
                    else if(!(laby.cases[i][j].murSud) && !(laby.cases[i][j].murEst) && laby.cases[i][j + 1].murSud && laby.cases[i + 1][j].murEst){
                        /*┌*/
                        printf("%s", intersections[1][0][0][1]);
                    }
                    else if(!(laby.cases[i][j].murSud) && laby.cases[i][j].murEst && !(laby.cases[i][j + 1].murSud) && laby.cases[i + 1][j].murEst){
                        /*│*/
                        printf("%s", intersections[1][1][0][0]);
                    }
                    else if(!(laby.cases[i][j].murSud) && laby.cases[i][j].murEst && laby.cases[i][j + 1].murSud && !(laby.cases[i + 1][j].murEst)){
                        /*└*/
                        printf("%s", intersections[0][1][0][1]);
                    }
                    else if(laby.cases[i][j].murSud && !(laby.cases[i][j].murEst) && !(laby.cases[i][j + 1].murSud) && laby.cases[i + 1][j].murEst){
                        /*┐*/
                        printf("%s", intersections[1][0][1][0]);
                    }
                    else if(laby.cases[i][j].murSud && !(laby.cases[i][j].murEst) && laby.cases[i][j + 1].murSud && !(laby.cases[i + 1][j].murEst)){
                        /*─*/
                        printf("%s", intersections[0][0][1][1]);
                    }
                    else if(laby.cases[i][j].murSud && laby.cases[i][j].murEst && !(laby.cases[i][j + 1].murSud) && !(laby.cases[i + 1][j].murEst)){
                        /*┘*/
                        printf("%s", intersections[0][1][1][0]);
                    }
                    else if(!(laby.cases[i][j].murSud) && !(laby.cases[i][j].murEst) && !(laby.cases[i][j + 1].murSud) && laby.cases[i + 1][j].murEst){
                        /*╷*/
                        printf("%s", intersections[1][0][0][0]);
                    }
                    else if(!(laby.cases[i][j].murSud) && laby.cases[i][j].murEst && !(laby.cases[i][j + 1].murSud) && !(laby.cases[i + 1][j].murEst)){
                        /*╵*/
                        printf("%s", intersections[0][1][0][0]);
                    }
                    else if(!(laby.cases[i][j].murSud) && !(laby.cases[i][j].murEst) && laby.cases[i][j + 1].murSud && !(laby.cases[i + 1][j].murEst)){
                        /*╶*/
                        printf("%s", intersections[0][0][0][1]);
                    }
                    else if(laby.cases[i][j].murSud && !(laby.cases[i][j].murEst) && !(laby.cases[i][j + 1].murSud) && !(laby.cases[i + 1][j].murEst)){
                        /*╴*/
                        printf("%s", intersections[0][0][1][0]);
                    }
                    else if(!(laby.cases[i][j].murSud) && !(laby.cases[i][j].murEst) && !(laby.cases[i][j + 1].murSud) && !(laby.cases[i + 1][j].murEst)){
                        /* */
                        printf("%s", intersections[0][0][0][0]);
                    }
                }
            }
            printf("\n");
        }
        else{
            /*Murs externes en bas.*/
            for(j = 0; j < laby.taille.y; j++){        
                if(j == 0){
                    /*└*/
                    printf("%s", intersections[0][1][0][1]); 
                }
                else{
                    
                    if(laby.cases[laby.taille.x - 1][j - 1].murEst){
                        /*┴*/
                        printf("%s", intersections[0][1][1][1]);
                        
                    }
                    else{
                        /*─*/
                        printf("%s", intersections[0][0][1][1]);
                    }
                }
                /*─*/
                printf("%s", intersections[0][0][1][1]); 
            }
        }
    }
    printf("\n");
}

void dessineLaby(laby_t laby){
/* Dessine un labyrinthe dans une fenêtre ouverte au préalable.
* Sa taille et son centrage sont adaptés en fonction des dimensions du labyrinthe et de la fenêtre.
* Le centrage n'est pas effectué si la fenêtre n'est pas carrée. */

    int i, j;
    float origine_x, origine_y; 
    float pas_x, pas_y;
    
    pas_x = (HAUTEUR - 20.0) / laby.taille.x;
    pas_y = (LARGEUR - 20.0) / laby.taille.y;
    origine_x = 10.0;
    origine_y = 10.0;

    for(j = 0; j < laby.taille.y; j++)
        MLV_draw_line( origine_y + pas_y * j, origine_x, origine_y + pas_y * (j + 1), origine_x, MLV_COLOR_WHITE);

    for (i = 0; i < laby.taille.x; i++){

        if (i != 0) 
            MLV_draw_line(origine_y, origine_x + pas_x * i, origine_y, origine_x + pas_x * (i + 1), MLV_COLOR_WHITE);
        for (j = 0; j < laby.taille.y; j++){
            if (laby.cases[i][j].murEst && (i != laby.taille.x - 1 || j != laby.taille.y - 1)) 
                MLV_draw_line(origine_y + pas_y * (j + 1), origine_x + pas_x * i, origine_y + pas_y * (j + 1) , origine_x + pas_x * (i + 1), MLV_COLOR_WHITE);
            if (laby.cases[i][j].murSud) 
                MLV_draw_line( origine_y + pas_y * j, origine_x + pas_x * (i + 1), origine_y + pas_y * (j + 1), origine_x + pas_x * (i + 1), MLV_COLOR_WHITE);
        }       
    }
}

int lire_taille_laby(coordonnees_t *taille_laby, char *argument, int taille){
/* Fonction dediée à la chaîne de caractères qui corespond à l'argument de choix de taille.
 * Lis les deux entiers servant de taille au labyrinthe et renvoie 1 si tout s'est bien passé.
 * 0 si des caractères se sont glissés après ou s'il n'y a pas les deux entiers prévus. Fonctionne principalement pour --taille=nxm,
 * n et m deux entiers.  */
    char *pointeur = NULL;
    long abscisse, ordonnee;
    while (!(abscisse = strtol(argument, &pointeur, 10))){ /* Parcours la chaîne jusqu'à tomber sur un caractère numérique. */
        pointeur ++; /* Déplace le pointeur sur le caractère suivant. */
        argument = pointeur; /* Prend comme nouvel argument à lire la partie de la chaîne de caractère encore  non parcourue. */
        if (strlen(argument) == 0) return 0; /* Renvoie 0 s'il n'y pas de caractère numérique dans la chaîne. */
    }
    argument = pointeur;
    if (argument[0] == 'x'){ /* Vérifie la présence du 'x' entre les deux nombres. */
        pointeur ++;
        argument = pointeur;
        if ((ordonnee = strtol(argument, &pointeur, 10)) != 0){ /* Vérifie qu'il y a bien un deuxième entier à la suite du x. */
            if (!strlen(pointeur)){ /* Vérifie qu'il n'y a pas d'autres caractères non désirés à la suite du deuxième entier. */
                taille_laby->x = abscisse; /* Change les dimensions du labyrinthe en fonction des données recueillies. */
                taille_laby->y = ordonnee;
                return 1;
            }
        }
    }
    return 0;
}

int lire_entier(int *entier, char *argument, int taille){
/* Parcours  une chaîne de caractère (exemple "--attente=0" ou "--graine=4") jusqu'au premier caractère numérique/
 * Convertis alors en entier l'ensemble du nombre saisi dans cette chaîne de caractère. Renvoie 1 si tout s'est bien passé,
 * 0 si des caractères non numériques se sont glissés après l'entier ou s'il n'y avait pas de caractères numériques,
 * (ex "--attente=0nb" ou "abcde"). */
    char *pointeur = NULL;
    long temp;
    while (argument[0] != '0' && !(temp = strtol(argument, &pointeur, 10))){ /* Parcours la chaîne jusqu'à tomber sur un caractère numérique. */
        pointeur ++; /* Déplace le pointeur sur le caractère suivant. */
        argument = pointeur; /* Prend comme nouvel argument à lire la partie de la chaîne de caractère encore  non parcourue. */
        if (strlen(argument) == 0) return 0; /* Renvoie 0 s'il n'y pas de caractère numérique dans la chaîne. */
    }
    if (temp == 0)
        pointeur ++; /* Décale le pointeur de 1 si l'entier est égal à 0 (la fonction strtol renvoie 0 si le caractère lu est 0, mais aussi si la fonction ne peut convertir le caractère en entier 
                      * L'entier 0 n'est ainsi pas conservé dans les caractères non lus. */


    if (!strlen(pointeur)){ /* Vérifie qu'il n'y a pas d'autres caractères non désirés à la suite du deuxième entier. */
        *entier = temp; /* On affilie à l'entier la valeur collectée. */
        return 1;
    }
    return 0;
}



/****************MAIN*****************/

int main(int argc, char *argv[]){

    /* Déclaration des variables */
    laby_t laby;
    coordonnees_t taille_laby;
    int arg, graine, attente, indice = 0; 
    int mode_texte = 0, choix_taille = 0, choix_graine = 0, choix_attente = 0, unique = 0, acces = 0, fast = 0;
    char *argument;
    MLV_Keyboard_button touche;
    int tailleTableau;
    cibleMur *tableauMur;
    clock_t debut, fin;
    double tempsExec;

    /* Initialisation de la taille par défaut. */
    taille_laby.x = 8;
    taille_laby.y = 6;

    /* Lancement du timer */
    debut = clock();

    /* Lecture des arguments fournis après le nom de l'exécutable.
     * Adaptation des paramètres. */
    for (arg = 1; arg < argc; arg++){
        if (strlen(argv[arg]) >= 10){
            if(!strcmp(argv[arg], "--mode=texte")) 
                mode_texte = 1;
            if(!strncmp(argv[arg], "--taille=", 9) && !choix_taille){
                argument = argv[arg];
                if (lire_taille_laby(&taille_laby, argument, strlen(argv[arg])))
                    choix_taille = 1;
            }
            if(!strncmp(argv[arg], "--graine=", 9) && !choix_graine){
                argument = argv[arg];
                if (lire_entier(&graine, argument, strlen(argv[arg])))
                    choix_graine = 1;
            }
            if(!strncmp(argv[arg], "--attente=", 10) && !choix_attente){
                argument = argv[arg];
                if (lire_entier(&attente, argument, strlen(argv[arg]))){
                    choix_attente = 1;
                    assert(attente >= 0);
                }
            }
        }
        if(!strcmp(argv[arg], "--unique")) 
            unique = 1;
        if(!strcmp(argv[arg], "--acces")) 
            acces = 1;
        if(!strcmp(argv[arg], "--fast")) 
            fast = 1;
    }

    /* Placement de la graine aléatoire, fixée ou non. */
    if(choix_graine)
        srand(graine);
    else
        srand(time(NULL));

    /* Initialisation du labyrinthe. */
    laby = initLaby_t(taille_laby);

    /* Création du tableau des murs. */
    if (fast){
        tailleTableau = init_taille_tableau_mur(laby);
        tableauMur = malloc (tailleTableau * sizeof(cibleMur));
        remplit_tableau_mur(laby, tableauMur);
        melange_tableau_mur(tableauMur, tailleTableau);
    }

    /* Affichage dans le terminal. */
    if(mode_texte){

        /* Affichage du labyrinthe initial. */
        afficherLabyUTF8(laby);

        /* Boucle principale de la génération d'un labyrinthe valide. */
        while (!bonLaby(laby, acces)){

            /* Effacement d'un mur aléatoire, avec l'algorithme choisi. */
            if (fast)
                indice = supprime_mur_fast(&laby, tableauMur, indice, unique);
            else
                supprime_mur(&laby, unique);

            /* Affichage du labyrinthe avec temps d'attente. */ 
            if (choix_attente){
                if(attente != 0){
                    usleep(attente * 1000);
                    afficherLabyUTF8(laby);
                }
            }
            /* Ou affichage avec attente de saisie au clavier. */
            else{
                getchar();
                afficherLabyUTF8(laby);
            }
        }
        /* Affichage du labyrinthe valide. */
        afficherLabyUTF8(laby);
        fin = clock();
        tempsExec = ((double) (fin - debut)) / CLOCKS_PER_SEC;
        printf("temps d'execution du programme : %f\n", tempsExec);

    }

    /* Affichage sous forme graphique. */
    else{

        /* Création de la fenêtre et affichage sur fenêtre du labyrinthe initial. */      
        MLV_create_window("labyrinthe","", LARGEUR, HAUTEUR);
        dessineLaby(laby);
        MLV_actualise_window();

        /* Boucle principale de la generation d'un labyrinthe valide. */
        while (!bonLaby(laby, acces)){

            /* Effacement d'un mur aléatoire, avec l'algorithme choisi. */
            if (fast)
                indice = supprime_mur_fast(&laby, tableauMur, indice, unique);
            else
                supprime_mur(&laby, unique);

            /* Affichage du labyrinthe avec temps d'attente. */
            if (choix_attente){
                if(attente != 0){
                    usleep(attente * 1000);
                    MLV_clear_window(MLV_COLOR_BLACK);/* Nettoyage de la fenêtre. */
                    dessineLaby(laby);/* On dessine le labyrinthe dans la fenêtre. */
                    MLV_actualise_window();/* Mise à jour de la fenêtre. */
                }
            }
            /* Ou affichage avec attente de saisie au clavier. */
            else{
                MLV_wait_keyboard( &touche, NULL, NULL );
                MLV_clear_window(MLV_COLOR_BLACK);
                dessineLaby(laby);
                MLV_actualise_window();
            }
        }
        /*Affichage du labyrinthe valide*/ 
        MLV_clear_window(MLV_COLOR_BLACK);
        dessineLaby(laby);
        MLV_actualise_window();

        /* Fermeture de la fenêtre s'il y a une saisie au clavier. */
        fin = clock();
        tempsExec = ((double) (fin - debut)) / CLOCKS_PER_SEC;
        printf("temps d'execution du programme : %f\n", tempsExec);
        MLV_wait_keyboard( &touche, NULL, NULL );
        MLV_free_window();
    }

    /* Libération de l'espace mémoire occupée par les cases du labyrinthe. */
    libererLaby_t(&laby);
    
    /* Et éventuellement du tableau. */
    if(fast)
        free(tableauMur);

    return 0;
}