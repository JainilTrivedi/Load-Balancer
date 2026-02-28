#include "ConsistentHashing.h"
#include<gtest/gtest.h>

#include<string>
#include<unordered_map>


using namespace std;

class ConsistentHashingTest : public ::testing::Test
{
protected:
    ConsistentHashing ring{2};

    Node n1{"node-a", "10.0.0.1", 1};
    Node n2{"node-b", "10.0.0.2", 1};
    Node n3{"node-c", "10.0.0.3", 1};

    Request makeRequest(const string &id)
    {
        return {id, "svc-1", "GET"};
    }
};


TEST_F(ConsistentHashingTest, AddNode)
{
    ring.addNode(n1);

    auto loads = ring.snapshotLoads();

    ASSERT_TRUE(loads.count("node-a"));
    EXPECT_EQ(loads["node-a"], 0);
    // since 0 req have been made, load should be 0
}

TEST_F(ConsistentHashingTest, RemoveNode)
{
    ring.addNode(n1);
    ring.addNode(n2);
    ring.addNode(n3);
    ring.removeNode(n2);

    for (int i = 0; i < 5; ++i)
    {
        auto assigned = ring.handleRequest(makeRequest("req-" + to_string(i * 7)));
        ASSERT_TRUE(assigned.has_value());
        EXPECT_NE(assigned->id, "node-b");
    }
}


TEST_F(ConsistentHashingTest, HandleRequest)
{
    ring.addNode(n1);
    ring.addNode(n2);
    ring.addNode(n3);

    Request r = makeRequest("req-999");
    auto first  = ring.handleRequest(r);
    auto second = ring.handleRequest(r);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->id, second->id);
}

TEST_F(ConsistentHashingTest, LoadDistribution)
{
    ring.addNode(n1);
    ring.addNode(n2);
    ring.addNode(n3);

    int total = 5;
    for (int i = 0; i < total; ++i)
        ring.handleRequest(makeRequest("req-dist-" + to_string(i * 3)));

    auto loads = ring.snapshotLoads();
    size_t handled = 0;
    for (const auto &l : loads)
        handled += l.second;

    EXPECT_EQ(handled, total);
}


// g++ ConsistentHashingTest.cpp ConsistentHashing.c++ -lgtest -lgtest_main -pthread -o tests