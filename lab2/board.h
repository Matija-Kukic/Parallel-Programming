// Deklaracija razreda Board

#include <assert.h>
typedef int data_t;
const data_t EMPTY = 0;
const data_t CPU = 1;
const data_t HUMAN = 2;

typedef struct {
    data_t field[6][7];
    data_t LastMover;
    int rows, cols;
} BoardData;

typedef struct {
    BoardData bd;
    int id;
} Job;

typedef struct {
    bool use;
    double score;
} Sol;
typedef struct {
    int jid;
    double score;
} Score;

class Board {
  private:
    data_t **field;
    int *height;
    int rows, cols;
    data_t LastMover;
    int lastcol;
    void Take(); // zauzmi i popuni prazninama
    void Free();

  public:
    Board() : rows(6), cols(7), LastMover(EMPTY), lastcol(-1) { Take(); }
    Board(const int row, const int col)
        : rows(row), cols(col), LastMover(EMPTY), lastcol(-1) {
        Take();
    }
    ~Board() { Free(); }
    int Columns() // broj stupaca
    {
        return cols;
    }
    Board(const Board &src);
    Board(BoardData bd);
    data_t *operator[](const int row);
    bool MoveLegal(const int col);                 // moze li potez u stupcu col
    bool Move(const int col, const data_t player); // napravi potez
    bool UndoMove(const int col);                  // vrati potez iz stupca
    bool GameEnd(const int lastcol);               // je li zavrsno stanje
    bool Load(const char *fname);
    void Save(const char *fname);
    BoardData toData();
    void Print();
};

inline data_t *Board::operator[](const int row) {
    assert(row >= 0 && row < rows);
    return field[row];
}
