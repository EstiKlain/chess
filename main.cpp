#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace config
{

    constexpr int CELL_SIZE = 100;

    constexpr int NUM_PLAYERS = 2;

    struct PieceStats
    {
        double speedCellsPerSec;
        long restMs;
    };

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
            return {0.0, 0};
        }
    }

    using MoveShapeFn = std::function<bool(int dRow, int dCol)>;

    inline bool kingShape(int dRow, int dCol)
    {
        return (dRow != 0 || dCol != 0) && std::abs(dRow) <= 1 && std::abs(dCol) <= 1;
    }
    inline bool rookShape(int dRow, int dCol)
    {
        return (dRow == 0) != (dCol == 0);
    }
    inline bool bishopShape(int dRow, int dCol)
    {
        return dRow != 0 && std::abs(dRow) == std::abs(dCol);
    }
    inline bool queenShape(int dRow, int dCol)
    {
        return rookShape(dRow, dCol) || bishopShape(dRow, dCol);
    }
    inline bool knightShape(int dRow, int dCol)
    {
        int r = std::abs(dRow), c = std::abs(dCol);
        return (r == 1 && c == 2) || (r == 2 && c == 1);
    }

    struct MoveRule
    {
        MoveShapeFn shape;
        bool slides;
    };

    inline std::map<char, MoveRule> moveShapes = {
        {'K', {kingShape, false}},
        {'Q', {queenShape, true}},
        {'R', {rookShape, true}},
        {'B', {bishopShape, true}},
        {'N', {knightShape, false}},
    };
}

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
    long selectedAtMs = 0;
};

struct PieceMove
{
    int fromRow, fromCol;
    int toRow, toCol;
    long startMs;
    long durationMs;
    std::string piece;
};

struct GameState
{
    Board board;
    long elapsedMs = 0;
    std::vector<Selection> selections;
    std::vector<PieceMove> activeMoves;
};

class BoardError : public std::runtime_error
{
public:
    explicit BoardError(const std::string &code)
        : std::runtime_error(code), code_(code) {}
    const std::string &code() const { return code_; }

private:
    std::string code_;
};

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

Board parseBoard(const std::vector<std::string> &boardLines)
{
    Board b;
    for (const auto &line : boardLines)
        b.grid.push_back(splitWords(line));
    return b;
}

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

bool isEmpty(const std::string &tok) { return tok == "."; }
char colorOf(const std::string &tok) { return tok[0]; }
char pieceOf(const std::string &tok) { return tok[1]; }

int sign(int v) { return (v > 0) - (v < 0); }

bool isPathClear(const Board &board, int fromRow, int fromCol, int toRow, int toCol)
{
    int stepRow = sign(toRow - fromRow);
    int stepCol = sign(toCol - fromCol);

    int r = fromRow + stepRow, c = fromCol + stepCol;
    while (r != toRow || c != toCol)
    {
        if (!isEmpty(board.grid[r][c]))
            return false;
        r += stepRow;
        c += stepCol;
    }
    return true;
}

int playerIndexOf(char color)
{
    switch (color)
    {
    case 'w':
        return 0;
    case 'b':
        return 1;
    default:
        return -1;
    }
}

double cellDistance(int r1, int c1, int r2, int c2)
{
    double dr = r2 - r1, dc = c2 - c1;
    return std::sqrt(dr * dr + dc * dc);
}

bool isLegalMove(const Board &board, const PieceMove &move, char piece)
{
    auto it = config::moveShapes.find(piece);
    if (it == config::moveShapes.end())
        return true; // no rule registered yet (e.g. pawn) -> unrestricted for now

    const config::MoveRule &rule = it->second;

    int dRow = move.toRow - move.fromRow;
    int dCol = move.toCol - move.fromCol;
    if (!rule.shape(dRow, dCol))
        return false;

    if (rule.slides && !isPathClear(board, move.fromRow, move.fromCol, move.toRow, move.toCol))
        return false;

    const std::string &destination = board.grid[move.toRow][move.toCol];
    if (!isEmpty(destination) && colorOf(destination) == move.piece[0])
        return false;

    return true;
}

