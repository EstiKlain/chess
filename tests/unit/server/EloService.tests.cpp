#include "doctest.h"

#include "server/application/EloService.hpp"

TEST_CASE("EloService: equal ratings - winner gains and loser loses the same, symmetric amount") {
    const EloService::EloUpdate update = EloService::apply(1200, 1200);

    CHECK(update.winnerNewElo == 1216);  // 1200 + 32 * (1 - 0.5)
    CHECK(update.loserNewElo == 1184);   // 1200 + 32 * (0 - 0.5)
}

TEST_CASE("EloService: a lower-rated player winning against a higher-rated one gains more than 16") {
    const EloService::EloUpdate update = EloService::apply(1200, 1600);

    CHECK(update.winnerNewElo > 1216);
    CHECK(update.loserNewElo < 1584);
}

TEST_CASE("EloService: a higher-rated player winning against a lower-rated one gains less than 16") {
    const EloService::EloUpdate update = EloService::apply(1600, 1200);

    CHECK(update.winnerNewElo < 1616);
    CHECK(update.winnerNewElo > 1600);
    CHECK(update.loserNewElo > 1184);
    CHECK(update.loserNewElo < 1200);
}

TEST_CASE("EloService: rating changes are symmetric between winner and loser") {
    const EloService::EloUpdate update = EloService::apply(1400, 1400);

    const int winnerGain = update.winnerNewElo - 1400;
    const int loserLoss = 1400 - update.loserNewElo;
    CHECK(winnerGain == loserLoss);
}
