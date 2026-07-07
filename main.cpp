#include <cctype>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================
// Data model
// ============================================================

struct Board
{
    std::vector<std::vector<std::string>> grid;
};

// Carries a VPL-style error code (e.g. "UNKNOWN_TOKEN").
// Thrown by validate_board, caught once in main().
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
// Small string helpers
// ============================================================

std::string trim(const std::string &value)
{
    size_t start = 0;
    while (start < value.size() &&
           std::isspace(static_cast<unsigned char>(value[start])))
    {
        ++start;
    }
    size_t end = value.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(value[end - 1])))
    {
        --end;
    }
    return value.substr(start, end - start);
}

std::vector<std::string> splitWords(const std::string &line)
{
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

// ============================================================
// Layer 1 — parse_sections
// Responsibility: split raw stdin text into the "Board:" lines
// and the "Commands:" lines. Nothing else. No interpretation of
// tokens, no validation.
// ============================================================

struct Sections
{
    std::vector<std::string> boardLines;
    std::vector<std::string> commandLines;
};

Sections parse_sections(const std::string &text)
{
    Sections sections;
    std::istringstream stream(text);
    std::string line;
    bool inBoard = false;
    bool inCommands = false;

    while (std::getline(stream, line))
    {
        std::string trimmed = trim(line);

        if (trimmed == "Board:")
        {
            inBoard = true;
            inCommands = false;
            continue;
        }
        if (trimmed == "Commands:")
        {
            inBoard = false;
            inCommands = true;
            continue;
        }
        if (trimmed.empty())
        {
            continue;
        }

        if (inBoard)
        {
            sections.boardLines.push_back(trimmed);
        }
        else if (inCommands)
        {
            sections.commandLines.push_back(trimmed);
        }
    }
    return sections;
}

// ============================================================
// Layer 2 — parse_board
// Responsibility: turn board text lines into a Board (grid of
// tokens). Purely structural — does NOT judge whether tokens or
// row widths are valid. That is validate_board's job.
// ============================================================

Board parse_board(const std::vector<std::string> &boardLines)
{
    Board board;
    for (const std::string &line : boardLines)
    {
        board.grid.push_back(splitWords(line));
    }
    return board;
}

// ============================================================
// Layer 3 — validate_board
// Responsibility: judge whether the parsed Board is well-formed.
//
// Design decision (documented on purpose): row-width consistency
// is checked BEFORE token validity. Rationale: without a
// consistent number of columns per row, the board has no
// well-defined shape to validate tokens against in the first
// place. If the spec later requires a different priority when
// both problems occur together, only this function needs to
// change.
// ============================================================

bool isValidToken(const std::string &token)
{
    if (token == ".")
    {
        return true;
    }
    if (token.size() != 2)
    {
        return false;
    }
    char color = token[0];
    char piece = token[1];
    if (color != 'w' && color != 'b')
    {
        return false;
    }
    switch (piece)
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

void validate_board(const Board &board)
{
    if (board.grid.empty())
    {
        return;
    }

    size_t expectedCols = board.grid[0].size();
    for (const auto &row : board.grid)
    {
        if (row.size() != expectedCols)
        {
            throw BoardError("ROW_WIDTH_MISMATCH");
        }
    }

    for (const auto &row : board.grid)
    {
        for (const std::string &token : row)
        {
            if (!isValidToken(token))
            {
                throw BoardError("UNKNOWN_TOKEN");
            }
        }
    }
}

// ============================================================
// Layer 4 — format_board
// Responsibility: turn a valid Board back into canonical text.
// One row per line, tokens separated by a single space, every
// line (including the last) terminated by '\n'.
// ============================================================

std::string format_board(const Board &board)
{
    std::ostringstream out;
    for (const auto &row : board.grid)
    {
        for (size_t j = 0; j < row.size(); ++j)
        {
            if (j > 0)
            {
                out << ' ';
            }
            out << row[j];
        }
        out << '\n';
    }
    return out.str();
}

// ============================================================
// Layer 5 — run_commands
// Responsibility: execute the command list via a dispatch table,
// so future iterations can add new commands without touching
// parsing or validation at all.
// ============================================================

using CommandHandler = std::function<void(const Board &)>;

void run_commands(const std::vector<std::string> &commands, const Board &board)
{
    static const std::unordered_map<std::string, CommandHandler> handlers = {
        {"print board", [](const Board &b)
         { std::cout << format_board(b); }},
    };

    for (const std::string &command : commands)
    {
        auto it = handlers.find(trim(command));
        if (it != handlers.end())
        {
            it->second(board);
        }
        // Unknown commands are silently ignored in this iteration;
        // only "print board" is defined so far.
    }
}

// ============================================================
// main — wires the layers together, owns I/O and error handling.
// ============================================================

int main()
{
    std::string input;
    std::string line;
    while (std::getline(std::cin, line))
    {
        input += line;
        input += '\n';
    }

    Sections sections = parse_sections(input);
    Board board = parse_board(sections.boardLines);

    try
    {
        validate_board(board);
    }
    catch (const BoardError &error)
    {
        std::cout << "ERROR " << error.code() << '\n';
        return 0;
    }

    run_commands(sections.commandLines, board);
    return 0;
}