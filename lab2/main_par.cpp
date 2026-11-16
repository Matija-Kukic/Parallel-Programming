// 4 u nizu - glavni program
#include "board.h" // razred za igracu plocu
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mpi.h>
#include <queue>
#include <vector>
#define ALG_DEP 2

using namespace std;
using namespace std::chrono;
const int DEPTH = 6; // default dubina stabla

typedef struct {
    int j_size, depth;
} info;

double Evaluate(Board Current, data_t LastMover, int iLastCol, int iDepth);
void perform_move(Board Current, int argc, char **argv);
void create_jobs(Board Current, int algDep, data_t Mover, queue<Job> &jobs,
                 vector<Sol> &sol, int jid);
void send_jobs(queue<Job> &job, queue<Job> &masterJobs, int depth, int &p_num);
void collect_jobs(queue<Job> &jobs, int &depth);
void do_jobs(queue<Job> &jobs, int depth);
int parse_sol(vector<Sol> &sol, int algDep, int num_moves, int id);
void end_game();

int main(int argc, char **argv) {

    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        int p_num;
        MPI_Comm_size(MPI_COMM_WORLD, &p_num);
        Board B;
        double dResult, dBest;
        int iBestCol, iDepth = DEPTH, algDep = ALG_DEP;
        if (argc < 2) {
            cout << "Uporaba: <program> <fajl s trenutnim stanjem> [<dubina>]"
                 << endl;
            return 0;
        }
        B.Load(argv[1]);
        if (argc > 2)
            iDepth = atoi(argv[2]);
        srand((unsigned)time(NULL));
        if (argc > 3) {
            algDep = atoi(argv[3]);
        }
        int tree_num = 0, start = 1;
        for (int i = 0; i < algDep + 1; i++) {
            tree_num += start;
            start *= B.Columns();
        }
        vector<Sol> sol(tree_num);
        while (1) {
            // provjerimo jel igra vec gotova (npr. ako je igrac pobijedio)
            bool end = 0;
            for (int iCol = 0; iCol < B.Columns(); iCol++)
                if (B.GameEnd(iCol)) {
                    cout << "Igra zavrsena!" << endl;
                    B.Print();
                    end_game();
                    end = 1;
                    break;
                }
            if (end) {
                break;
            }
            // pretpostavka: na potezu je CPU
            // racunamo potez

            auto start = high_resolution_clock::now();
            queue<Job> jobs;
            create_jobs(B, algDep, HUMAN, jobs, sol, 0);
            // cout << jobs.size() << endl;
            int j_num = jobs.size();
            queue<Job> masterJobs;
            send_jobs(jobs, masterJobs, iDepth - algDep, p_num);
            int mj_num = masterJobs.size();
            int other_jobs = j_num - mj_num;
            // Master obavlja svoje poslove
            while (!masterJobs.empty()) {
                Job j = masterJobs.front();
                Board b(j.bd);
                double score =
                    Evaluate(b, j.bd.LastMover, (j.id - 1) % b.Columns(),
                             iDepth - algDep);
                masterJobs.pop();
                sol[j.id] = {1, score};
            }
            // Master skuplja svoje poslove
            // cout << "OTHER JOBS: " << other_jobs << endl;
            while (other_jobs > 0) {
                Score sc;
                MPI_Status st;
                MPI_Recv(&sc, sizeof(sc), MPI_CHAR, MPI_ANY_SOURCE, 4,
                         MPI_COMM_WORLD, &st);
                sol[sc.jid] = {1, sc.score};

                other_jobs--;
            }
            // cout << "OBAVIO SVA PRIMANJA " << endl;
            int bestMove = parse_sol(sol, algDep, B.Columns(), 0);
            B.Move(bestMove, CPU);
            auto endt = high_resolution_clock::now();
            auto duration_ms = duration_cast<milliseconds>(endt - start);
            double seconds = duration_ms.count() / 1000.0;
            cout << "Trajanje poteza: " << seconds << endl;
            B.Save(argv[1]);
            for (int i = 0; i < tree_num; i++) {
                sol[i].use = false;
                sol[i].score = 0;
            }
            // jesmo li pobijedili
            if (B.GameEnd(bestMove)) {
                cout << "Igra zavrsena! (pobjeda racunala)" << endl;
                B.Print();
                end_game();
                end = 1;
                break;
            }

            cout << "Vi ste na potezu. Upisite stupac za potez (0-indeks)"
                 << endl;
            B.Print();
            int pcol;
            cout << "Vas stupac: " << endl;
            cin >> pcol;
            if (pcol == -1) {
                end_game();
                break;
            }
            B.Move(pcol, HUMAN);
            B.Save(argv[1]);
            B.Print();
            if (argc > 2)
                iDepth = atoi(argv[2]);
        }
    } else {

        int depth;
        queue<Job> jobs;
        while (1) {
            collect_jobs(jobs, depth);
            if (jobs.size() == 0) {
                break;
            }
            // Job j = jobs.front();
            // Board b(j.bd);
            // b.Print();
            do_jobs(jobs, depth);
        }
    }
    MPI_Finalize();
}

