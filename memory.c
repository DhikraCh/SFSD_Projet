#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    int numeroBloc;               
    int estLibre;                 
    char nomFichier[50];          
    int nombreEnregistrements;    
} Bloc;
typedef struct {
    Bloc *blocs;                  
    int nombreBlocs;              
} MemoireSecondaire;



void initialiserMemoireSecondaire(MemoireSecondaire *ms, int nombreBlocs) {
    ms->nombreBlocs = nombreBlocs;
    ms->blocs = (Bloc *)malloc(nombreBlocs * sizeof(Bloc));
    
    for (int i = 0; i < nombreBlocs; i++) {
        ms->blocs[i].numeroBloc = i;
        ms->blocs[i].estLibre = 1; // Libre
        strcpy(ms->blocs[i].nomFichier, "Libre"); // Nom par défaut
        ms->blocs[i].nombreEnregistrements = 0;
    }
    
    printf("Mémoire secondaire initialisée avec %d blocs.\n", nombreBlocs);
}


void compacterContMS(FILE **MS){

    TableDAllocation table;
    fseek(*MS, 0, SEEK_SET);
    fread(&table, sizeof(TableDAllocation), 1, *MS);

    // Compactage : déplacer les blocs occupés vers les premières positions
    int writeIndex = 0;  // Indique où écrire les blocs occupés
    ContBloc bloc;

    for (int i = 0; i < table.nbBlocs; i++) {
        fseek(*MS, sizeof(TableDAllocation) + i * sizeof(ContBloc), SEEK_SET);
        fread(&bloc, sizeof(ContBloc), 1, *MS);

        if (table.blocs[i] == 0) { // Bloc occupé
            // Mettre à jour le numéro du bloc avant de le déplacer
            bloc.numBloc = writeIndex;

            // Si le bloc n'est pas déjà à sa place finale, le déplacer
            if (i != writeIndex) {
                fseek(*MS, sizeof(TableDAllocation) + writeIndex * sizeof(ContBloc), SEEK_SET);
                fwrite(&bloc, sizeof(ContBloc), 1, *MS);

                // Marquer l'ancien bloc comme libre
                table.blocs[i] = 1;
            }

            // Marquer la nouvelle position comme occupée
            table.blocs[writeIndex] = 0;
            writeIndex++;
        }
    }

    // Mettre à jour les blocs restants comme libres
    for (int i = writeIndex; i < table.nbBlocs; i++) {
        fseek(*MS, sizeof(TableDAllocation) + i * sizeof(ContBloc), SEEK_SET);

        // Initialiser manuellement le bloc
        bloc.numBloc = i;
        bloc.nbEnregistrement = 0;
        for (int j = 0; j < FB; j++) {
            bloc.Bloc[j].ID = -1;
            bloc.Bloc[j].logicDel = 1;
        }

        fwrite(&bloc, sizeof(ContBloc), 1, *MS);
        table.blocs[i] = 1;
    }

    // Mise à jour de la table d'allocation
    table.libres = table.nbBlocs - writeIndex;
    fseek(*MS, 0, SEEK_SET);
    fwrite(&table, sizeof(TableDAllocation), 1, *MS);

    printf("Compactage termine. %d blocs libres disponibles.\n", table.libres);

}


void afficherEtatMemoire(MemoireSecondaire ms) {
    printf("État de la mémoire secondaire :\n");
    for (int i = 0; i < ms.nombreBlocs; i++) {
        if (ms.blocs[i].estLibre) {
            printf("[Bloc %d: Libre] ", ms.blocs[i].numeroBloc);
        } else {
            printf("[Bloc %d: %s (%d enregistrements)] ", 
                   ms.blocs[i].numeroBloc, 
                   ms.blocs[i].nomFichier, 
                   ms.blocs[i].nombreEnregistrements);
        }
    }
    printf("\n");
}


void viderMemoireSecondaire(MemoireSecondaire *ms) {
    for (int i = 0; i < ms->nombreBlocs; i++) {
        ms->blocs[i].estLibre = 1; // Libre
        strcpy(ms->blocs[i].nomFichier, "Libre"); // Réinitialiser le nom
        ms->blocs[i].nombreEnregistrements = 0; // Réinitialiser les enregistrements
    }
    printf("Mémoire secondaire vidée avec succès.\n");
}


