#pragma once

#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>
#include <cstdint>


using namespace std;

struct Node
{
    string id;
    string ipAddress;
    int weight = 1;
};

struct Request
{
    string id;
    string serviceId;
    string method;
};

static uint64_t fnv1a_64(const string &s);

class ConsistentHashing
{
private:
    int pointMultiplier_;
    map<uint64_t, Node> nodeMappings_;
    unordered_map<string, vector<uint64_t>> nodePositions_;
    unordered_map<string, size_t> nodeLoad_;
    mutex mu_;

    uint64_t makeKey(const Request &request) const;
    void removeNodePoints(const string &nodeId);

public:
    ConsistentHashing() {};
    explicit ConsistentHashing(int pointMultiplier = 100);

    void addNode(const Node &node);
    void removeNode(const Node &node);
    optional<Node> handleRequest(const Request &request);
    void printRingState(ostream &out = cout);
    unordered_map<string, size_t> snapshotLoads();
};