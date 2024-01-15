#include <iostream>
#include "mpi.h"
#include <unistd.h>

std::string toString(bool* donnees, int l, int c) {
    std::string res = "";
    for(int i=0; i<l; i++) {
        for(int j=0; j<c; j++) {
            if(donnees[i*c+j])
                res += "1";
            else
                res += "_";
        }
        res += "\n";
    }
    return res;
}

void affiche(bool* donnees, int l, int c) {
    std::cout << toString(donnees, l, c) << std::endl;
}

bool* jeuToJeuAvecBordures(bool* jeu, int nbLignes, int nbColonnes) {
    bool* res = (bool*) malloc((nbLignes+2) * (nbColonnes+2) * sizeof(bool));
    for(int i=0; i<nbColonnes+2; i++) {
        res[i] = false;
        res[(nbLignes+1)*(nbColonnes+2)+i] = false;
    }
    for(int i=1; i<nbLignes+1; i++) {
        res[i*(nbColonnes+2)] = false;
        res[(i+1)*(nbColonnes+2)-1] = false;
    }
    for(int i=0; i<nbLignes; i++) {
        for (int j=0; j<nbColonnes; j++) {
            res[(i+1)*(nbColonnes+2)+(j+1)] = jeu[i*nbColonnes+j];
        }
    }
    return res;
} 

bool* init(int l, int c) {
    //allocation mémoire donnees
    bool* donnees = (bool*) calloc((l+2)*(c+2), sizeof(bool));

    //initialisation donnees
    srand(time(NULL));
    for(int i=0; i<l; i++) {
        for(int j=0; j<c; j++) {
            int random = rand() % 2;
            donnees[(i+1)*(c+2)+(j+1)] = random;
        }
    }
    return donnees;
}

int nbVivantsAdjacents(bool* donnees, int position, int nbColonnes) {
    int res = 0;
    res += donnees[position-nbColonnes-1] + donnees[position-nbColonnes] + donnees[position-nbColonnes+1]
            + donnees[position-1] + donnees[position+1]
            + donnees[position+nbColonnes-1] + donnees[position+nbColonnes] + donnees[position+nbColonnes+1];
    return res;
}

