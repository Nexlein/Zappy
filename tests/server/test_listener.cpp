#include <gtest/gtest.h>

#include "network/Listener.hpp"

TEST(Listener, ValidPort)
{
    Listener l(14242);
    EXPECT_GE(l.fd(), 0);
}

TEST(Listener, PortAlreadyInUse)
{
    Listener l(14243);
    EXPECT_THROW(Listener(14243), ListenerException);
}

TEST(Listener, InvalidPort)
{
    EXPECT_THROW(Listener(99999), ListenerException);
}
