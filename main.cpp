#include <cctype>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================
// CONFIG — every invented / tunable number lives here ONLY.
// When the assignment gives real values, change them here and
// nowhere else. The engine below never hard-codes these.
// ============================================================
namespace config
{

    // Pixel size of one cell: click x y -> cell (x / CELL_SIZE, y / CELL_SIZE).
    constexpr int CELL_SIZE = 100;

    // Per-piece tunables. Add fields here (e.g. a movement pattern)
    // as future iterations introduce new rules.
    struct PieceStats
    {
        double speedCellsPerSec; // travel speed
        long restMs;             // cooldown after arriving (future; not enforced yet)
    };

    // !!! PLACEHOLDER VALUES — NOT from the spec. Replace when known. !!!
    inline PieceStats statsFor(char piece)
    {
        switch (piece)
        {
        case 'Q':
            return {4.0, 0};
        case 'R':
            return {3.0, 0};
        case 'B':
            return {3.0, 0};
        case 'N':
            return {3.5, 0};
        case 'K':
            return {3.0, 0};
        case 'P':
            return {2.0, 0};
        default:
            return {0.0, 0}; // unreachable after validation
        }
    }
}

// ============================================================
// DATA MODEL
// ============================================================
struct Board
{
    std::vector<std::vector<std::string>> grid;
    int rows() const { return (int)grid.size(); }
    int cols() const { return grid.empty() ? 0 : (int)grid[0].size(); }
};

struct Selection
{
    bool active = false;
    int row = 0, col = 0;
};

struct PieceMove
{
    int fromRow, fromCol;
    int toRow, toCol;
    long startMs;
    long durationMs;
};

struct GameState
{
    Board board;
    long elapsedMs = 0;
    Selection selection;
    std::vector<PieceMove> activeMoves;
};

// Error code carrier (e.g. "UNKNOWN_TOKEN"); thrown by validate, caught in main.
class BoardError : public std::runtime_error
{
public:
    explicit BoardError(const std::string &code)
        : std::runtime_error(code), code_(code) {}
    const std::string &code() const { return code_; }

private:
    std::string code_;
};

// ============================================================
// STRING HELPERS
// ============================================================
std::string trim(const std::string &v)
{
    size_t a = 0, b = v.size();
    while (a < b && std::isspace((unsigned char)v[a]))
        ++a;
    while (b > a && std::isspace((unsigned char)v[b - 1]))
        --b;
    return v.substr(a, b - a);
}

std::vector<std::string> splitWords(const std::string &line)
{
    std::vector<std::string> out;
    std::istringstream ss(line);
    std::string tok;
    while (ss >> tok)
        out.push_back(tok);
    return out;
}

// ============================================================
// LAYER 1 — split input into Board / Commands sections
// ============================================================
struct Sections
{
    std::vector<std::string> boardLines;
    std::vector<std::string> commandLines;
};

Sections parseSections(const std::string &text)
{
    Sections s;
    std::istringstream stream(text);
    std::string line;
    enum
    {
        NONE,
        BOARD,
        COMMANDS
    } where = NONE;

    while (std::getline(stream, line))
    {
        std::string t = trim(line);
        if (t == "Board:")
        {
            where = BOARD;
            continue;
        }
        if (t == "Commands:")
        {
            where = COMMANDS;
            continue;
        }
        if (t.empty())
        {
            continue;
        }
        if (where == BOARD)
            s.boardLines.push_back(t);
        else if (where == COMMANDS)
            s.commandLines.push_back(t);
    }
    return s;
}

// ============================================================
// LAYER 2 — build the board
// ============================================================
Board parseBoard(const std::vector<std::string> &boardLines)
{
    Board b;
    for (const auto &line : boardLines)
        b.grid.push_back(splitWords(line));
    return b;
}

// ============================================================
// LAYER 3 — validation
// ============================================================
bool isValidToken(const std::string &t)
{
    if (t == ".")
        return true;
    if (t.size() != 2)
        return false;
    if (t[0] != 'w' && t[0] != 'b')
        return false;
    switch (t[1])
    {
    case 'K':
    case 'Q':
    case 'R':
    case 'B':
    case 'N':
    case 'P':
        return true;
    default:
        return false;
    }
}

void validateBoard(const Board &b)
{
    if (b.grid.empty())
        return;

    size_t expected = b.grid[0].size();
    for (const auto &row : b.grid) // structural check
        if (row.size() != expected)
            throw BoardError("ROW_WIDTH_MISMATCH");

    for (const auto &row : b.grid) // token check
        for (const auto &tok : row)
            if (!isValidToken(tok))
                throw BoardError("UNKNOWN_TOKEN");
}

