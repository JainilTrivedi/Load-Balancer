#include "ConsistentHashing.h"


#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>
#include <cstdint>

using namespace std;


// FNV-1a 64-bit hash  = big prime number ^ (ints, chars of request) * big prime number
//int had issues on SOL
static uint64_t fnv1a_64(const string &s)
{
    uint64_t hash = 14695981039346656037ULL; // FNV  Prime number / offset basis
    for (unsigned char c : s)
    {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return (hash%20)+1; 
    // for better demonstration
}


// composite hash of id|serviceId|method for better spread
uint64_t ConsistentHashing::makeKey(const Request &request) const
{
    const string composite_key = request.id + "|" + request.serviceId + "|" + request.method;
    return fnv1a_64(composite_key);
}

void ConsistentHashing::removeNodePoints(const string &nodeId)
{
    auto it = nodePositions_.find(nodeId);
    if (it == nodePositions_.end()) return;
    for (uint64_t point : it->second)
        nodeMappings_.erase(point);
    nodePositions_.erase(it);
}


ConsistentHashing::ConsistentHashing(int pointMultiplier)
    : pointMultiplier_(pointMultiplier)
{
    if (pointMultiplier_ <= 0)
        throw invalid_argument("pointMultiplier must be > 0");
}

// add a node with virtual points = pointMultiplier * weight
void ConsistentHashing::addNode(const Node &node)
{
    lock_guard<mutex> lg(mu_);
    auto it_existing = nodePositions_.find(node.id);
    if (it_existing != nodePositions_.end())
    {
        removeNodePoints(node.id);
    }

    auto &positions = nodePositions_[node.id];
    for (int i = 0; i < pointMultiplier_; ++i)
    {
        for (int j = 0; j < node.weight; ++j)
        {
            // mix i, j and node id to produce distinct virtual points
            string token = to_string(i * pointMultiplier_ + j) + "#" + node.id;
            uint64_t point = fnv1a_64(token);
            positions.push_back(point);
            nodeMappings_[point] = node;
        }
    }
    // ensure load counter exists
    nodeLoad_.try_emplace(node.id, 0);
}

// remove node and its virtual points
void ConsistentHashing::removeNode(const Node &node)
{
    lock_guard<mutex> lg(mu_);
    removeNodePoints(node.id);
}


// handleRequest: assigns and records load for the chosen node
optional<Node> ConsistentHashing::handleRequest(const Request &request)
{
    lock_guard<mutex> lg(mu_);
    if (nodeMappings_.empty())
        return nullopt;
    uint64_t key = makeKey(request);
    auto it = nodeMappings_.upper_bound(key);
    if (it == nodeMappings_.end())
        it = nodeMappings_.begin();
    const Node &assigned = it->second;
    nodeLoad_[assigned.id] += 1;
    // in real use case actually forward the request to assigned.ipAddress
    cout << "Request " << request.id << " -> hashed to position: " << key  << " -> nearest virtual point: " << it->first  << " (" << assigned.id << ")" << "\n";
    return assigned;
}

// print the current state of the ring and per-node stats
void ConsistentHashing::printRingState(ostream &out)
{
    lock_guard<mutex> lg(mu_);
    out << "=== Ring state ===\n";
    out << "Total virtual points: " << nodeMappings_.size() << "\n";
    out << "Nodes (id -> virtual points):\n";
    for (const auto &kv : nodePositions_)
    {
        out << "  " << kv.first << " -> " << kv.second.size() << " points\n";
    }
    out << "Ring map (point -> node):\n";
    size_t shown = 0;
    for (const auto &p : nodeMappings_)
    {
        out << "  " << p.first << " -> " << p.second.id << "\n";
        if (++shown >= 60)
            break;
    }
    out << "Per-node handled request counts:\n";
    for (const auto &nl : nodeLoad_)
    {
        out << "  " << nl.first << " => " << nl.second << "\n";
    }
    out << "===================\n";
}

// snapshot of node loads (thread-safe copy)
unordered_map<string, size_t> ConsistentHashing::snapshotLoads()
{
    lock_guard<mutex> lg(mu_);
    return nodeLoad_;
}