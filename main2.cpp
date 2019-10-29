//
// Created by Ben Taylor on 10/22/19.
//
#include <iostream>
#include <mpi.h>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>
#include <chrono>
#include <ctime>

#define MCW MPI_COMM_WORLD
const int WIDTH = 32;
const int HEIGHT = 32;

void createOrganism(int (&gameBoard)[HEIGHT][WIDTH]);

using namespace std;

int main(int argc, char **argv){

    /**
     * HOW DO I DO PSEUDO RANDOM NUMBER GENERATOR FOR MPI AND MULTIPLE PROCESSORS?????
     *    srand at the beginning of each processor and seed it with processor's rank
     * Create new organism is that only for those spaces that currently don't exist???
     *
     * Just print it to the screen with 0s and 1s??
     * create a portable pix map, ppm files, that show a pixel per organism on the board
     */

    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MCW, &rank);
    MPI_Comm_size(MCW, &size);

    char gameBoard[HEIGHT][WIDTH];
    char nextGeneration[HEIGHT][WIDTH];
    char segment[HEIGHT/size][WIDTH];
    char above[WIDTH], below[WIDTH];
    auto start = std::chrono::system_clock::now();

    if (!rank) {
        srand(rank);
        for (int i = 0; i < HEIGHT; i++) {
            for (int j = 0; j < WIDTH; j++) {
                gameBoard[i][j] = (rand() % 5 == 0 ? '0' : '1');
            }
        }

//        cout << "ORIGINAL BOARD: " << endl;
        ofstream generation_image("generation.ppm");
        if (generation_image.is_open()) {
            generation_image << "P3\n" << WIDTH << " " << HEIGHT << " 255\n";
            for (int i = 0; i < HEIGHT; i++) {
                for (int j = 0; j < WIDTH; j++) {
//                    cout << " " << gameBoard[i][j] << " ";
                    generation_image << (gameBoard[i][j] - '0') * 255 << " " << (gameBoard[i][j] - '0') * 255 << " "
                                     << (gameBoard[i][j] - '0') * 255 << " ";
                }
//                cout << endl;
                generation_image << endl;
            }
            generation_image.close();
        }
        else {
            cout << "File didn't open" << endl;
            MPI_Finalize();
        }


        for (int i = 1; i < size; i++) {
            for (int j = 0; j < HEIGHT/size; j++) {
                for (int k = 0; k < WIDTH; k++) {
                    segment[j][k] = gameBoard[(HEIGHT/size)*i+j][k];
                }
            }
            MPI_Send(&segment, (HEIGHT/size)*WIDTH, MPI_CHAR, i, 0, MCW);
        }
    }

    else {
        MPI_Recv(&segment, (HEIGHT/size)*WIDTH, MPI_CHAR, 0, 0, MCW, MPI_STATUS_IGNORE);
    }

    for (int z = 0; z < 5; z++) {
        start = std::chrono::system_clock::now();
        if (rank == 0) {
            MPI_Send(gameBoard[HEIGHT / size - 1], WIDTH, MPI_CHAR, 1, 0, MCW);
            MPI_Recv(&below, WIDTH, MPI_CHAR, 1, 0, MCW, MPI_STATUS_IGNORE);
        } else if (rank == size - 1) {
            MPI_Send(segment[0], WIDTH, MPI_CHAR, rank - 1, 0, MCW);
            MPI_Recv(&above, WIDTH, MPI_CHAR, rank - 1, 0, MCW, MPI_STATUS_IGNORE);
        } else {
            MPI_Send(segment[0], WIDTH, MPI_CHAR, rank - 1, 0, MCW);
            MPI_Send(segment[HEIGHT / size - 1], WIDTH, MPI_CHAR, rank + 1, 0, MCW);
            MPI_Recv(&above, WIDTH, MPI_CHAR, rank - 1, 0, MCW, MPI_STATUS_IGNORE);
            MPI_Recv(&below, WIDTH, MPI_CHAR, rank + 1, 0, MCW, MPI_STATUS_IGNORE);
        }

        if (!rank) {
            for (int x = 0; x < (HEIGHT / size); x++) {
                for (int y = 0; y < WIDTH; y++) {
                    segment[x][y] = gameBoard[x][y];
                }
            }
        }
//        cout << rank << " made it past receiving and sending above below" << endl;
        for (int i = 0; i < (HEIGHT / size); i++) {
            for (int j = 0; j < WIDTH; j++) {
                // check the neighbors
                int neighbors = 0;
                if (i == 0) {
                    for (int m = -1; m <= 1; m++) {
                        if (j + m < WIDTH && j + m > -1 && above[j + m] == '0') neighbors++;
                    }
                    for (int k = 0; k <= 1; k++) {
                        for (int m = -1; m <= 1; m++) {
                            if (j + m < WIDTH && j + m > -1 && segment[i + k][j + m] == '0') neighbors++;
                        }
                    }
                } else if (i == (HEIGHT / size) - 1) {
                    for (int k = -1; k <= 0; k++) {
                        for (int m = -1; m <= 1; m++) {
                            if (j + m < WIDTH && j + m > -1 && segment[i + k][j + m] == '0') neighbors++;
                        }
                    }
                    for (int m = -1; m <= 1; m++) {
                        if (j + m < WIDTH && j + m > -1 && below[j + m] == '0') neighbors++;
                    }
                } else {
                    for (int k = -1; k <= 1; k++) {
                        for (int m = -1; m <= 1; m++) {
                            if (i + k < HEIGHT && i + k > -1 && j + m < WIDTH && j + m > -1 &&
                                gameBoard[i + k][j + m] == '0') {
                                neighbors++;
                            }
                        }
                    }
                }
                if (rank != 0 && segment[i][j] == '0') neighbors--; // subtract itself as a neighbor if it's an organism
                else if (rank == 0 && segment[i][j] == '0') neighbors--; // subtract itself as a neighbor if it's an organism

                if (neighbors < 2 or neighbors > 3) {
                    nextGeneration[i][j] = '1';
                } else if (neighbors == 3) {
                    nextGeneration[i][j] = '0';
                } else {
                    nextGeneration[i][j] = segment[i][j];
                }
            }
        }
//        cout << rank << " made it past calculation" << endl;
//    MPI_Barrier(MCW);

        if (rank) {
            for (int j = 0; j < HEIGHT / size; j++) {
                for (int k = 0; k < WIDTH; k++) {
                    segment[j][k] = nextGeneration[j][k];
                }
            }
            cout << segment[1] << endl;
            MPI_Send(&segment, (HEIGHT / size) * WIDTH, MPI_CHAR, 0, 1, MCW);
        } else {
            for (int i = 1; i < size; i++) {
                MPI_Recv(&segment, (HEIGHT / size) * WIDTH, MPI_CHAR, i, 1, MCW, MPI_STATUS_IGNORE);
                for (int j = 0; j < HEIGHT / size; j++) {
                    for (int k = 0; k < WIDTH; k++) {
                        nextGeneration[j + (HEIGHT / size) * i][k] = segment[j][k];
                    }
                }
            }

            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end - start;
            cout << "Generation" << z << " took " << elapsed_seconds.count() << " seconds" << endl;
            ofstream generation_image("generation" + to_string(z) + ".ppm");
            if (generation_image.is_open()) {
                generation_image << "P3\n" << WIDTH << " " << HEIGHT << " 255\n";
                for (int i = 0; i < HEIGHT; i++) {
                    for (int j = 0; j < WIDTH; j++) {
//                        cout << " " << nextGeneration[i][j] << " ";
                        gameBoard[i][j] = nextGeneration[i][j];
                        generation_image << (gameBoard[i][j] - '0') * 255 << " " << (gameBoard[i][j] - '0') * 255 << " "
                                         << (gameBoard[i][j] - '0') * 255 << " ";
                    }
                    generation_image << endl;
//                    cout << endl;
                }
                generation_image.close();
            }else {cout << "File didn't open" << endl; MPI_Finalize();}
        }
    }


    MPI_Finalize();

    return 0;
}

void createOrganism(int (&gameBoard)[HEIGHT][WIDTH]) {

}
