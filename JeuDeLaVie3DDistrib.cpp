#include <iostream>
#include "mpi.h"
#include <unistd.h>

std::string toString(bool* donnees, int nbTranches, int nbLignes, int nbColonnes) {
    std::string res = "";
    for(int i=0; i<nbTranches; i++) {
        res += "tranche ";
        res += std::to_string(i);
        res += "\n";
        for(int j=0; j<nbLignes; j++) {
            for(int k=0; k<nbColonnes; k++) {
                if(donnees[i*nbLignes*nbColonnes+j*nbColonnes+k])
                    res += "1";
                else
                    res += "_";
            }
            res += "\n";
        }
        res += "\n";
    }
    return res;
}

void affiche(bool* donnees, int nbTranches, int nbLignes, int nbColonnes, std::string texte) {
    std::cout << texte << "\n" << toString(donnees, nbTranches, nbLignes, nbColonnes) << std::endl;
}

bool* jeuToJeuAvecBordures(bool* jeu, int nbTranches, int nbLignes, int nbColonnes) {
    bool* res = (bool*) calloc((nbTranches+2) * (nbLignes+2) * (nbColonnes+2), sizeof(bool));

    for(int i=0; i<nbTranches; i++) {
        for(int j=0; j<nbLignes; j++) {
            for(int k=0; k<nbColonnes; k++) {
                res[(i+1)*(nbLignes+2)*(nbColonnes+2) + (j+1)*(nbColonnes+2) + k+1] = jeu[i*nbLignes*nbColonnes + j*nbColonnes + k];
            }
        }
    }
    return res;
} 

bool* init(int nbTranches, int nbLignes, int nbColonnes) {
    //allocation mémoire donnees
    bool* donnees = (bool*) calloc((nbTranches+2)*(nbLignes+2)*(nbColonnes+2), sizeof(bool));

    //initialisation donnees
    srand(time(NULL));
    for(int i=0; i<nbTranches; i++) {
        for(int j=0; j<nbLignes; j++) {
            for(int k=0; k<nbColonnes; k++) {
                int random = rand() % 2;
                donnees[(i+1)*(nbLignes+2)*(nbColonnes+2)+(j+1)*(nbColonnes+2)+k+1] = random;
            } 
        }
    }
    return donnees;
}

int nbVivantsAdjacents(bool* donnees, int position, int nbLignes, int nbColonnes) {
    int res = 0;
    int positionVoisinDirectTranchePrecedante = position - nbLignes*nbColonnes;
    int positionVoisinDirectTrancheSuivante = position + nbLignes*nbColonnes;
    res += donnees[positionVoisinDirectTranchePrecedante-nbColonnes-1] + donnees[positionVoisinDirectTranchePrecedante-nbColonnes] + donnees[positionVoisinDirectTranchePrecedante-nbColonnes+1]
         + donnees[positionVoisinDirectTranchePrecedante-1] + donnees[positionVoisinDirectTranchePrecedante] + donnees[positionVoisinDirectTranchePrecedante+1]
         + donnees[positionVoisinDirectTranchePrecedante+nbColonnes-1] + donnees[positionVoisinDirectTranchePrecedante+nbColonnes] + donnees[positionVoisinDirectTranchePrecedante+nbColonnes+1]

         + donnees[position-nbColonnes-1] + donnees[position-nbColonnes] + donnees[position-nbColonnes+1]
         + donnees[position-1] + donnees[position+1]
         + donnees[position+nbColonnes-1] + donnees[position+nbColonnes] + donnees[position+nbColonnes+1]

         + donnees[positionVoisinDirectTrancheSuivante-nbColonnes-1] + donnees[positionVoisinDirectTrancheSuivante-nbColonnes] + donnees[positionVoisinDirectTrancheSuivante-nbColonnes+1]
         + donnees[positionVoisinDirectTrancheSuivante-1] + donnees[positionVoisinDirectTrancheSuivante] + donnees[positionVoisinDirectTrancheSuivante+1]
         + donnees[positionVoisinDirectTrancheSuivante+nbColonnes-1] + donnees[positionVoisinDirectTrancheSuivante+nbColonnes] + donnees[positionVoisinDirectTrancheSuivante+nbColonnes+1];
    return res;
}