// rekurzivna funkcija: ispituje sve moguce poteze i vraca ocjenu dobivenog
// stanja ploce Current: trenutno stanje ploce LastMover: HUMAN ili CPU
// iLastCol: stupac prethodnog poteza
// iDepth: dubina se smanjuje do 0
double Evaluate(Board Current, data_t LastMover, int iLastCol, int iDepth) {
    double dResult, dTotal;
    data_t NewMover;
    bool bAllLose = true, bAllWin = true;
    int iMoves;

    if (Current.GameEnd(iLastCol)) { // igra gotova?
        if (LastMover == CPU) {
            return 1;  // pobjeda
        } else {       // if(LastMover == HUMAN)
            return -1; // poraz
        }
    }
    // nije gotovo, idemo u sljedecu razinu
    if (iDepth == 0)
        return 0; // a mozda i ne... :)
    iDepth--;
    if (LastMover == CPU) // tko je na potezu
        NewMover = HUMAN;
    else
        NewMover = CPU;
    dTotal = 0;
    iMoves = 0; // broj mogucih poteza u ovoj razini
    for (int iCol = 0; iCol < Current.Columns(); iCol++) {
        if (Current.MoveLegal(iCol)) // jel moze u stupac iCol
        {
            iMoves++;
            Current.Move(iCol, NewMover);
            dResult = Evaluate(Current, NewMover, iCol, iDepth);
            Current.UndoMove(iCol);
            if (dResult > -1)
                bAllLose = false;
            if (dResult != 1)
                bAllWin = false;
            if (dResult == 1 && NewMover == CPU)
                return 1; // ako svojim potezom mogu doci do pobjede (pravilo 1)
            if (dResult == -1 && NewMover == HUMAN)
                return -1; // ako protivnik moze potezom doci do pobjede
                           // (pravilo 2)
            dTotal += dResult;
        }
    }
    if (bAllWin == true) // ispitivanje za pravilo 3.
        return 1;
    if (bAllLose == true)
        return -1;
    dTotal /=
        iMoves; // dijelimo ocjenu s brojem mogucih poteza iz zadanog stanja
    return dTotal;
}
void create_jobs(Board Current, int depth, data_t Mover, queue<Job> &jobs,
                 vector<Sol> &sol, int jid) {
    if (depth == 0) {
        // Current.Print();
        BoardData bd = Current.toData();
        jobs.push({bd, jid});
        return;
    }
    data_t NewMover;
    NewMover = Mover == CPU ? HUMAN : CPU;
    int lastM = jid == 0 ? 0 : (jid - 1) % Current.Columns();
    if (Current.GameEnd(lastM)) { // igra gotova? jid % 7 daje
                                  // stupac koji je odigran
        if (Mover == CPU) {
            sol[jid].score = 1;
            sol[jid].use = 1;
            return;
        } else {
            sol[jid].score = -1;
            sol[jid].use = 1;
            return;
        }
    }

    for (int i = 0; i < Current.Columns(); i++) {
        if (Current.MoveLegal(i)) {
            sol[jid * Current.Columns() + i + 1].use = 1;
            Current.Move(i, NewMover);
            create_jobs(Current, depth - 1, NewMover, jobs, sol,
                        jid * Current.Columns() + i + 1);
            Current.UndoMove(i);
        } else {
            sol[jid * Current.Columns() + i + 1].use = 0;
        }
    }
    return;
}
void send_jobs(queue<Job> &jobs, queue<Job> &masterJobs, int depth,
               int &p_num) {
    int j_num = jobs.size();

    int amount[p_num];
    for (int i = 0; i < p_num; i++) {
        amount[i] = j_num / p_num;
        if (i < j_num % p_num)
            amount[i]++;
    }

    for (int i = 0; i < amount[0]; i++) {
        masterJobs.push(jobs.front());
        jobs.pop();
    }
    int pnum = p_num;
    for (int i = 1; i < pnum; i++) {
        char give;
        MPI_Status st;
        MPI_Recv(&give, 1, MPI_CHAR, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &st);
        int from = st.MPI_SOURCE;

        int info[2] = {amount[from], depth};
        if (amount[from] == 0) {
            p_num--;
            cout << "NOVI PNUM" << p_num << endl;
        }

        MPI_Send(&info, 2, MPI_INT, from, 2, MPI_COMM_WORLD);

        for (int k = 0; k < info[0]; k++) {
            Job j = jobs.front();
            MPI_Send(&j, sizeof(Job), MPI_CHAR, from, 3, MPI_COMM_WORLD);
            jobs.pop();
        }
    }
}