// ============================================================
// LAYER 4 — formatting
// ============================================================
std::string formatBoard(const Board &b)
{
    std::ostringstream out;
    for (const auto &row : b.grid)
    {
        for (size_t j = 0; j < row.size(); ++j)
        {
            if (j)
                out << ' ';
            out << row[j];
        }
        out << '\n';
    }
    return out.str();
}

// ============================================================
// MOVE ENGINE
// ============================================================
bool isEmpty(const std::string &tok) { return tok == "."; }
char colorOf(const std::string &tok) { return tok[0]; }
char pieceOf(const std::string &tok) { return tok[1]; }

double cellDistance(int r1, int c1, int r2, int c2)
{
    double dr = r2 - r1, dc = c2 - c1;
    return std::sqrt(dr * dr + dc * dc);
}

// EXTENSION SEAM — per-piece legality. Future iteration fills this in.
// Returns true today so behaviour is unchanged.
bool isLegalMove(const Board & /*board*/, const PieceMove & /*move*/, char /*piece*/)
{
    return true;
}

// A move settles only once the clock passes its arrival time.
// EXTENSION SEAM: rest/cooldown (config.restMs) will be applied here on arrival.
void resolveMoves(GameState &st)
{
    std::vector<PieceMove> stillMoving;
    for (const auto &m : st.activeMoves)
    {
        if (st.elapsedMs >= m.startMs + m.durationMs)
        {
            std::string piece = st.board.grid[m.fromRow][m.fromCol];
            st.board.grid[m.fromRow][m.fromCol] = ".";
            st.board.grid[m.toRow][m.toCol] = piece;
        }
        else
        {
            stillMoving.push_back(m);
        }
    }
    st.activeMoves = stillMoving;
}

void cancelMoveFrom(GameState &st, int row, int col)
{
    std::vector<PieceMove> kept;
    for (const auto &m : st.activeMoves)
        if (!(m.fromRow == row && m.fromCol == col))
            kept.push_back(m);
    st.activeMoves = kept;
}

// ============================================================
// COMMAND HANDLERS
// ============================================================
void handleClick(GameState &st, int x, int y)
{
    int col = x / config::CELL_SIZE;
    int row = y / config::CELL_SIZE;

    if (row < 0 || row >= st.board.rows() ||
        col < 0 || col >= st.board.cols())
        return; // outside board — ignored

    const std::string &token = st.board.grid[row][col];

    if (!st.selection.active)
    {
        if (!isEmpty(token))
            st.selection = {true, row, col};
        return; // empty + no selection — ignored
    }

    const std::string &selected =
        st.board.grid[st.selection.row][st.selection.col];

    if (!isEmpty(token) && colorOf(token) == colorOf(selected))
    {
        st.selection = {true, row, col}; // friendly — replace selection
        return;
    }

    // Build a move request from the selected piece to the target cell.
    PieceMove m;
    m.fromRow = st.selection.row;
    m.fromCol = st.selection.col;
    m.toRow = row;
    m.toCol = col;
    m.startMs = st.elapsedMs;

    char piece = pieceOf(selected);
    double speed = config::statsFor(piece).speedCellsPerSec;
    double dist = cellDistance(m.fromRow, m.fromCol, m.toRow, m.toCol);
    m.durationMs = (speed > 0.0) ? (long)(dist / speed * 1000.0) : 0;

    if (isLegalMove(st.board, m, piece))
    {
        cancelMoveFrom(st, m.fromRow, m.fromCol); // one move per piece at a time
        st.activeMoves.push_back(m);
    }
    st.selection = Selection{}; // selection clears once a move is sent
}

void handleWait(GameState &st, long ms)
{
    st.elapsedMs += ms;
    resolveMoves(st);
}

// ============================================================
// LAYER 5 — command dispatch
// ============================================================
void runCommands(const std::vector<std::string> &commands, GameState &st)
{
    for (const auto &command : commands)
    {
        std::istringstream ss(command);
        std::string verb;
        ss >> verb;

        if (verb == "click")
        {
            int x, y;
            ss >> x >> y;
            handleClick(st, x, y);
        }
        else if (verb == "wait")
        {
            long ms;
            ss >> ms;
            handleWait(st, ms);
        }
        else if (verb == "print")
        {
            std::string rest;
            std::getline(ss, rest);
            if (trim(rest) == "board")
                std::cout << formatBoard(st.board);
        }
        // unknown commands ignored this iteration
    }
}

// ============================================================
// MAIN
// ============================================================
int main()
{
    std::string input, line;
    while (std::getline(std::cin, line))
        input += line + '\n';

    Sections sections = parseSections(input);

    GameState state;
    state.board = parseBoard(sections.boardLines);

    try
    {
        validateBoard(state.board);
    }
    catch (const BoardError &e)
    {
        std::cout << "ERROR " << e.code() << '\n';
        return 0;
    }

    runCommands(sections.commandLines, state);
    return 0;
}