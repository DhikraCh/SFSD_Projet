#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NOM_TAILLE 20
#define NB_BLOCS 10
#define FB 3

typedef struct {
    char nomFichier[50];          
    int tailleEnBlocs;            
    int tailleEnEnregistrements;  
    int premierBloc;              
    char organisationGlobale[20]; 
    char organisationInterne[20]; 
} Fichier;

typedef struct {
    int ID;
    char Nom[NOM_TAILLE];
    int logicDel;
} Enregistrement;

// Représentation contiguë de bloc
typedef struct {
    Enregistrement Bloc[FB];
    int nbEnregistrement; // Nombre actuel d'enregistrement dans le bloc
    int numBloc;          // Numéro du bloc
} ContBloc;

// Représentation chaînée de bloc
typedef struct ListBloc {
    Enregistrement Bloc[FB];
    int nbEnregistrement; // Nombre actuel d'enregistrement dans le bloc
    int numBloc;          // Numéro du bloc
    struct ListBloc *nextB;
} ListBloc;

// Métadonnées
typedef struct {
    char Nom[NOM_TAILLE];
    int TailleEnBlocs;
    int TailleEnEng;
    int FirstBlocIndex;
    int ModeOrgGlobal;
    int ModeOrgIntern;
} MetaDonnees;

// Table d'allocation
typedef struct {
    int nbBlocs;           // Nombre total de blocs
    int libres;            // Nombre de blocs libres
    int blocs[NB_BLOCS];   // État de chaque bloc (1 = libre, 0 = occupé)
} TableDAllocation;

void afficherMetadonneesFichier(Fichier fichier) {
    printf("Métadonnées du fichier '%s':\n", fichier.nomFichier);
    printf("Taille en blocs          : %d\n", fichier.tailleEnBlocs);
    printf("Taille en enregistrements: %d\n", fichier.tailleEnEnregistrements);
    printf("Premier bloc             : %d\n", fichier.premierBloc);
    printf("Organisation globale     : %s\n", fichier.organisationGlobale);
    printf("Organisation interne     : %s\n", fichier.organisationInterne);
    printf("-----------------------------\n");
}