void collect_jobs(queue<Job> &jobs, int &depth) {

    char give = 1;
    MPI_Send(&give, 1, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
    int info[2];
    MPI_Status st;
    MPI_Recv(&info, 2, MPI_INT, 0, 2, MPI_COMM_WORLD, &st);
    if (info[0] == -1 || info[0] == 0) {
        return;
    }
    for (int i = 0; i < info[0]; i++) {
        Job j;
        MPI_Recv(&j, sizeof(j), MPI_CHAR, 0, 3, MPI_COMM_WORLD, &st);
        jobs.push(j);
    }
    depth = info[1];
}

void do_jobs(queue<Job> &jobs, int depth) {
    while (!jobs.empty()) {
        Job j = jobs.front();
        Board b(j.bd);
        double score =
            Evaluate(b, j.bd.LastMover, (j.id - 1) % b.Columns(), depth);
        jobs.pop();
        Score sc = {j.id, score};
        MPI_Send(&sc, sizeof(sc), MPI_CHAR, 0, 4, MPI_COMM_WORLD);
    }
}
double get_score(vector<Sol> &sol, int algDep, int num_moves, int id,
                 data_t LastMover) {
    if (!sol[id].use) {
        return 0.0;
    }
    if (algDep == 0) {
        return sol[id].score;
    }

    data_t NewMover = (LastMover == CPU) ? HUMAN : CPU;
    bool bAllLose = true, bAllWin = true;
    int moves = 0;
    double total = 0;

    for (int i = 0; i < num_moves; i++) {
        int child_id = id * num_moves + i + 1;
        if (sol[child_id].use) {
            moves++;
            double score =
                get_score(sol, algDep - 1, num_moves, child_id, NewMover);

            if (score > -1)
                bAllLose = false;
            if (score != 1)
                bAllWin = false;

            if (score == 1 && NewMover == CPU)
                return 1.0;

            if (score == -1 && NewMover == HUMAN)
                return -1.0;

            total += score;
        }
    }

    if (bAllWin && moves > 0)
        return 1.0;
    if (bAllLose && moves > 0)
        return -1.0;

    if (moves != 0) {
        total /= moves;
        return total;
    }
    return sol[id].score;
}

int parse_sol(vector<Sol> &sol, int algDep, int num_moves, int id) {
    int best = 0;
    double bscore = -INFINITY;

    data_t LastMover = CPU;

    for (int i = 0; i < num_moves; i++) {
        int child_id = id * num_moves + i + 1;
        if (sol[child_id].use) {
            double score =
                get_score(sol, algDep - 1, num_moves, child_id, LastMover);

            if (score > bscore || (score == bscore && rand() % 2 == 0)) {
                best = i;
                bscore = score;
            }
            cout << "STUPAC (" << i << ") SCORE: " << score << endl;
        } else {
            cout << "STUPAC (" << i << ") SCORE: NELEGALAN" << endl;
        }
    }
    cout << "ODABRAO: " << best << endl;
    return best;
}
void end_game() {

    int p_num;
    MPI_Comm_size(MPI_COMM_WORLD, &p_num);
    char give;
    MPI_Status st;
    for (int i = 0; i < p_num - 1; i++) {
        MPI_Recv(&give, 1, MPI_CHAR, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &st);
        int from = st.MPI_SOURCE;
        int info[2] = {-1, -1}; // num_jobs,search depth
        MPI_Send(&info, 2, MPI_INT, from, 2, MPI_COMM_WORLD);
    }
}
