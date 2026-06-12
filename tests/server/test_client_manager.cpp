#include <gtest/gtest.h>

#include "network/ClientManager.hpp"
#include "network/Listener.hpp"

TEST(ClientManager, GetConnectionThrowsOnUnknownId)
{
    Listener l(14250);
    ClientManager cm(l);
    EXPECT_THROW(cm.getConnection(99), std::out_of_range);
}

TEST(ClientManager, DisconnectUnknownIdIsNoOp)
{
    Listener l(14251);
    ClientManager cm(l);
    EXPECT_NO_THROW(cm.disconnect(99));
}

TEST(ClientManager, SendUnknownIdIsNoOp)
{
    Listener l(14252);
    ClientManager cm(l);
    EXPECT_NO_THROW(cm.send(99, "hello\n"));
}
