#include "rules/CaptureRule.hpp"

bool capturesOnArrival(char moverColor, char residentColor)
{
    return moverColor != residentColor;
}
