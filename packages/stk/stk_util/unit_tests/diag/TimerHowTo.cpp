#include <gtest/gtest.h>
#include <stk_util/diag/PrintTimer.hpp>
#include <stk_util/diag/Timer.hpp>
#include <comparison/stringAndNumberComparisons.h>

namespace
{

const double tolerance = 5e-2;

void doWork()
{
    ::usleep(1e5);
}

TEST(StkDiagTimerHowTo, useTheRootTimer)
{
    stk::diag::TimerSet enabledTimerSet(0);
    stk::diag::Timer rootTimer = createRootTimer("totalTestRuntime", enabledTimerSet);

    {
        stk::diag::TimeBlock totalTestRuntime(rootTimer);
        doWork();

        std::ostringstream outputStream;
        bool printTimingsOnlySinceLastPrint = false;
        stk::diag::printTimersTable(outputStream, rootTimer, stk::diag::METRICS_ALL, printTimingsOnlySinceLastPrint);

        std::string expectedOutput = "                                                     \
                 Timer                   Count       CPU Time              Wall Time       \
---------------------------------------- ----- --------------------- --------------------- \
totalTestRuntime                           1        0.001 SKIP             0.100 SKIP      \
                                                                                           \
Took 0.0001 seconds to generate the table above.                                           \
    ";
        EXPECT_TRUE(unitTestUtils::areStringsEqualWithToleranceForNumbers(expectedOutput, outputStream.str(), tolerance));
    }

    stk::diag::deleteRootTimer(rootTimer);
}

TEST(StkDiagTimerHowTo, useChildTimers)
{
    enum {CHILDMASK1 = 1, CHILDMASK2 = 2};
    stk::diag::TimerSet enabledTimerSet(CHILDMASK1 | CHILDMASK2);
    stk::diag::Timer rootTimer = createRootTimer("totalTestRuntime", enabledTimerSet);
    rootTimer.start();

    stk::diag::Timer childTimer1("childTimer1", CHILDMASK1, rootTimer);
    stk::diag::Timer childTimer2("childTimer2", CHILDMASK2, rootTimer);

    {
        stk::diag::TimeBlock timeStuffInThisScope(childTimer1);
        stk::diag::TimeBlock timeStuffInThisScopeAgain(childTimer2);
        doWork();
    }

    std::ostringstream outputStream;
    bool printTimingsOnlySinceLastPrint = false;
    stk::diag::printTimersTable(outputStream, rootTimer, stk::diag::METRICS_ALL, printTimingsOnlySinceLastPrint);

    {
        stk::diag::TimeBlock timeStuffInThisScope(childTimer1);
        doWork();
    }

    stk::diag::printTimersTable(outputStream, rootTimer, stk::diag::METRICS_ALL, printTimingsOnlySinceLastPrint);

    std::string expectedOutput = "                                                         \
                 Timer                   Count       CPU Time              Wall Time       \
---------------------------------------- ----- --------------------- --------------------- \
totalTestRuntime                             1        0.000  SKIP        0.100 SKIP        \
  childTimer1                                1        0.000  SKIP        0.100 SKIP        \
  childTimer2                                1        0.000  SKIP        0.100 SKIP        \
                                                                                           \
Took 0.0001 seconds to generate the table above.                                           \
                 Timer                   Count       CPU Time              Wall Time       \
---------------------------------------- ----- --------------------- --------------------- \
totalTestRuntime                             1        0.000  SKIP        0.200 SKIP        \
  childTimer1                                2        0.000  SKIP        0.200 SKIP        \
  childTimer2                                1        0.000  SKIP        0.100 SKIP        \
                                                                                           \
Took 0.0001 seconds to generate the table above.                                           \
            ";
    EXPECT_TRUE(unitTestUtils::areStringsEqualWithToleranceForNumbers(expectedOutput, outputStream.str(), tolerance));

    stk::diag::deleteRootTimer(rootTimer);
}

TEST(StkDiagTimerHowTo, disableChildTimers)
{
    enum {CHILDMASK1 = 1, CHILDMASK2 = 2};
    stk::diag::TimerSet enabledTimerSet(CHILDMASK2);
    stk::diag::Timer rootTimer = createRootTimer("totalTestRuntime", enabledTimerSet);
    rootTimer.start();

    stk::diag::Timer disabledTimer("disabledTimer", CHILDMASK1, rootTimer);
    stk::diag::Timer enabledTimer("enabledTimer", CHILDMASK2, rootTimer);

    {
        stk::diag::TimeBlock timeStuffInThisScope(disabledTimer);
        stk::diag::TimeBlock timeStuffInThisScopeAgain(enabledTimer);
        doWork();
    }

    std::ostringstream outputStream;
    bool printTimingsOnlySinceLastPrint = false;
    stk::diag::printTimersTable(outputStream, rootTimer, stk::diag::METRICS_ALL, printTimingsOnlySinceLastPrint);

    {
        stk::diag::TimeBlock timeStuffInThisScope(disabledTimer);
        doWork();
    }

    stk::diag::printTimersTable(outputStream, rootTimer, stk::diag::METRICS_ALL, printTimingsOnlySinceLastPrint);

    std::string expectedOutput = "                                                         \
                 Timer                   Count       CPU Time              Wall Time       \
---------------------------------------- ----- --------------------- --------------------- \
totalTestRuntime                             1        0.000  SKIP        0.100 SKIP        \
  enabledTimer                               1        0.000  SKIP        0.100 SKIP        \
                                                                                           \
Took 0.0001 seconds to generate the table above.                                           \
                 Timer                   Count       CPU Time              Wall Time       \
---------------------------------------- ----- --------------------- --------------------- \
totalTestRuntime                             1        0.000  SKIP        0.200 SKIP        \
  enabledTimer                               1        0.000  SKIP        0.100 SKIP        \
                                                                                           \
Took 0.0001 seconds to generate the table above.                                           \
            ";
    EXPECT_TRUE(unitTestUtils::areStringsEqualWithToleranceForNumbers(expectedOutput, outputStream.str(), tolerance));

    stk::diag::deleteRootTimer(rootTimer);
}

}
