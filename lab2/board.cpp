// board.cpp

#include "board.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>

using namespace std;

#ifndef CHECK
    #define CHECKMSG(condition, text)                                          \
        if (!(condition)) {                                                    \
            fprintf(stderr, "file: " __FILE__ "\nline: %d\nmsg: " text "\n",   \
                    __LINE__);                                                 \
            exit(1);                                                           \
        }

    #define CHECK(condition)                                                   \
        if (!(condition)) {                                                    \
            fprintf(stderr,                                                    \
                    "Assertion failed!\nfile: " __FILE__ "\nline: %d\n",       \
                    __LINE__);                                                 \
            exit(1);                                                           \
        }
#endif

Board::Board(BoardData bd) {
    rows = bd.rows;
    cols = bd.cols;
    LastMover = bd.LastMover; // Kopiramo zadnjeg igrača
    lastcol = -1;             // Podrazumijevana vrijednost

    Take();

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            field[i][j] = bd.field[i][j];
        }
    }

    // KLJUČNI POPRAVAK: Rekonstrukcija polja height[] iz ucitanog polja
    for (int c = 0; c < cols; c++) {
        int h = 0;
        while (h < rows && field[h][c] != EMPTY) {
            h++;
        }
        height[c] = h;
    }
}

void Board::Free(void) {
    if (field == NULL)
        return;

    for (int i = 0; i < rows; i++)
        delete[] field[i];

    delete[] field;
    delete[] height;

    field = NULL;
    height = NULL;
}

void Board::Take(void) {
    field = new data_t *[rows];

    for (int i = 0; i < rows; i++) {
        field[i] = new data_t[cols];

        for (int j = 0; j < cols; j++)
            field[i][j] = EMPTY;
    }

    height = new int[cols];

    for (int i = 0; i < cols; i++)
        height[i] = 0;
}

Board::Board(const Board &src) {
    rows = src.rows;
    cols = src.cols;
    LastMover = src.LastMover;
    lastcol = src.lastcol;

    Take();

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            field[i][j] = src.field[i][j];

    for (int i = 0; i < cols; i++)
        height[i] = src.height[i];
}

bool Board::MoveLegal(const int col) {
    assert(col >= 0 && col < cols);

    return (height[col] < rows);
}

bool Board::Move(const int col, const data_t player) {
    if (!MoveLegal(col))
        return false;

    field[height[col]][col] = player;
    height[col]++;

    LastMover = player;
    lastcol = col;

    return true;
}

bool Board::UndoMove(const int col) {
    assert(col >= 0 && col < cols);

    if (height[col] == 0)
        return false;

    height[col]--;
    field[height[col]][col] = EMPTY;

    return true;
}

bool Board::GameEnd(const int lastcol) {
    assert(lastcol >= 0 && lastcol < cols);

    int col = lastcol;
    int row = height[col] - 1;

    if (row < 0 || row >= rows)
        return false;

    int player = field[row][col];

    int seq;
    int r, c;

    // vertikalno
    seq = 1;
    r = row - 1;

    while (r >= 0 && field[r][col] == player) {
        seq++;
        r--;
    }

    if (seq >= 4)
        return true;

    // horizontalno
    seq = 1;

    c = col - 1;
    while (c >= 0 && field[row][c] == player) {
        seq++;
        c--;
    }

    c = col + 1;
    while (c < cols && field[row][c] == player) {
        seq++;
        c++;
    }

    if (seq >= 4)
        return true;

    // dijagonala /
    seq = 1;

    r = row + 1;
    c = col - 1;

    while (r < rows && c >= 0 && field[r][c] == player) {
        seq++;
        r++;
        c--;
    }

    r = row - 1;
    c = col + 1;

    while (r >= 0 && c < cols && field[r][c] == player) {
        seq++;
        r--;
        c++;
    }

    if (seq >= 4)
        return true;

    // dijagonala
    seq = 1;

    r = row - 1;
    c = col - 1;

    while (r >= 0 && c >= 0 && field[r][c] == player) {
        seq++;
        r--;
        c--;
    }

    r = row + 1;
    c = col + 1;

    while (r < rows && c < cols && field[r][c] == player) {
        seq++;
        r++;
        c++;
    }

    if (seq >= 4)
        return true;

    return false;
}

bool Board::Load(const char *fname) {
    FILE *fp;
    data_t value;

    fp = fopen(fname, "r");

    if (fp == NULL)
        CHECKMSG(0, "Nema datoteke!");

    // prvo oslobodi staru memoriju
    Free();

    // ucitaj dimenzije
    if (fscanf(fp, "%d %d", &rows, &cols) != 2)
        CHECK(0);

    // alociraj novu plocu
    Take();

    // ucitaj polja
    for (int r = rows - 1; r >= 0; r--) {
        for (int c = 0; c < cols; c++) {

            if (fscanf(fp, "%d", &value) != 1)
                CHECK(0);

            field[r][c] = value;
        }
    }

    fclose(fp);

    // rekonstrukcija height[]
    for (int c = 0; c < cols; c++) {

        int h = 0;

        while (h < rows && field[h][c] != EMPTY)
            h++;

        height[c] = h;
    }

    return true;
}

void Board::Save(const char *fname) {
    FILE *fp;

    fp = fopen(fname, "w");

    if (fp == NULL)
        CHECKMSG(0, "Ne mogu otvoriti datoteku!");

    fprintf(fp, "%d %d\n", rows, cols);

    for (int r = rows - 1; r >= 0; r--) {

        for (int c = 0; c < cols; c++)
            fprintf(fp, "%d ", field[r][c]);

        fprintf(fp, "\n");
    }

    fclose(fp);
}
void Board::Print() {
    cout << "IDX: 0 1 2 3 4 5 6" << endl;
    cout << "----------------------------" << endl;
    for (int i = rows - 1; i >= 0; --i) {
        cout << "     ";
        for (int j = 0; j < cols; j++) {
            cout << field[i][j] << " ";
        }
        cout << endl;
    }

    cout << "----------------------------" << endl;
}

BoardData Board::toData() {
    BoardData bd;
    std::memset(&bd, 0, sizeof(BoardData));

    bd.cols = cols;
    bd.rows = rows;
    bd.LastMover = LastMover;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            bd.field[i][j] = field[i][j];
        }
    }
    return bd;
}
