#include <gtest/gtest.h>
#include <memory>
#include "Database.h"

class ChatDBTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        db = std::make_unique<ChatDB>(":memory:");
    }

    std::unique_ptr<ChatDB> db;
};

TEST_F(ChatDBTest, RegisterAndVerifyPassword)
{
    ASSERT_TRUE(db->addUser("alice", "Alice", "secret123"));
    EXPECT_EQ(db->getUserId("alice"), 1);
    EXPECT_TRUE(db->verifyUser("alice", "secret123"));
    EXPECT_FALSE(db->verifyUser("alice", "wrong-password"));
}

TEST_F(ChatDBTest, RejectDuplicateLogin)
{
    ASSERT_TRUE(db->addUser("bob", "Bob", "password1"));
    EXPECT_FALSE(db->addUser("bob", "Bobby", "password2"));
}

TEST_F(ChatDBTest, PrivateMessageHistory)
{
    ASSERT_TRUE(db->addUser("u1", "User1", "pass111"));
    ASSERT_TRUE(db->addUser("u2", "User2", "pass222"));
    db->addMessage("u1", "u2", "hello");
    db->addMessage("u2", "u1", "hi");

    auto history = db->getMessages("u1", "u2");
    ASSERT_EQ(history.size(), 2u);
    EXPECT_NE(history[0].find("hello"), std::string::npos);
    EXPECT_NE(history[1].find("hi"), std::string::npos);
}

TEST_F(ChatDBTest, PublicMessageHistory)
{
    ASSERT_TRUE(db->addUser("ann", "Ann", "pass333"));
    db->addMessage("ann", "", "broadcast");
    auto history = db->getPublicMessages();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_NE(history[0].find("broadcast"), std::string::npos);
}

TEST_F(ChatDBTest, UnknownUserVerifyFails)
{
    EXPECT_FALSE(db->verifyUser("nosuch", "anything"));
}
