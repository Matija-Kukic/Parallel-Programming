#include <cstdlib>
#include <iostream>
#include <mpi.h>
#include <ostream>
#include <random>
#include <string>
#include <unistd.h>
#include <vector>

class bla {
  public:
    bla(const bla &) = default;
    bla(bla &&) = default;
    bla &operator=(const bla &) = default;
    bla &operator=(bla &&) = default;
};

int main(int argc, char **argv) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 5);
    MPI_Init(&argc, &argv);

    // int world_size;
    // MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // int world_rank;
    // MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // printf("Hello world! Ja sam proces %d od ukupno %d procesa.\n",
    // world_rank,
    //        world_size);

    int np;
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    std::string tpad = "";
    for (int i = 0; i < pid; i++)
        tpad += "\t";

    int idxForkLeft, idxForkRight;
    int haveLeft, haveRight;
    int cleanLeft, cleanRight;
    int usrRight, usrLeft;

    idxForkLeft = pid;
    idxForkRight = (pid + 1) % np;

    usrRight = (pid + 1) % np;
    usrLeft = (pid + np - 1) % np;

    haveLeft = pid < usrLeft ? 1 : 0;
    haveRight = pid < usrRight ? 1 : 0;

    cleanLeft = 0;
    cleanRight = 0;

    // std::cout << "Ja sam proces " << pid << " i trebam vilice: " <<
    // idxForkLeft
    //           << " i " << idxForkRight << std::endl;

    while (1) {
        int seconds1 = dist(gen);
        int seconds2 = dist(gen);

        // MISLJNJE
        // tagovi: 0 - nemam vilicu , 1 - slanje desne, 2 - slanje
        // lijeve, 3 - zahtjev za desnu, 4 - zahtjev za lijevu
        int flag = true;
        MPI_Status st;

        std::cout << tpad << "mislim (" << seconds1 << ") sekundi" << std::endl;
        for (int i = 0; i < seconds1; i++) {
            sleep(1);
            while (flag == true) {
                flag = false;
                MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag,
                           &st);
                if (flag == true) {
                    int message;
                    int sender = st.MPI_SOURCE;
                    int tag = st.MPI_TAG;
                    MPI_Recv(NULL, 0, MPI_INT, sender, tag, MPI_COMM_WORLD,
                             NULL);
                    if (tag == 3) {
                        if (haveRight) {
                            MPI_Send(NULL, 0, MPI_INT, sender, 2,
                                     MPI_COMM_WORLD);
                            haveRight = 0;
                        } else {
                            MPI_Send(NULL, 0, MPI_INT, sender, 0,
                                     MPI_COMM_WORLD);
                        }
                    }
                    if (tag == 4) {
                        if (haveLeft) {
                            MPI_Send(NULL, 0, MPI_INT, sender, 1,
                                     MPI_COMM_WORLD);
                            haveLeft = 0;
                        } else {
                            MPI_Send(NULL, 0, MPI_INT, sender, 0,
                                     MPI_COMM_WORLD);
                        }
                    }
                }
            }
            flag = true;
        }
        int sent = 0;
        flag = false;

        MPI_Status st2;
        std::vector<MPI_Status> req;
        std::cout << tpad << "Krecem traziti" << std::endl;
        while (!(haveLeft and haveRight)) {
            if (!sent) {
                if (!haveRight) {
                    MPI_Send(NULL, 0, MPI_INT, usrRight, 4, MPI_COMM_WORLD);
                    std::cout << tpad << "trazim vilicu (" << idxForkRight
                              << ")" << std::endl;
                }
                if (!haveLeft) {
                    MPI_Send(NULL, 0, MPI_INT, usrLeft, 3, MPI_COMM_WORLD);
                    std::cout << tpad << "trazim vilicu (" << idxForkLeft << ")"
                              << std::endl;
                }
                sent = 1;
            }
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag,
                       &st2);
            if (flag == true) {
                int sender = st2.MPI_SOURCE;
                int rtag = st2.MPI_TAG;
                MPI_Status st3;
                MPI_Recv(NULL, 0, MPI_INT, sender, rtag, MPI_COMM_WORLD, &st3);
                switch (rtag) {
                case 1:
                    haveRight = 1;
                    cleanRight = 1;

                    break;
                case 2:
                    haveLeft = 1;
                    cleanLeft = 1;
                    break;
                case 3:
                    if (haveRight && !cleanRight) {
                        MPI_Send(NULL, 0, MPI_INT, sender, 2, MPI_COMM_WORLD);
                        haveRight = 0;
                        sent = 0;
                    } else {
                        // MPI_Send(NULL, 0, MPI_INT, sender, 0,
                        // MPI_COMM_WORLD);
                        req.push_back(st3);
                    }

                    break;
                case 4:
                    if (haveLeft && !cleanLeft) {
                        MPI_Send(NULL, 0, MPI_INT, sender, 1, MPI_COMM_WORLD);
                        haveLeft = 0;
                        sent = 0;
                    } else {
                        // MPI_Send(NULL, 0, MPI_INT, sender, 0,
                        // MPI_COMM_WORLD);
                        req.push_back(st3);
                    }

                    break;
                }
            }
            flag = false;
        }

        // jedenje
        std::cout << tpad << "jedem (" << seconds2 << ") sekundi" << std ::endl;
        for (int i = 0; i < seconds2; i++) {
            sleep(1);
        }
        cleanRight = 0;
        cleanLeft = 0;
        for (int i = 0; i < req.size(); i++) {
            int sender = req[i].MPI_SOURCE;
            int tag = req[i].MPI_TAG;
            // std::cout << tpad << "IMAM OD " << sender << std::endl;
            if (tag == 3) {
                if (haveRight) {
                    MPI_Send(NULL, 0, MPI_INT, sender, 2, MPI_COMM_WORLD);
                    haveRight = 0;
                }
            }
            if (tag == 4) {
                if (haveLeft) {
                    MPI_Send(NULL, 0, MPI_INT, sender, 1, MPI_COMM_WORLD);
                    haveLeft = 0;
                }
            }
        }
        req.clear();
    }

    MPI_Finalize();

    return 0;
}
