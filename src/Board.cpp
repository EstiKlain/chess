#include "Board.hpp"

bool isEmpty(const std::string& tok) { return tok == "."; }
char colorOf(const std::string& tok) { return tok[0]; }
char pieceOf(const std::string& tok) { return tok[1]; }