void CreerFichier(FILE **MS, TableDAllocation *table, MetaDonnees *meta) {
    printf("Nom du fichier : ");
    scanf("%s", meta->Nom);

    printf("Nombre d'enregistrements : ");
    int nbEng;
    scanf("%d", &nbEng);

    printf("Mode d'organisation globale (1: Contiguë, 2: Chaînée) : ");
    scanf("%d", &meta->ModeOrgGlobal);

    printf("Mode d'organisation interne (1: Ordonné, 2: Non ordonné) : ");
    scanf("%d", &meta->ModeOrgIntern);

    meta->TailleEnEng = nbEng;
    meta->TailleEnBlocs = (nbEng + FB - 1) / FB; // Calcul du nombre de blocs nécessaires

    if (meta->TailleEnBlocs > table->libres) {
        printf("Espace insuffisant pour créer le fichier. Tentative de compactage...\n");
        return ;
    // Allocation des blocs selon le mode d'organisation, mais sans allouer dynamiquement ici
    if (meta->ModeOrgGlobal == 1) {
        // Mode contigu
        int premierBloc = -1;
        for (int i = 0, blocsAlloues = 0; i < table->nbBlocs && blocsAlloues < meta->TailleEnBlocs; i++) {
            if (table->blocs[i] == 1) {
                if (premierBloc == -1) {
                    premierBloc = i;
                }
                marquerBlocOccupe(table, i);
                blocsAlloues++;
            }
        }

        meta->FirstBlocIndex = premierBloc;
        printf("Fichier %s créé avec succès en mode contigu.\n", meta->Nom);
    } else if (meta->ModeOrgGlobal == 2) {
        // Mode chaîné
        int premierBloc = -1;
        int dernierBloc = -1; // Pour chaîner les blocs
        for (int i = 0, blocsAlloues = 0; i < table->nbBlocs && blocsAlloues < meta->TailleEnBlocs; i++) {
            if (table->blocs[i] == 1) {
                if (premierBloc == -1) {
                    premierBloc = i;
                }
                if (dernierBloc != -1) {
                    // Chaîner les blocs
                    ListBloc bloc;
                    fseek(*MS, dernierBloc * sizeof(ListBloc), SEEK_SET);
                    fread(&bloc, sizeof(ListBloc), 1, *MS);
                    bloc.nextB = (ListBloc *)malloc(sizeof(ListBloc));
                    bloc.nextB->numBloc = i;
                    bloc.nextB->nextB = NULL;
                    fseek(*MS, dernierBloc * sizeof(ListBloc), SEEK_SET);
                    fwrite(&bloc, sizeof(ListBloc), 1, *MS);
                }
                marquerBlocOccupe(table, i);
                blocsAlloues++;
                dernierBloc = i;
            }
        }

        // Marquer le dernier bloc de la chaîne
        if (dernierBloc != -1) {
            ListBloc bloc;
            fseek(*MS, dernierBloc * sizeof(ListBloc), SEEK_SET);
            fread(&bloc, sizeof(ListBloc), 1, *MS);
            bloc.nextB = NULL;
            fseek(*MS, dernierBloc * sizeof(ListBloc), SEEK_SET);
            fwrite(&bloc, sizeof(ListBloc), 1, *MS);
        }

        meta->FirstBlocIndex = premierBloc;
        printf("Fichier %s créé avec succès en mode chaîné.\n", meta->Nom);
    }
}


void ChargerFichier(FILE **MS, TableDAllocation *table, MetaDonnees *meta) {
    if (meta->FirstBlocIndex == -1) {
        printf("Erreur : Les blocs du fichier n'ont pas été alloués. Veuillez d'abord créer le fichier.\n");
        return;
    }

    // Simulation du chargement en mémoire secondaire
    if (meta->ModeOrgGlobal == 1) {
        // Mode contigu
        ContBloc *blocsContigus = (ContBloc *)malloc(meta->TailleEnBlocs * sizeof(ContBloc));
        if (blocsContigus == NULL) {
            printf("Erreur d'allocation mémoire.\n");
            return;
        }

        for (int i = 0; i < meta->TailleEnBlocs; i++) {
            fseek(*MS, (meta->FirstBlocIndex + i) * sizeof(ContBloc), SEEK_SET);
            fread(&blocsContigus[i], sizeof(ContBloc), 1, *MS);
            printf("Chargement du bloc %d dans le mode contigu.\n", blocsContigus[i].numBloc);
        }

        // Libérer la mémoire allouée pour les blocs
        free(blocsContigus);
    } else if (meta->ModeOrgGlobal == 2) {
        // Mode chaîné
        ListBloc *firstBloc = NULL, *lastBloc = NULL;
        int indexBloc = meta->FirstBlocIndex;

        while (indexBloc != -1) {
            ListBloc *bloc = (ListBloc *)malloc(sizeof(ListBloc));
            if (bloc == NULL) {
                printf("Erreur d'allocation mémoire.\n");
                return;
            }

            // Charger le bloc à partir de la mémoire secondaire
            fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
            fread(bloc, sizeof(ListBloc), 1, *MS);
            printf("Chargement du bloc %d dans le mode chaîné.\n", bloc->numBloc);

            // Chaîner les blocs
            if (lastBloc == NULL) {
                firstBloc = bloc;  // Premier bloc
            } else {
                lastBloc->nextB = bloc;  // Lier le bloc précédent au bloc actuel
            }
            lastBloc = bloc;  // Mettre à jour le dernier bloc
            indexBloc = (bloc->nextB != NULL) ? bloc->nextB->numBloc : -1;  // Passer au bloc suivant
        }

        // Libérer la mémoire allouée pour les blocs chaînés
        ListBloc *tempBloc;
        while (firstBloc != NULL) {
            tempBloc = firstBloc;
            firstBloc = firstBloc->nextB;
            free(tempBloc);
        }
    }

    printf("Fichier %s chargé avec succès.\n", meta->Nom);
}


void InsererEnregistrement(FILE **MS, TableDAllocation *table, MetaDonnees *meta, Enregistrement *enreg) {
    if (meta->ModeOrgGlobal == 1) {  // Mode contigu
        for (int i = meta->FirstBlocIndex; i < meta->FirstBlocIndex + meta->TailleEnBlocs; i++) {
            if (table->blocs[i] == 1) {
                // Le bloc est libre, insérer l'enregistrement ici
                ContBloc bloc;
                fseek(*MS, i * sizeof(ContBloc), SEEK_SET);
                fread(&bloc, sizeof(ContBloc), 1, *MS);

                if (bloc.nbEnregistrement < FB) {
                    if (meta->ModeOrgIntern == 1) {
                        // Ordonné, insérer dans l'ordre
                        int j;
                        for (j = bloc.nbEnregistrement - 1; j >= 0 && bloc.Bloc[j].ID > enreg->ID; j--) {
                            bloc.Bloc[j + 1] = bloc.Bloc[j];
                        }
                        bloc.Bloc[j + 1] = *enreg;
                    } else {
                        // Non ordonné, ajouter à la fin
                        bloc.Bloc[bloc.nbEnregistrement] = *enreg;
                    }
                    bloc.nbEnregistrement++;
                    fseek(*MS, i * sizeof(ContBloc), SEEK_SET);
                    fwrite(&bloc, sizeof(ContBloc), 1, *MS);
                    printf("Enregistrement inséré avec succès dans le bloc %d.\n", i);
                    return;
                }
            }
        }
        printf("Erreur : Aucun bloc disponible pour l'insertion.\n");
    }
    else if (meta->ModeOrgGlobal == 2) {  // Mode chaîné
        int indexBloc = meta->FirstBlocIndex;
        ListBloc *lastBloc = NULL;

        while (indexBloc != -1) {
            ListBloc bloc;
            fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
            fread(&bloc, sizeof(ListBloc), 1, *MS);

            if (bloc.nbEnregistrement < FB) {
                if (meta->ModeOrgIntern == 1) {
                    // Ordonné, insérer dans l'ordre
                    int j;
                    for (j = bloc.nbEnregistrement - 1; j >= 0 && bloc.Bloc[j].ID > enreg->ID; j--) {
                        bloc.Bloc[j + 1] = bloc.Bloc[j];
                    }
                    bloc.Bloc[j + 1] = *enreg;
                } else {
                    // Non ordonné, ajouter à la fin
                    bloc.Bloc[bloc.nbEnregistrement] = *enreg;
                }
                bloc.nbEnregistrement++;
                fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
                fwrite(&bloc, sizeof(ListBloc), 1, *MS);
                printf("Enregistrement inséré dans le bloc chaîné %d.\n", indexBloc);
                return;
            }

            lastBloc = &bloc;
            if (bloc.nextB != NULL) {
    indexBloc = bloc.nextB->numBloc;  // Passer au bloc suivant
} else {
    indexBloc = -1;  // Aucun bloc suivant, on termine la boucle
}


        // Si le dernier bloc est plein, allouer un nouveau bloc et chaîner
        ListBloc *nouveauBloc = (ListBloc *)malloc(sizeof(ListBloc));
        if (nouveauBloc == NULL) {
            printf("Erreur d'allocation mémoire.\n");
            return;
        }

        nouveauBloc->numBloc = table->libres;
        nouveauBloc->nbEnregistrement = 1;
        if (meta->ModeOrgIntern == 1) {
            // Ordonné, insérer dans l'ordre
            nouveauBloc->Bloc[0] = *enreg;
        } else {
            // Non ordonné, ajouter à la fin
            nouveauBloc->Bloc[0] = *enreg;
        }
        nouveauBloc->nextB = NULL;

        fseek(*MS, table->libres * sizeof(ListBloc), SEEK_SET);
        fwrite(nouveauBloc, sizeof(ListBloc), 1, *MS);
        printf("Enregistrement inséré dans un nouveau bloc chaîné.\n");

        if (lastBloc != NULL) {
            lastBloc->nextB = nouveauBloc;
            fseek(*MS, table->libres * sizeof(ListBloc), SEEK_SET);
            fwrite(lastBloc, sizeof(ListBloc), 1, *MS);
        }

        free(nouveauBloc);  // Libérer la mémoire du nouveau bloc alloué
    }



void RechercheEnregistrement(FILE **MS, TableDAllocation *table, MetaDonnees *meta, int idRecherche) {
    if (meta->FirstBlocIndex == -1) {
        printf("Erreur : Les blocs du fichier n'ont pas été alloués. Veuillez d'abord créer le fichier.\n");
        return;
    }

    int indexBloc = meta->FirstBlocIndex;

    if (meta->ModeOrgGlobal == 1) {
        // Mode contigu
        while (indexBloc != -1) {
            ContBloc bloc;
            fseek(*MS, indexBloc * sizeof(ContBloc), SEEK_SET);
            fread(&bloc, sizeof(ContBloc), 1, *MS);

            // Recherche d'enregistrement dans un bloc contigu
            if (meta->ModeOrgIntern == 1) {
                // Mode interne ordonné - Recherche dichotomique
                int left = 0, right = bloc.nbEnregistrement - 1;
                while (left <= right) {
                    int mid = left + (right - left) / 2;

                    if (bloc.Bloc[mid].ID == idRecherche) {
                        printf("Enregistrement trouvé dans le bloc %d (Mode contigu, ordonné), position %d.\n", indexBloc, mid);
                        return;
                    }
                    if (bloc.Bloc[mid].ID < idRecherche) {
                        left = mid + 1;
                    } else {
                        right = mid - 1;
                    }
                }
            } else {
                // Mode interne non ordonné - Recherche linéaire
                for (int i = 0; i < bloc.nbEnregistrement; i++) {
                    if (bloc.Bloc[i].ID == idRecherche) {
                        printf("Enregistrement trouvé dans le bloc %d (Mode contigu, non ordonné), position %d.\n", indexBloc, i);
                        return;
                    }
                }
            }

            indexBloc++; // Passer au bloc suivant
        }
    } else if (meta->ModeOrgGlobal == 2) {
        // Mode chaîné
        ListBloc *currentBloc = NULL;
        while (indexBloc != -1) {
            ListBloc bloc;
            fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
            fread(&bloc, sizeof(ListBloc), 1, *MS);

            currentBloc = &bloc;

            // Recherche d'enregistrement dans un bloc chaîné
            for (int i = 0; i < currentBloc->nbEnregistrement; i++) {
                if (currentBloc->Bloc[i].ID == idRecherche) {
                    printf("Enregistrement trouvé dans le bloc chaîné %d, position %d.\n", indexBloc, i);
                    return;
                }
            }

            // Passer au bloc suivant
            indexBloc = (currentBloc->nextB != NULL) ? currentBloc->nextB->numBloc : -1;
        }
    }

    printf("Enregistrement avec ID %d non trouvé.\n", idRecherche);
}


void SuppressionLogique(FILE **MS, TableDAllocation *table, MetaDonnees *meta, int idSuppression) {
    if (meta->FirstBlocIndex == -1) {
        printf("Erreur : Les blocs du fichier n'ont pas été alloués. Veuillez d'abord créer le fichier.\n");
        return;
    }

    int indexBloc = meta->FirstBlocIndex;
    int found = 0;

    if (meta->ModeOrgGlobal == 1) {
        // Mode contigu
        while (indexBloc != -1) {
            ContBloc bloc;
            fseek(*MS, indexBloc * sizeof(ContBloc), SEEK_SET);
            fread(&bloc, sizeof(ContBloc), 1, *MS);

            // Recherche de l'enregistrement à supprimer
            for (int i = 0; i < bloc.nbEnregistrement; i++) {
                if (bloc.Bloc[i].ID == idSuppression) {
                    // Logique : Marquer comme supprimé
                    printf("Enregistrement trouvé et marqué comme supprimé (logique) dans le bloc %d, position %d.\n", indexBloc, i);
                    bloc.Bloc[i].logicDel = 1;  // Marquer comme supprimé
                    fseek(*MS, indexBloc * sizeof(ContBloc), SEEK_SET);
                    fwrite(&bloc, sizeof(ContBloc), 1, *MS);
                    found = 1;
                    break;
                }
            }

            if (found) break;
            indexBloc++;  // Passer au bloc suivant
        }
    } else if (meta->ModeOrgGlobal == 2) {
        // Mode chaîné
        ListBloc *currentBloc = NULL;
        while (indexBloc != -1) {
            ListBloc bloc;
            fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
            fread(&bloc, sizeof(ListBloc), 1, *MS);

            currentBloc = &bloc;

            // Recherche de l'enregistrement à supprimer
            for (int i = 0; i < currentBloc->nbEnregistrement; i++) {
                if (currentBloc->Bloc[i].ID == idSuppression) {
                    // Logique : Marquer comme supprimé
                    printf("Enregistrement trouvé et marqué comme supprimé (logique) dans le bloc chaîné %d, position %d.\n", indexBloc, i);
                    currentBloc->Bloc[i].logicDel = 1;  // Marquer comme supprimé
                    fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
                    fwrite(&currentBloc, sizeof(ListBloc), 1, *MS);
                    found = 1;
                    break;
                }
            }

            if (found) break;
            if (currentBloc->nextB != NULL) {
               indexBloc = currentBloc->nextB->numBloc; } // Passer au bloc suivant
          else {
            indexBloc = -1; } // Fin de la chaîne, aucun bloc suivant


        }
    }

    if (!found) {
        printf("Enregistrement avec ID %d non trouvé.\n", idSuppression);
    } else {
        printf("Suppression logique réussie.\n");
    }
}


void SuppressionPhysique(FILE **MS, TableDAllocation *table, MetaDonnees *meta, int idSuppression) {
    if (meta->FirstBlocIndex == -1) {
        printf("Erreur : Les blocs du fichier n'ont pas été alloués. Veuillez d'abord créer le fichier.\n");
        return;
    }

    if (meta->ModeOrgGlobal == 1) {
        // Mode contigu
        int indexBloc = meta->FirstBlocIndex;

        while (indexBloc < meta->FirstBlocIndex + meta->TailleEnBlocs) {
            ContBloc bloc;
            fseek(*MS, indexBloc * sizeof(ContBloc), SEEK_SET);
            fread(&bloc, sizeof(ContBloc), 1, *MS);

            int position = -1;

            if (meta->ModeOrgIntern == 1) {
                // Organisation interne ordonnée : recherche dichotomique
                int left = 0, right = bloc.nbEnregistrement - 1;
                while (left <= right) {
                    int mid = left + (right - left) / 2;
                    if (bloc.Bloc[mid].ID == idSuppression) {
                        position = mid;
                        break;
                    }
                    if (bloc.Bloc[mid].ID < idSuppression) {
                        left = mid + 1;
                    } else {
                        right = mid - 1;
                    }
                }
            } else {
                // Organisation interne non ordonnée : recherche linéaire
                for (int i = 0; i < bloc.nbEnregistrement; i++) {
                    if (bloc.Bloc[i].ID == idSuppression) {
                        position = i;
                        break;
                    }
                }
            }

            if (position != -1) {
                printf("Enregistrement ID %d trouvé dans le bloc contigu %d à la position %d.\n", idSuppression, indexBloc, position);

                // Réorganisation des enregistrements
                for (int i = position; i < bloc.nbEnregistrement - 1; i++) {
                    bloc.Bloc[i] = bloc.Bloc[i + 1];
                }
                bloc.nbEnregistrement--;

                // Si le bloc devient vide
                if (bloc.nbEnregistrement == 0) {
                    table->blocs[indexBloc] = 1; // Marquer le bloc comme libre
                    table->libres++;
                    printf("Bloc contigu %d devenu vide et marqué comme libre.\n", indexBloc);
                } else {
                    // Sinon, sauvegarder le bloc mis à jour
                    fseek(*MS, indexBloc * sizeof(ContBloc), SEEK_SET);
                    fwrite(&bloc, sizeof(ContBloc), 1, *MS);
                }

                printf("Enregistrement supprimé physiquement dans le mode contigu.\n");
                return;
            }

            indexBloc++;
        }

        printf("Enregistrement avec ID %d non trouvé dans le mode contigu.\n", idSuppression);
    } else if (meta->ModeOrgGlobal == 2) {
        // Mode chaîné
        int prevBlocIndex = -1;
        int indexBloc = meta->FirstBlocIndex;

        while (indexBloc != -1) {
            ListBloc currentBloc;

            // Lire le bloc actuel
            fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
            fread(&currentBloc, sizeof(ListBloc), 1, *MS);

            // Recherche de l'enregistrement à supprimer
            for (int i = 0; i < currentBloc.nbEnregistrement; i++) {
                if (currentBloc.Bloc[i].ID == idSuppression) {
                    printf("Enregistrement ID %d trouvé dans le bloc chaîné %d à la position %d.\n", idSuppression, indexBloc, i);

                    // Décaler les enregistrements si ce n'est pas le dernier
                    for (int j = i; j < currentBloc.nbEnregistrement - 1; j++) {
                        currentBloc.Bloc[j] = currentBloc.Bloc[j + 1];
                    }

                    currentBloc.nbEnregistrement--;

                    // Si le bloc devient vide
                    if (currentBloc.nbEnregistrement == 0) {
                        table->blocs[indexBloc] = 1; // Marquer le bloc comme libre
                        table->libres++;
                        printf("Bloc chaîné %d devenu vide et marqué comme libre.\n", indexBloc);

                        // Ajuster le chaînage
                        if (prevBlocIndex != -1) {
                            // Si le bloc précédent existe, mettre à jour le chaînage
                            ListBloc prevBloc;
                            fseek(*MS, prevBlocIndex * sizeof(ListBloc), SEEK_SET);
                            fread(&prevBloc, sizeof(ListBloc), 1, *MS);

                            prevBloc.nextB = currentBloc.nextB; // Le bloc précédent pointe sur le suivant
                            fseek(*MS, prevBlocIndex * sizeof(ListBloc), SEEK_SET);
                            fwrite(&prevBloc, sizeof(ListBloc), 1, *MS);
                        } else {
                            // Cas spécial : si le bloc supprimé est le premier
                            meta->FirstBlocIndex = (currentBloc.nextB != NULL) ? currentBloc.nextB->numBloc : -1;
                            printf("Mise à jour du FirstBlocIndex : %d\n", meta->FirstBlocIndex);
                        }
                    } else {
                        // Sinon, sauvegarder le bloc mis à jour
                        fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
                        fwrite(&currentBloc, sizeof(ListBloc), 1, *MS);
                    }

                    printf("Enregistrement supprimé physiquement dans le mode chaîné.\n");
                    return;
                }
            }

            // Passer au bloc suivant
            prevBlocIndex = indexBloc;
            indexBloc = (currentBloc.nextB != NULL) ? currentBloc.nextB->numBloc : -1;
        }

        printf("Enregistrement avec ID %d non trouvé dans le mode chaîné.\n", idSuppression);
    }
}


void Defragmentation(FILE **MS, TableDAllocation *table, MetaDonnees *meta) {
    if (meta->FirstBlocIndex == -1) {
        printf("Erreur : Les blocs du fichier n'ont pas été alloués. Veuillez d'abord créer le fichier.\n");
        return;
    }

    int indexBloc = meta->FirstBlocIndex;
    int newIndexBloc = 0;  // Nouvelle position pour les enregistrements réorganisés
    int totalEnregistrements = 0;

    if (meta->ModeOrgGlobal == 1) {
        // Mode contigu
        ContBloc bloc;
        ContBloc nouveauBloc;
        nouveauBloc.nbEnregistrement = 0;

        while (indexBloc < meta->FirstBlocIndex + meta->TailleEnBlocs) {
            fseek(*MS, indexBloc * sizeof(ContBloc), SEEK_SET);
            fread(&bloc, sizeof(ContBloc), 1, *MS);

            for (int i = 0; i < bloc.nbEnregistrement; i++) {
                if (bloc.Bloc[i].logicDel == 0) {  // Enregistrement valide
                    nouveauBloc.Bloc[nouveauBloc.nbEnregistrement++] = bloc.Bloc[i];
                    totalEnregistrements++;

                    if (nouveauBloc.nbEnregistrement == MAX_ENREGISTREMENTS_PAR_BLOC) {
                        // Sauvegarder le bloc rempli
                        fseek(*MS, newIndexBloc * sizeof(ContBloc), SEEK_SET);
                        fwrite(&nouveauBloc, sizeof(ContBloc), 1, *MS);
                        table->blocs[newIndexBloc] = 0;  // Marquer le bloc comme occupé
                        nouveauBloc.nbEnregistrement = 0;
                        newIndexBloc++;
                    }
                }
            }

            // Marquer l'ancien bloc comme libre
            table->blocs[indexBloc] = 1;
            table->libres++;

            indexBloc++;
        }

        // Sauvegarder les enregistrements restants dans un dernier bloc
        if (nouveauBloc.nbEnregistrement > 0) {
            fseek(*MS, newIndexBloc * sizeof(ContBloc), SEEK_SET);
            fwrite(&nouveauBloc, sizeof(ContBloc), 1, *MS);
            table->blocs[newIndexBloc] = 0;  // Marquer le bloc comme occupé
            newIndexBloc++;
        }

        // Mettre à jour les métadonnées
        meta->FirstBlocIndex = 0;
        meta->TailleEnBlocs = newIndexBloc;

        printf("Défragmentation terminée. Total d'enregistrements réorganisés : %d.\n", totalEnregistrements);

    } else if (meta->ModeOrgGlobal == 2) {
        // Mode chaîné
        ListBloc bloc;
        ListBloc nouveauBloc;
        nouveauBloc.nbEnregistrement = 0;
        nouveauBloc.nextB = NULL;

        int prevBlocIndex = -1;

        while (indexBloc != -1) {
            fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
            fread(&bloc, sizeof(ListBloc), 1, *MS);

            for (int i = 0; i < bloc.nbEnregistrement; i++) {
                if (bloc.Bloc[i].logicDel == 0) {  // Enregistrement valide
                    nouveauBloc.Bloc[nouveauBloc.nbEnregistrement++] = bloc.Bloc[i];
                    totalEnregistrements++;

                    if (nouveauBloc.nbEnregistrement == MAX_ENREGISTREMENTS_PAR_BLOC) {
                        // Sauvegarder le bloc rempli
                        fseek(*MS, newIndexBloc * sizeof(ListBloc), SEEK_SET);
                        fwrite(&nouveauBloc, sizeof(ListBloc), 1, *MS);
                        table->blocs[newIndexBloc] = 0;  // Marquer le bloc comme occupé

                        if (prevBlocIndex != -1) {
                            // Mettre à jour le chaînage
                            ListBloc prevBloc;
                            fseek(*MS, prevBlocIndex * sizeof(ListBloc), SEEK_SET);
                            fread(&prevBloc, sizeof(ListBloc), 1, *MS);
                            prevBloc.nextB = &nouveauBloc;
                            fseek(*MS, prevBlocIndex * sizeof(ListBloc), SEEK_SET);
                            fwrite(&prevBloc, sizeof(ListBloc), 1, *MS);
                        } else {
                            meta->FirstBlocIndex = newIndexBloc;
                        }

                        prevBlocIndex = newIndexBloc;
                        nouveauBloc.nbEnregistrement = 0;
                        nouveauBloc.nextB = NULL;
                        newIndexBloc++;
                    }
                }
            }

            // Marquer l'ancien bloc comme libre
            table->blocs[indexBloc] = 1;
            table->libres++;

            indexBloc = (bloc.nextB != NULL) ? bloc.nextB->numBloc : -1;
        }

        // Sauvegarder les enregistrements restants dans un dernier bloc
        if (nouveauBloc.nbEnregistrement > 0) {
            fseek(*MS, newIndexBloc * sizeof(ListBloc), SEEK_SET);
            fwrite(&nouveauBloc, sizeof(ListBloc), 1, *MS);
            table->blocs[newIndexBloc] = 0;  // Marquer le bloc comme occupé

            if (prevBlocIndex != -1) {
                // Mettre à jour le chaînage
                ListBloc prevBloc;
                fseek(*MS, prevBlocIndex * sizeof(ListBloc), SEEK_SET);
                fread(&prevBloc, sizeof(ListBloc), 1, *MS);
                prevBloc.nextB = &nouveauBloc;
                fseek(*MS, prevBlocIndex * sizeof(ListBloc), SEEK_SET);
                fwrite(&prevBloc, sizeof(ListBloc), 1, *MS);
            } else {
                meta->FirstBlocIndex = newIndexBloc;
            }

            newIndexBloc++;
        }

        printf("Défragmentation terminée. Total d'enregistrements réorganisés : %d.\n", totalEnregistrements);
    }
}



void RenommerFichier(MetaDonnees *meta, const char *nouveauNom) {
    // Vérifier que le nouveau nom est valide (pas vide)
    if (nouveauNom == NULL || strlen(nouveauNom) == 0) {
        printf("Erreur : Le nouveau nom de fichier est invalide.\n");
        return;
    }

    // Vérifier que le nom n'est pas trop long
    if (strlen(nouveauNom) >= NOM_TAILLE) {
        printf("Erreur : Le nouveau nom de fichier est trop long.\n");
        return;
    }

    // Mettre à jour le nom du fichier dans les métadonnées
    strcpy(meta->Nom, nouveauNom);

    // Vous pouvez ajouter ici une logique pour sauvegarder les métadonnées mises à jour dans un fichier.
    // Exemple : fwrite(meta, sizeof(MetaDonnees), 1, fichierMeta);

    printf("Le fichier a été renommé en : %s\n", meta->Nom);
}



// Fonction pour afficher les blocs (mode contigu)
void afficherBlocs(ContBloc blocs[], int nbBlocs) {
    for (int i = 0; i < nbBlocs; i++) {
        printf("Bloc %d (%d enregistrements) : ", blocs[i].numBloc, blocs[i].nbEnregistrement);
        for (int j = 0; j < blocs[i].nbEnregistrement; j++) {
            if (!blocs[i].Bloc[j].logicDel) {
                printf("[%d: %s] ", blocs[i].Bloc[j].ID, blocs[i].Bloc[j].Nom);
            }
        }
        printf("\n");
    }
}

// Fonction de défragmentation (mode contigu)
void defragmenterBlocs(ContBloc blocs[], int nbBlocs, TableDAllocation *tableAlloc) {
    Enregistrement tampon[NB_BLOCS * FB]; // Tampon pour les enregistrements valides
    int compteur = 0; // Compteur d'enregistrements valides

    // Étape 1 : Collecter les enregistrements valides
    for (int i = 0; i < nbBlocs; i++) {
        for (int j = 0; j < blocs[i].nbEnregistrement; j++) {
            if (!blocs[i].Bloc[j].logicDel) { // Si l'enregistrement est valide
                tampon[compteur] = blocs[i].Bloc[j];
                compteur++;
            }
        }
        // Marquer le bloc comme libre dans la table d'allocation
        tableAlloc->blocs[blocs[i].numBloc] = 1;
    }

    // Étape 2 : Réorganiser les enregistrements dans les blocs
    int indexEnregistrement = 0;
    for (int i = 0; i < nbBlocs && indexEnregistrement < compteur; i++) {
        blocs[i].nbEnregistrement = 0; // Réinitialiser le nombre d'enregistrements dans le bloc
        for (int j = 0; j < FB && indexEnregistrement < compteur; j++) {
            blocs[i].Bloc[j] = tampon[indexEnregistrement];
            blocs[i].Bloc[j].logicDel = 0; // Marquer comme valide
            indexEnregistrement++;
            blocs[i].nbEnregistrement++;
        }
        tableAlloc->blocs[blocs[i].numBloc] = 0; // Marquer le bloc comme occupé
    }

    // Mettre à jour les blocs restants comme vides
    for (int i = indexEnregistrement / FB; i < nbBlocs; i++) {
        blocs[i].nbEnregistrement = 0;
        for (int j = 0; j < FB; j++) {
            blocs[i].Bloc[j].logicDel = 1; // Marquer les enregistrements comme supprimés
        }
    }

    printf("Défragmentation terminée !\n");
}

//**************************************** Fonction principale ****************************************//


void defragmenterChainee(ListBloc *blocs, TableDAllocation *tableAlloc) {
    Enregistrement tampon[NB_BLOCS * FB];  // Tampon pour les enregistrements valides
    int compteur = 0;                      // Compteur d'enregistrements valides

    // Étape 1 : Collecter les enregistrements valides
    ListBloc *courant = blocs;
    while (courant != NULL) {
        for (int j = 0; j < courant->nbEnregistrement; j++) {
            if (!courant->Bloc[j].logicDel) { // Si l'enregistrement est valide
                tampon[compteur] = courant->Bloc[j];
                compteur++;
            }
        }
        // Marquer le bloc comme libre dans la table d'allocation
        tableAlloc->blocs[courant->numBloc] = 1;
        courant = courant->nextB;  // Passer au bloc suivant
    }

    // Étape 2 : Réorganiser les enregistrements dans les blocs chaînés
    ListBloc *dernierBlocLibre = NULL;  // Pointeur vers le dernier bloc libre
    int indexEnregistrement = 0;
    courant = blocs;

    while (courant != NULL && indexEnregistrement < compteur) {
        courant->nbEnregistrement = 0;  // Réinitialiser le nombre d'enregistrements dans le bloc

        // Réinsérer les enregistrements valides dans le bloc courant
        for (int j = 0; j < FB && indexEnregistrement < compteur; j++) {
            courant->Bloc[j] = tampon[indexEnregistrement];
            courant->Bloc[j].logicDel = 0;  // Marquer comme valide
            indexEnregistrement++;
            courant->nbEnregistrement++;
        }

        // Marquer le bloc comme occupé dans la table d'allocation
        tableAlloc->blocs[courant->numBloc] = 0;

        // Lier le bloc précédent au bloc courant si nécessaire
        if (dernierBlocLibre != NULL) {
            dernierBlocLibre->nextB = courant;
        }

        // Mettre à jour le dernier bloc libre
        dernierBlocLibre = courant;

        courant = courant->nextB;  // Passer au bloc suivant
    }

    // Mettre à jour les blocs restants comme vides
    while (courant != NULL) {
        courant->nbEnregistrement = 0;
        for (int j = 0; j < FB; j++) {
            courant->Bloc[j].logicDel = 1;  // Marquer les enregistrements comme supprimés
        }
        tableAlloc->blocs[courant->numBloc] = 1;  // Marquer le bloc comme libre
        courant = courant->nextB;  // Passer au bloc suivant
    }

    // Finaliser la chaîne en marquant le dernier bloc comme n'ayant pas de suivant
    if (dernierBlocLibre != NULL) {
        dernierBlocLibre->nextB = NULL;
    }

    printf("Défragmentation chaînée terminée !\n");
}
void SupprimerFichierPhysiquement(FILE **MS, TableDAllocation *table, MetaDonnees *meta) {
    // Vérifier si le fichier existe
    if (meta->FirstBlocIndex == -1) {
        printf("Erreur : Le fichier n'existe pas ou a déjà été supprimé.\n");
        return;
    }

    printf("Suppression physique du fichier : %s\n", meta->Nom);

    if (meta->ModeOrgIntern == 1) { // Fichier ordonné
        if (meta->ModeOrgGlobal == 1) {
            // Mode contigu
            for (int i = 0; i < meta->TailleEnBlocs; i++) {
                int blocIndex = meta->FirstBlocIndex + i;
                if (blocIndex < table->nbBlocs) {
                    table->blocs[blocIndex] = 1; // Marquer le bloc comme libre
                }
            }
            printf("Blocs libérés en mode contigu pour fichier ordonné.\n");
        } else if (meta->ModeOrgGlobal == 2) {
            // Mode chaîné
            int indexBloc = meta->FirstBlocIndex;
            while (indexBloc != -1) {
                ListBloc bloc;
                fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
                fread(&bloc, sizeof(ListBloc), 1, *MS);

                // Libérer le bloc actuel
                table->blocs[indexBloc] = 1; // Marquer comme libre

                // Passer au bloc suivant
                indexBloc = (bloc.nextB != NULL) ? bloc.nextB->numBloc : -1;
            }
            printf("Blocs libérés en mode chaîné pour fichier ordonné.\n");
        }
    } else if (meta->ModeOrgIntern == 2) { // Fichier non ordonné
        if (meta->ModeOrgGlobal == 1) {
            // Mode contigu
            for (int i = 0; i < meta->TailleEnBlocs; i++) {
                int blocIndex = meta->FirstBlocIndex + i;
                if (blocIndex < table->nbBlocs) {
                    table->blocs[blocIndex] = 1; // Marquer le bloc comme libre
                }
            }
            printf("Blocs libérés en mode contigu pour fichier non ordonné.\n");
        } else if (meta->ModeOrgGlobal == 2) {
            // Mode chaîné
            int indexBloc = meta->FirstBlocIndex;
            while (indexBloc != -1) {
                ListBloc bloc;
                fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
                fread(&bloc, sizeof(ListBloc), 1, *MS);

                // Libérer le bloc actuel
                table->blocs[indexBloc] = 1; // Marquer comme libre

                // Passer au bloc suivant
                indexBloc = (bloc.nextB != NULL) ? bloc.nextB->numBloc : -1;
            }
            printf("Blocs libérés en mode chaîné pour fichier non ordonné.\n");
        }
    } else {
        printf("Erreur : Mode d'organisation interne inconnu.\n");
        return;
    }

    // Réinitialiser les métadonnées
    meta->FirstBlocIndex = -1;
    meta->TailleEnBlocs = 0;
    meta->TailleEnEng = 0;
    strcpy(meta->Nom, "");

    printf("Fichier supprimé physiquement avec succès.\n");
}
void SupprimerFichierLogiquement(FILE **MS, TableDAllocation *table,MetaDonnees *meta) {
    // Vérifier si le fichier existe
    if (meta->FirstBlocIndex == -1) {
        printf("Erreur : Le fichier n'existe pas ou a déjà été supprimé.\n");
        return;
    }

    printf("Suppression logique du fichier : %s\n", meta->Nom);

    // Cas fichier ordonné
    if (meta->ModeOrgIntern == 1) {
        if (meta->ModeOrgGlobal == 1) {
            // Mode contigu ordonné
            for (int i = 0; i < meta->TailleEnBlocs; i++) {
                int blocIndex = meta->FirstBlocIndex + i;
                ContBloc bloc;

                // Lire le bloc
                fseek(*MS, blocIndex * sizeof(ContBloc), SEEK_SET);
                fread(&bloc, sizeof(ContBloc), 1, *MS);

                // Marquer tous les enregistrements comme logiquement supprimés
                for (int j = 0; j < bloc.nbEnregistrement; j++) {
                    bloc.Bloc[j].logicDel = 1;
                }

                // Sauvegarder le bloc modifié
                fseek(*MS, blocIndex * sizeof(ContBloc), SEEK_SET);
                fwrite(&bloc, sizeof(ContBloc), 1, *MS);
            }
            printf("Enregistrements marqués comme supprimés en mode contigu ordonné.\n");
        }
        else if (meta->ModeOrgGlobal == 2) {
            // Mode chaîné ordonné
            int indexBloc = meta->FirstBlocIndex;
            while (indexBloc != -1) {
                ListBloc bloc;
                fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
                fread(&bloc, sizeof(ListBloc), 1, *MS);

                // Marquer tous les enregistrements du bloc comme supprimés
                for (int j = 0; j < bloc.nbEnregistrement; j++) {
                    bloc.Bloc[j].logicDel = 1;
                }

                // Sauvegarder le bloc modifié
                fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
                fwrite(&bloc, sizeof(ListBloc), 1, *MS);

                // Passer au bloc suivant dans la chaîne
                indexBloc = (bloc.nextB != NULL) ? bloc.nextB->numBloc : -1;
            }
            printf("Enregistrements marqués comme supprimés en mode chaîné ordonné.\n");
        }
    }
    // Cas fichier non ordonné
    else if (meta->ModeOrgIntern == 2) {
        if (meta->ModeOrgGlobal == 1) {
            // Mode contigu non ordonné
            for (int i = 0; i < meta->TailleEnBlocs; i++) {
                int blocIndex = meta->FirstBlocIndex + i;
                ContBloc bloc;

                // Lire le bloc
                fseek(*MS, blocIndex * sizeof(ContBloc), SEEK_SET);
                fread(&bloc, sizeof(ContBloc), 1, *MS);

                // Marquer simplement tous les enregistrements comme supprimés
                for (int j = 0; j < bloc.nbEnregistrement; j++) {
                    bloc.Bloc[j].logicDel = 1;
                }

                // Sauvegarder le bloc modifié
                fseek(*MS, blocIndex * sizeof(ContBloc), SEEK_SET);
                fwrite(&bloc, sizeof(ContBloc), 1, *MS);
            }
            printf("Enregistrements marqués comme supprimés en mode contigu non ordonné.\n");
        }
        else if (meta->ModeOrgGlobal == 2) {
            // Mode chaîné non ordonné
            int indexBloc = meta->FirstBlocIndex;
            while (indexBloc != -1) {
                ListBloc bloc;
                fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
                fread(&bloc, sizeof(ListBloc), 1, *MS);

                // Marquer tous les enregistrements du bloc comme supprimés
                for (int j = 0; j < bloc.nbEnregistrement; j++) {
                    bloc.Bloc[j].logicDel = 1;
                }

                // Sauvegarder le bloc modifié
                fseek(*MS, indexBloc * sizeof(ListBloc), SEEK_SET);
                fwrite(&bloc, sizeof(ListBloc), 1, *MS);

                // Passer au bloc suivant dans la chaîne
                indexBloc = (bloc.nextB != NULL) ? bloc.nextB->numBloc : -1;
            }
            printf("Enregistrements marqués comme supprimés en mode chaîné non ordonné.\n");
        }
    }

    printf("Fichier marqué comme logiquement supprimé avec succès.\n");
}