#include <iostream>
#include <cstdlib>

class JeuDeLaVie2D {
private:
    int m;
    int n;
    int nbIterations;
    bool** donnees;
    bool** tabCalcul;

public:
    JeuDeLaVie2D(int m, int n, int nbIterations) {
        init(m, n, nbIterations);
        for (int i=0; i<nbIterations; i++) {
            iteration();
            affiche();
        }
    }

    void init(int m, int n, int nbIterations) {
        this->m = m;
        this->n = n;
        this->nbIterations = nbIterations;
        donnees = new bool*[m];
        tabCalcul = new bool*[m+2];

        for (int i=0; i<m; i++) {
            donnees[i] = new bool[n];
        }

        for (int i=0; i<m+2; i++) {
            tabCalcul[i] = new bool[n+2];
        }

        genereDonnees();
        affiche();
    }

    void genereDonnees() {
        srand(time(NULL));

        for (int i=0; i<m; i++) {
            for (int j=0; j<n; j++) {
                int random = rand() % 2;
                donnees[i][j] = random;
            }
        }

        for (int i=0; i<m+2; i++) {
            tabCalcul[i][0] = false;
            tabCalcul[i][n+1] = false;
            for (int j=1; j<n+1; j++) {
                tabCalcul[i][j] = false;
            }
        }
    }

    void iteration() {
        for (int i=0; i<m; i++) {
            for (int j=0; j<n; j++) {
                tabCalcul[i+1][j+1] = donnees[i][j];
            }
        }

        for (int i=1; i<m+1; i++) {
            for (int j=1; j<n+1; j++) {
                int nbVivants = nbVivantsAdjacents(i, j);

                if (nbVivants == 3)
                    donnees[i-1][j-1] = true;
                else {
                    if (tabCalcul[i][j] && nbVivants == 2)
                        donnees[i-1][j-1] = true;
                    else
                        donnees[i-1][j-1] = false;
                }
            }
        }
    }

    int nbVivantsAdjacents(int ligne, int colonne) {
        int res = 0;
        for (int i=ligne-1; i<ligne+2; i++) {
            for (int j=colonne-1; j<colonne+2; j++) {
                if (tabCalcul[i][j] && (i!=ligne || j!=colonne))
                    res++;
            }
        }
        return res;
    }

    std::string toString() {
        std::string res = "";
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                if (donnees[i][j])
                    res += "1";
                else
                    res += "_";
            }
            res += "\n";
        }
        return res;
    }

    void affiche() {
        std::cout << toString() << std::endl;
    }

    ~JeuDeLaVie2D() {
        for (int i=0; i<m; i++) {
            delete[] donnees[i];
        }
        delete[] donnees;

        for (int i=0; i<m+2; i++) {
            delete[] tabCalcul[i];
        }
        delete[] tabCalcul;
    }
};

int main() {
    JeuDeLaVie2D app(4, 3, 4);
    return 0;
}
