#include <stdio.h>
#include "file.c"
#include "memory.c"

void afficherMenuPrincipal() {
    printf("\n========== Menu Principal =========\n");
    printf("1. Initialiser la mémoire secondaire\n");
    printf("2. Créer et charger un fichier\n");
    printf("3. Afficher l'état de la mémoire secondaire\n");
    printf("4. Afficher les métadonnées des fichiers\n");
    printf("5. Rechercher un enregistrement\n");
    printf("6. Insérer un nouvel enregistrement\n");
    printf("7. Supprimer un enregistrement (logique)\n");
    printf("8. Supprimer un enregistrement (physique)\n");
    printf("9. Défragmenter un fichier\n");
    printf("10. Supprimer un fichier\n");
    printf("11. Renommer un fichier\n");
    printf("12. Compactage de la mémoire secondaire\n");
    printf("13. Vider la mémoire secondaire\n");
    printf("14. Quitter\n");
    printf("===================================\n");
}

int main() {
    int choix;
    initialiserMemoireSecondaire(); // Initialise la mémoire secondaire au démarrage

    do {
        afficherMenuPrincipal();
        printf("\nEntrez votre choix : ");
        scanf("%d", &choix);

        switch (choix) {
            case 1:
                initialiserMemoireSecondaire();
                printf("\nMémoire secondaire initialisée avec succès.\n");
                break;

            case 2:
                creerFichier();
                printf("\nFichier créé et chargé en mémoire secondaire.\n");
                break;

            case 3:
                afficherEtatMemoire();
                break;

            case 4:
                afficherMetadonneesFichier();
                break;

            case 5:
                rechercherEnregistrement();
                break;

            case 6:
                insererEnregistrement();
                break;

            case 7:
                SuppressionLogique();
                break;

            case 8:
                SuppressionPhysique();
                break;

            case 9:
                Defragmentation();
                break;

            case 10:
                supprimerFichier();
                break;

            case 11:
                renommerFichier();
                break;

            case 12:
                compacterContMS();
                printf("\nCompactage de la mémoire secondaire effectué.\n");
                break;

            case 13:
                viderMemoireSecondaire();
                printf("\nMémoire secondaire vidée avec succès.\n");
                break;

            case 14:
                printf("\nMerci d'avoir utilisé le simulateur !\n");
                break;

            default:
                printf("\nChoix invalide. Veuillez réessayer.\n");
                break;
        }
    } while (choix != 14);

    return 0;
}

	