void resolveMoves(GameState &st)
{
    std::vector<size_t> due;
    std::vector<PieceMove> stillMoving;
    for (size_t i = 0; i < st.activeMoves.size(); ++i)
    {
        const PieceMove &m = st.activeMoves[i];
        if (st.elapsedMs >= m.startMs + m.durationMs)
            due.push_back(i);
        else
            stillMoving.push_back(m);
    }

    std::sort(due.begin(), due.end(), [&](size_t a, size_t b)
              {
        long ta = st.activeMoves[a].startMs + st.activeMoves[a].durationMs;
        long tb = st.activeMoves[b].startMs + st.activeMoves[b].durationMs;
        if (ta != tb) return ta < tb;
        return a < b; });

    for (size_t idx : due)
    {
        const PieceMove &m = st.activeMoves[idx];
        std::string &target = st.board.grid[m.toRow][m.toCol];
        if (isEmpty(target) || colorOf(target) != m.piece[0])
        {
            target = m.piece;
        }
        else
        {
            std::string &origin = st.board.grid[m.fromRow][m.fromCol];
            if (isEmpty(origin))
                origin = m.piece;
        }
    }

    st.activeMoves = stillMoving;
}

int oldestPendingSide(const GameState &st)
{
    int side = -1;
    for (size_t i = 0; i < st.selections.size(); ++i)
    {
        if (!st.selections[i].active)
            continue;
        if (side < 0 || st.selections[i].selectedAtMs < st.selections[side].selectedAtMs)
            side = (int)i;
    }
    return side;
}

void sendMove(GameState &st, int side, int toRow, int toCol)
{
    Selection &sel = st.selections[side];
    const std::string selected = st.board.grid[sel.row][sel.col];

    PieceMove m;
    m.fromRow = sel.row;
    m.fromCol = sel.col;
    m.toRow = toRow;
    m.toCol = toCol;
    m.startMs = st.elapsedMs;
    m.piece = selected;

    char piece = pieceOf(selected);
    double speed = config::statsFor(piece).speedCellsPerSec;
    double dist = cellDistance(m.fromRow, m.fromCol, toRow, toCol);
    m.durationMs = (speed > 0.0) ? (long)(dist / speed * 1000.0) : 0;

    if (isLegalMove(st.board, m, piece))
    {
        st.board.grid[m.fromRow][m.fromCol] = ".";
        st.activeMoves.push_back(m);
    }
    sel = Selection{};
}

void handleClick(GameState &st, int x, int y)
{
    int col = x / config::CELL_SIZE;
    int row = y / config::CELL_SIZE;

    if (row < 0 || row >= st.board.rows() ||
        col < 0 || col >= st.board.cols())
        return;

    const std::string &token = st.board.grid[row][col];

    if (!isEmpty(token))
    {
        int side = playerIndexOf(colorOf(token));
        if (side < 0 || side >= (int)st.selections.size())
            return;

        if (st.selections[side].active)
        {
            st.selections[side] = {true, row, col, st.elapsedMs};
            return;
        }

        int target = oldestPendingSide(st);
        if (target >= 0)
        {
            sendMove(st, target, row, col);
            return;
        }

        st.selections[side] = {true, row, col, st.elapsedMs};
        return;
    }

    int target = oldestPendingSide(st);
    if (target >= 0)
        sendMove(st, target, row, col);
}

void handleWait(GameState &st, long ms)
{
    st.elapsedMs += ms;
    resolveMoves(st);
}

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
    }
}

int main()
{
    std::string input, line;
    while (std::getline(std::cin, line))
        input += line + '\n';

    Sections sections = parseSections(input);

    GameState state;
    state.board = parseBoard(sections.boardLines);
    state.selections.assign(config::NUM_PLAYERS, Selection{});

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