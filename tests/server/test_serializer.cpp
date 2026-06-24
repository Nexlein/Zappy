#include <gtest/gtest.h>

#include "protocol/Serializer.hpp"

TEST(Serializer, Msz) { EXPECT_EQ(Serializer::msz(10, 20), "msz 10 20\n"); }

TEST(Serializer, Bct)
{
    Resources r;
    r.food = 1;
    r.linemate = 2;
    EXPECT_EQ(Serializer::bct(3, 5, r), "bct 3 5 1 2 0 0 0 0 0\n");
}

TEST(Serializer, Ppo) { EXPECT_EQ(Serializer::ppo(7, 4, 2, Orientation::N), "ppo #7 4 2 1\n"); }

TEST(Serializer, MszZero) { EXPECT_EQ(Serializer::msz(0, 0), "msz 0 0\n"); }

TEST(Serializer, BctAllZero)
{
    Resources r;
    EXPECT_EQ(Serializer::bct(0, 0, r), "bct 0 0 0 0 0 0 0 0 0\n");
}

TEST(Serializer, BctAllFilled)
{
    Resources r;
    r.food = 1;
    r.linemate = 2;
    r.deraumere = 3;
    r.sibur = 4;
    r.mendiane = 5;
    r.phiras = 6;
    r.thystame = 7;
    EXPECT_EQ(Serializer::bct(1, 2, r), "bct 1 2 1 2 3 4 5 6 7\n");
}

TEST(Serializer, Tna) { EXPECT_EQ(Serializer::tna("TeamA"), "tna TeamA\n"); }

TEST(Serializer, Pnw)
{
    EXPECT_EQ(Serializer::pnw(3, 1, 2, Orientation::E, 1, "TeamA"), "pnw #3 1 2 2 1 TeamA\n");
}

TEST(Serializer, PpoAllOrientations)
{
    EXPECT_EQ(Serializer::ppo(0, 0, 0, Orientation::N), "ppo #0 0 0 1\n");
    EXPECT_EQ(Serializer::ppo(0, 0, 0, Orientation::E), "ppo #0 0 0 2\n");
    EXPECT_EQ(Serializer::ppo(0, 0, 0, Orientation::S), "ppo #0 0 0 3\n");
    EXPECT_EQ(Serializer::ppo(0, 0, 0, Orientation::W), "ppo #0 0 0 4\n");
}

TEST(Serializer, Plv) { EXPECT_EQ(Serializer::plv(5, 3), "plv #5 3\n"); }

TEST(Serializer, Pin)
{
    Resources r;
    r.food = 10;
    EXPECT_EQ(Serializer::pin(2, 3, 4, r), "pin #2 3 4 10 0 0 0 0 0 0\n");
}

TEST(Serializer, Pex) { EXPECT_EQ(Serializer::pex(1), "pex #1\n"); }
TEST(Serializer, Pbc) { EXPECT_EQ(Serializer::pbc(2, "hello world"), "pbc #2 hello world\n"); }
TEST(Serializer, Pfk) { EXPECT_EQ(Serializer::pfk(4), "pfk #4\n"); }
TEST(Serializer, Pdi) { EXPECT_EQ(Serializer::pdi(9), "pdi #9\n"); }

TEST(Serializer, PdrPgt)
{
    EXPECT_EQ(Serializer::pdr(1, 0), "pdr #1 0\n");
    EXPECT_EQ(Serializer::pgt(1, 6), "pgt #1 6\n");
}

TEST(Serializer, PicSinglePlayer) { EXPECT_EQ(Serializer::pic(2, 3, 1, {5}), "pic 2 3 1 #5\n"); }

TEST(Serializer, PicMultiplePlayers)
{
    EXPECT_EQ(Serializer::pic(0, 0, 2, {1, 2, 3}), "pic 0 0 2 #1 #2 #3\n");
}

TEST(Serializer, PieSuccess) { EXPECT_EQ(Serializer::pie(1, 2, true), "pie 1 2 1\n"); }
TEST(Serializer, PieFail) { EXPECT_EQ(Serializer::pie(1, 2, false), "pie 1 2 0\n"); }

TEST(Serializer, Enw) { EXPECT_EQ(Serializer::enw(0, 3, 5, 7), "enw #0 #3 5 7\n"); }
TEST(Serializer, Ebo) { EXPECT_EQ(Serializer::ebo(2), "ebo #2\n"); }
TEST(Serializer, Edi) { EXPECT_EQ(Serializer::edi(2), "edi #2\n"); }

TEST(Serializer, Sgt) { EXPECT_EQ(Serializer::sgt(100), "sgt 100\n"); }
TEST(Serializer, Sst) { EXPECT_EQ(Serializer::sst(200), "sst 200\n"); }
TEST(Serializer, Stu) { EXPECT_EQ(Serializer::stu(720, 7200), "stu 720 7200\n"); }
TEST(Serializer, Gtt) { EXPECT_EQ(Serializer::gtt("TeamA", 600, 6000), "gtt TeamA 600 6000\n"); }
TEST(Serializer, GttNeverJoined)
{
    EXPECT_EQ(Serializer::gtt("Ghost", -1, -1), "gtt Ghost -1 -1\n");
}

TEST(Serializer, Seg) { EXPECT_EQ(Serializer::seg("TeamA"), "seg TeamA\n"); }
TEST(Serializer, Smg) { EXPECT_EQ(Serializer::smg("hi"), "smg hi\n"); }
TEST(Serializer, Suc) { EXPECT_EQ(Serializer::suc(), "suc\n"); }
TEST(Serializer, Sbp) { EXPECT_EQ(Serializer::sbp(), "sbp\n"); }