bool* iteration(bool* donnees, int nbLignes, int nbColonnes) {

    bool* res = (bool*) malloc(nbLignes * nbColonnes * sizeof(bool));

    for(int i=1; i<nbLignes+1; i++) {
        for(int j=1; j<nbColonnes+1; j++) {
            int positionDonnees = i*(nbColonnes+2)+j;
            int positionRes = (i-1)*(nbColonnes)+(j-1);
            int nbVivants = nbVivantsAdjacents(donnees, positionDonnees, nbColonnes+2);

            if(nbVivants == 3)
                res[positionRes] = true;
            else {
                if(donnees[positionDonnees] && nbVivants == 2)
                    res[positionRes] = true;
                else
                    res[positionRes] = false;
            }
        }
    }
    return res;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    bool* donnees; //tableau contenant le jeu global
    bool* donneesLocales; //tableau contenant le jeu local
    bool* donneesLocalesCalculees; //tableau contenant le jeu local après une itération
    bool* donneesGlobalesCalculees; //tableau contenant le jeu global après une itération
    int nbLignes=8; //nombre de lignes du jeu
    int nbColonnes=5; //nombre de colonnes du jeu
    int nbIterations=4; //nombre total d'itérations à réaliser
    int nbIterationActuelle=0; //nombre actuel d'itérations

    if(rank == 0) {
        donnees = init(nbLignes, nbColonnes); //génération des données du jeu global
        std::cout << "Données globales : " << std::endl;
        affiche(donnees, nbLignes+2, nbColonnes+2); //affichage du jeu global
    }

    //création et initialisation du tableau contenant les tailles des tableaux de données locales
    int* taillesDonneesLocales = (int*) malloc(size*sizeof(int));
    for(int i=0; i<size; i++) {
        taillesDonneesLocales[i] = ((nbLignes/size)+2)*(nbColonnes+2);
    }

    //si le nombre de lignes n'est pas divisible par size, alors on attribue les lignes restantes unes à unes à des noeuds
    int lignesSupplementaires = nbLignes % size;
    for(int i = 0; i < lignesSupplementaires; i++) {
        taillesDonneesLocales[i] += nbColonnes+2;
    }

    //position du début des données locales dans les données globales
    int* debutDonneesLocales = (int*) malloc(size*sizeof(int));
    debutDonneesLocales[0] = 0;
    for(int i=1; i<size; i++) {
        debutDonneesLocales[i] = debutDonneesLocales[i-1] + taillesDonneesLocales[i-1] - 2*(nbColonnes+2);
    }

    donneesLocales = (bool*) malloc(taillesDonneesLocales[rank]*sizeof(bool));

    //nombre de lignes du jeu local
    int nbLignesLocal = (taillesDonneesLocales[rank]/(nbColonnes+2))-2;

    if(rank == 0) {
        donneesGlobalesCalculees = (bool*) malloc(nbLignes * nbColonnes * sizeof(bool));
    }

    //déclaration et initialisation du tableau contenant les tailles des tableaux de données calculées (après itération) locales
    int* taillesDonneesCalculeesLocales = (int*) malloc(size*sizeof(int)); 
    for(int i=0; i<size; i++) {
        taillesDonneesCalculeesLocales[i] = ((taillesDonneesLocales[i]/(nbColonnes+2))-2)*nbColonnes;
    }

    //déclaration et initialisation du tableau contenant les positions (dans le tableau global) où débutent les données calculées
    int* debutDonneesCalculeesLocales = (int*) malloc(size*sizeof(int));
    debutDonneesCalculeesLocales[0] = 0;
    for(int i=1; i<size; i++) {
        debutDonneesCalculeesLocales[i] = debutDonneesCalculeesLocales[i-1] + taillesDonneesCalculeesLocales[i-1];
    }

    for(int i=0; i<nbIterations; i++){
        //découpe du jeu global et transfert des parties aux différents noeuds
        MPI_Scatterv(donnees, taillesDonneesLocales, debutDonneesLocales, MPI_C_BOOL,
                    donneesLocales, taillesDonneesLocales[rank], MPI_C_BOOL, 0, MPI_COMM_WORLD);

        std::cout << "Données locales noeud " << rank << " : " << std::endl;
        affiche(donneesLocales, nbLignesLocal+2, nbColonnes+2); //affichage du jeu reçu localement

        donneesLocalesCalculees = iteration(donneesLocales, nbLignesLocal, nbColonnes); //calcul local de l'étape suivante du jeu

        std::cout << "Données calculées locales noeud " << rank << " : " << std::endl;
        affiche(donneesLocalesCalculees, nbLignesLocal, nbColonnes); //affichage du jeu calculé localement

        //regroupement de tous les jeux locaux calculés afin de former le jeu global après itération
        MPI_Gatherv(donneesLocalesCalculees, nbLignesLocal * nbColonnes, MPI_C_BOOL,
                    donneesGlobalesCalculees, taillesDonneesCalculeesLocales, debutDonneesCalculeesLocales, MPI_C_BOOL, 0, MPI_COMM_WORLD);

        if(rank == 0) {
            //affichage du jeu après itération
            std::cout << "Données calculées globales : " << std::endl;
            affiche(donneesGlobalesCalculees, nbLignes, nbColonnes);

            std::cout << "Données avant itération " << i << " : " << std::endl;
            affiche(donnees, nbLignes+2, nbColonnes+2);
            free(donnees);
            donnees = jeuToJeuAvecBordures(donneesGlobalesCalculees, nbLignes, nbColonnes);
            std::cout << "Données après itération " << i << " : " << std::endl;
            affiche(donnees, nbLignes+2, nbColonnes+2);
        }
    } 

    MPI_Barrier(MPI_COMM_WORLD);

    free(donneesLocales);
    free(taillesDonneesLocales);
    free(debutDonneesLocales);
    free(donneesLocalesCalculees);
    if(rank == 0) {
        free(donneesGlobalesCalculees);
        free(donnees);
    }

    MPI_Finalize();
    return 0;
}