bool* iteration(bool* donnees, int nbTranches, int nbLignes, int nbColonnes) {

    bool* res = (bool*) malloc(nbTranches * nbLignes * nbColonnes * sizeof(bool));

    for(int i=1; i<nbTranches+1; i++) {
        for(int j=1; j<nbLignes+1; j++) {
            for(int k=1; k<nbColonnes+1; k++) {
                int positionDonnees = i*(nbLignes+2)*(nbColonnes+2) + j*(nbColonnes+2) + k;
                int positionRes = (i-1)*nbLignes*nbColonnes + (j-1)*nbColonnes + k-1;
                int nbVivants = nbVivantsAdjacents(donnees, positionDonnees, nbLignes+2, nbColonnes+2);

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
    int nbLignes=5; //nombre de lignes du jeu
    int nbColonnes=3; //nombre de colonnes du jeu
    int nbTranches=6; //nombre de tranches du jeu
    int nbIterations=2; //nombre total d'itérations à réaliser
    int nbIterationActuelle=0; //nombre actuel d'itérations
    int tailleTranche = (nbLignes+2)*(nbColonnes+2);

    if(rank == 0) {
        donnees = init(nbTranches, nbLignes, nbColonnes); //génération des données du jeu global
        affiche(donnees, nbTranches+2, nbLignes+2, nbColonnes+2, "Données globales départ"); //affichage du jeu global
    }

    //création et initialisation du tableau contenant les tailles des tableaux de données locales
    int* taillesDonneesLocales = (int*) malloc(size*sizeof(int));
    for(int i=0; i<size; i++) {
        taillesDonneesLocales[i] = ((nbTranches/size)+2)*tailleTranche;
    }

    //si le nombre de tranches n'est pas divisible par size, alors on attribue les tranches restantes unes à unes à des noeuds
    int tranchesSupplementaires = nbTranches % size;
    for(int i = 0; i < tranchesSupplementaires; i++) {
        taillesDonneesLocales[i] += tailleTranche;
    }

    //position du début des données locales dans les données globales
    int* debutDonneesLocales = (int*) malloc(size*sizeof(int));
    debutDonneesLocales[0] = 0;
    for(int i=1; i<size; i++) {
        debutDonneesLocales[i] = debutDonneesLocales[i-1] + taillesDonneesLocales[i-1] - 2*tailleTranche;
    }

    donneesLocales = (bool*) malloc(taillesDonneesLocales[rank]*sizeof(bool));

    //nombre de tranches du jeu local
    int nbTranchesLocal = (taillesDonneesLocales[rank]/tailleTranche)-2;

    if(rank == 0) {
        donneesGlobalesCalculees = (bool*) malloc(nbTranches * nbLignes * nbColonnes * sizeof(bool));
    }

    //déclaration et initialisation du tableau contenant les tailles des tableaux de données calculées (après itération) locales
    int* taillesDonneesCalculeesLocales = (int*) malloc(size*sizeof(int)); 
    for(int i=0; i<size; i++) {
        taillesDonneesCalculeesLocales[i] = ((taillesDonneesLocales[i]/tailleTranche)-2)*nbLignes*nbColonnes;
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

        affiche(donneesLocales, nbTranchesLocal+2, nbLignes+2, nbColonnes+2, "Données locales noeud " + std::to_string(rank) + " iteration " + std::to_string(i)); //affichage du jeu reçu localement

        donneesLocalesCalculees = iteration(donneesLocales, nbTranchesLocal, nbLignes, nbColonnes); //calcul local de l'étape suivante du jeu

        affiche(donneesLocalesCalculees, nbTranchesLocal, nbLignes, nbColonnes, "Données calculées locales noeud " + std::to_string(rank) + " iteration " + std::to_string(i)); //affichage du jeu calculé localement

        //regroupement de tous les jeux locaux calculés afin de former le jeu global après itération
        MPI_Gatherv(donneesLocalesCalculees, nbTranchesLocal * nbLignes * nbColonnes, MPI_C_BOOL,
                    donneesGlobalesCalculees, taillesDonneesCalculeesLocales, debutDonneesCalculeesLocales, MPI_C_BOOL, 0, MPI_COMM_WORLD);

        if(rank == 0) {
            //affichage du jeu après itération
            affiche(donneesGlobalesCalculees, nbTranches, nbLignes, nbColonnes, "Données calculées globales iteration " + std::to_string(i));

            affiche(donnees, nbTranches+2, nbLignes+2, nbColonnes+2, "Données avant itération " + std::to_string(i));
            free(donnees);
            donnees = jeuToJeuAvecBordures(donneesGlobalesCalculees, nbTranches, nbLignes, nbColonnes);
            affiche(donnees, nbTranches+2, nbLignes+2, nbColonnes+2, "Données après itération " + std::to_string(i));
        }
    } 

    MPI_Barrier(MPI_COMM_WORLD);

    free(donneesLocales);
    free(taillesDonneesLocales);
    free(debutDonneesLocales);
    free(donneesLocalesCalculees);
    if (rank == 0) {
        free(donneesGlobalesCalculees);
        free(donnees);
    }

    MPI_Finalize();
    return 0;
}