#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>
#include <cstdint>

using namespace std;

struct Node {
    string id;
    string ipAddress;
    int weight = 1;
};

struct Request {
    string id;
    string serviceId;
    string method;
};

// FNV-1a 64-bit hash for stable hashing across platforms/runs
static uint64_t fnv1a_64(const string &s) {
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : s) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

class ConsistentHashing {
private:
    int pointMultiplier_;
    map<uint64_t, Node> nodeMappings_; // ordered circular ring
    unordered_map<string, vector<uint64_t>> nodePositions_; // node id, virtual points
    mutex mu_;
public:
    ConsistentHashing() {};
    explicit ConsistentHashing(int pointMultiplier = 100)
        : pointMultiplier_(pointMultiplier) {
        if (pointMultiplier_ <= 0) throw invalid_argument("pointMultiplier must be > 0");
    }

    // add a node with virtual points = pointMultiplier * weight
    void addNode(const Node &node) {
        lock_guard<mutex> lg(mu_);
        auto it_existing = nodePositions_.find(node.id);
        if (it_existing != nodePositions_.end()) {
            for (uint64_t p : it_existing->second) nodeMappings_.erase(p);
            nodePositions_.erase(it_existing);
        }
        auto &positions = nodePositions_[node.id]; 
        for (int i = 0; i < pointMultiplier_; ++i) {
            for (int j = 0; j < node.weight; ++j) {
                // mix i, j and node id to produce distinct virtual points
                string token = to_string(i * pointMultiplier_ + j) + "#" + node.id;
                uint64_t point = fnv1a_64(token);
                positions.push_back(point);
                nodeMappings_[point] = node;
            }
        }
    }

    // remove node and its virtual points
    void removeNode(const Node &node) {
        lock_guard<mutex> lg(mu_);
        auto it = nodePositions_.find(node.id);
        if (it == nodePositions_.end()) return;
        for (uint64_t point : it->second) {
            nodeMappings_.erase(point);
        }
        nodePositions_.erase(it);
    }

    // returns optional<Node>; empty if no nodes available
    optional<Node> getAssignedNode(const Request &request) {
        lock_guard<mutex> lg(mu_);
        if (nodeMappings_.empty()) return nullopt;
        uint64_t key = fnv1a_64(request.id);
        auto it = nodeMappings_.upper_bound(key);
        if (it == nodeMappings_.end()) it = nodeMappings_.begin();
        return it->second;
    }
};


int main() {
    ConsistentHashing ring(50); // 50 points per unit

    Node n1{"node-a", "10.0.0.1", 1};
    Node n2{"node-b", "10.0.0.2", 2};
    Node n3{"node-c", "10.0.0.3", 1};

    ring.addNode(n1);
    ring.addNode(n2);
    ring.addNode(n3);

    vector<Request> reqs = {
        {"req-1001", "svc-1", "GET"},
        {"req-2002", "svc-1", "POST"},
        {"req-3003", "svc-1", "GET"},
        {"req-4004", "svc-1", "PUT"},
        {"req-5005", "svc-1", "DELETE"}
    };

    cout << "Consistent hashing assignments:\n";
    for (const auto &r : reqs) {
        auto assigned = ring.getAssignedNode(r);
        if (assigned) {
            cout << r.id << " -> " << assigned->id << " (" << assigned->ipAddress << ")\n";
        } else {
            cout << r.id << " -> (no node available)\n";
        }
    }

    // removal
    cout << "\nRemoving node-b and reassigning:\n";
    ring.removeNode(n2);
    for (const auto &r : reqs) {
        auto assigned = ring.getAssignedNode(r);
        if (assigned) cout << r.id << " -> " << assigned->id << " (" << assigned->ipAddress << ")\n";
        else cout << r.id << " -> (no node available)\n";
    }

    return 0;
}