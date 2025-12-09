// #include <iostream>
// #include <string>
// #include <map>
// #include <unordered_map>
// #include <vector>
// #include <mutex>
// #include <optional>
// #include <cstdint>

// using namespace std;

// struct Node {
//     string id;
//     string ipAddress;
//     int weight = 1;
// };

// struct Request {
//     string id;
//     string serviceId;
//     string method;
// };

// // FNV-1a 64-bit hash for stable hashing across platforms/runs
// static uint64_t fnv1a_64(const string &s) {
//     uint64_t hash = 14695981039346656037ULL; // FNV  Prime number / offset basis
//     for (unsigned char c : s) {
//         hash ^= static_cast<uint64_t>(c);
//         hash *= 1099511628211ULL;
//     }
//     return hash;
// }

// class ConsistentHashing {
// private:
//     int pointMultiplier_;
//     map<uint64_t, Node> nodeMappings_; // ordered circular ring
//     unordered_map<string, vector<uint64_t>> nodePositions_; // node id, virtual points
//     mutex mu_;
// public:
//     ConsistentHashing() {};
//     explicit ConsistentHashing(int pointMultiplier = 100)
//         : pointMultiplier_(pointMultiplier) {
//         if (pointMultiplier_ <= 0) throw invalid_argument("pointMultiplier must be > 0");
//     }

//     // add a node with virtual points = pointMultiplier * weight
//     void addNode(const Node &node) {
//         lock_guard<mutex> lg(mu_);
//         auto it_existing = nodePositions_.find(node.id);
//         if (it_existing != nodePositions_.end()) {
//             for (uint64_t p : it_existing->second) nodeMappings_.erase(p);
//             nodePositions_.erase(it_existing);
//         }
//         auto &positions = nodePositions_[node.id];
//         for (int i = 0; i < pointMultiplier_; ++i) {
//             for (int j = 0; j < node.weight; ++j) {
//                 // mix i, j and node id to produce distinct virtual points
//                 string token = to_string(i * pointMultiplier_ + j) + "#" + node.id;
//                 uint64_t point = fnv1a_64(token);
//                 positions.push_back(point);
//                 nodeMappings_[point] = node;
//             }
//         }
//     }

//     // remove node and its virtual points
//     void removeNode(const Node &node) {
//         lock_guard<mutex> lg(mu_);
//         auto it = nodePositions_.find(node.id);
//         if (it == nodePositions_.end()) return;
//         for (uint64_t point : it->second) {
//             nodeMappings_.erase(point);
//         }
//         nodePositions_.erase(it);
//     }

//     // returns optional<Node>; empty if no nodes available
//     optional<Node> getAssignedNode(const Request &request) {
//         lock_guard<mutex> lg(mu_);
//         if (nodeMappings_.empty()) return nullopt;
//         uint64_t key = fnv1a_64(request.id);
//         auto it = nodeMappings_.upper_bound(key);
//         if (it == nodeMappings_.end()) it = nodeMappings_.begin();
//         return it->second;
//     }
// };

// int main() {
//     ConsistentHashing ring(50); // 50 points per unit

//     Node n1{"node-a", "10.0.0.1", 1};
//     Node n2{"node-b", "10.0.0.2", 2};
//     Node n3{"node-c", "10.0.0.3", 1};

//     ring.addNode(n1);
//     ring.addNode(n2);
//     ring.addNode(n3);

//     vector<Request> reqs = {
//         {"req-1001", "svc-1", "GET"},
//         {"req-2002", "svc-1", "POST"},
//         {"req-3003", "svc-1", "GET"},
//         {"req-4004", "svc-1", "PUT"},
//         {"req-5005", "svc-1", "DELETE"}
//     };

//     cout << "Consistent hashing assignments:\n";
//     for (const auto &r : reqs) {
//         auto assigned = ring.getAssignedNode(r);
//         if (assigned) {
//             cout << r.id << " -> " << assigned->id << " (" << assigned->ipAddress << ")\n";
//         } else {
//             cout << r.id << " -> (no node available)\n";
//         }
//     }

//     // removal
//     cout << "\nRemoving node-b and reassigning:\n";
//     ring.removeNode(n2);
//     for (const auto &r : reqs) {
//         auto assigned = ring.getAssignedNode(r);
//         if (assigned) cout << r.id << " -> " << assigned->id << " (" << assigned->ipAddress << ")\n";
//         else cout << r.id << " -> (no node available)\n";
//     }

//     return 0;
// }

// ...existing code...
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

// FNV-1a 64-bit hash for stable hashing across platforms/runs
static uint64_t fnv1a_64(const string &s)
{
    uint64_t hash = 14695981039346656037ULL; // FNV  Prime number / offset basis
    for (unsigned char c : s)
    {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

class ConsistentHashing
{
private:
    int pointMultiplier_;
    map<uint64_t, Node> nodeMappings_;                      // ordered circular ring
    unordered_map<string, vector<uint64_t>> nodePositions_; // node id, virtual points
    unordered_map<string, size_t> nodeLoad_;                // simple per-node request count
    mutex mu_;

    // composite hash of id|serviceId|method for better spread
    uint64_t makeKey(const Request &request) const
    {
        const string composite = request.id + "|" + request.serviceId + "|" + request.method;
        return fnv1a_64(composite);
    }

public:
    ConsistentHashing() {};
    explicit ConsistentHashing(int pointMultiplier = 100)
        : pointMultiplier_(pointMultiplier)
    {
        if (pointMultiplier_ <= 0)
            throw invalid_argument("pointMultiplier must be > 0");
    }

    // add a node with virtual points = pointMultiplier * weight
    void addNode(const Node &node)
    {
        lock_guard<mutex> lg(mu_);
        auto it_existing = nodePositions_.find(node.id);
        if (it_existing != nodePositions_.end())
        {
            for (uint64_t p : it_existing->second)
                nodeMappings_.erase(p);
            nodePositions_.erase(it_existing);
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
    void removeNode(const Node &node)
    {
        lock_guard<mutex> lg(mu_);
        auto it = nodePositions_.find(node.id);
        if (it == nodePositions_.end())
            return;
        for (uint64_t point : it->second)
        {
            nodeMappings_.erase(point);
        }
        nodePositions_.erase(it);
        // keep load counter (or erase if you prefer)
        // nodeLoad_.erase(node.id);
    }

    // returns optional<Node>; empty if no nodes available
    optional<Node> getAssignedNode(const Request &request)
    {
        lock_guard<mutex> lg(mu_);
        if (nodeMappings_.empty())
            return nullopt;
        uint64_t key = makeKey(request);
        auto it = nodeMappings_.upper_bound(key);
        if (it == nodeMappings_.end())
            it = nodeMappings_.begin();
        return it->second;
    }

    // handleRequest: assigns and records load for the chosen node
    optional<Node> handleRequest(const Request &request)
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

        return assigned;
    }

    // print the current state of the ring and per-node stats
    void printRingState(ostream &out = cout)
    {
        lock_guard<mutex> lg(mu_);
        out << "=== Ring state ===\n";
        out << "Total virtual points: " << nodeMappings_.size() << "\n";
        out << "Nodes (id -> virtual points):\n";
        for (const auto &kv : nodePositions_)
        {
            out << "  " << kv.first << " -> " << kv.second.size() << " points\n";
        }
        out << "Ring map (point -> node) [first 60 entries]:\n";
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
    unordered_map<string, size_t> snapshotLoads()
    {
        lock_guard<mutex> lg(mu_);
        return nodeLoad_;
    }
};

int main()
{
    ConsistentHashing ring(10); // 10 points per unit (still small)

    Node n1{"node-a", "10.0.0.1", 1};
    Node n2{"node-b", "10.0.0.2", 2};
    Node n3{"node-c", "10.0.0.3", 1};

    ring.addNode(n1);
    ring.addNode(n2);
    ring.addNode(n3);

    // show current ring state
    ring.printRingState();

    // list of sample requests
    vector<Request> reqs = {
        {"req-1001", "svc-1", "GET"},
        {"req-2002", "svc-1", "POST"},
        {"req-3003", "svc-1", "GET"},
        {"req-4004", "svc-1", "PUT"},
        {"req-5005", "svc-1", "DELETE"}};

    cout << "Consistent hashing assignments (single lookup):\n";
    for (const auto &r : reqs)
    {
        auto assigned = ring.getAssignedNode(r);
        if (assigned)
        {
            cout << r.id << " -> " << assigned->id << " (" << assigned->ipAddress << ")\n";
        }
        else
        {
            cout << r.id << " -> (no node available)\n";
        }
    }

    // Simulate handling multiple requests to show distribution
    cout << "\nSimulating 60 handled requests (varied ids/methods):\n";
    const vector<string> methods = {"GET", "POST", "PUT", "DELETE"};
    for (int i = 0; i < 60; ++i)
    {
        Request r{"req-sim-" + to_string(10000 + i * 7), "svc-1", methods[i % methods.size()]};
        auto assigned = ring.handleRequest(r);
        if (assigned && (i < 8))
        { // print first few assignments
            cout << r.id << " (" << r.method << ") -> " << assigned->id << " (" << assigned->ipAddress << ")\n";
        }
    }

    // show loads after simulation
    auto loads = ring.snapshotLoads();
    cout << "\nLoad distribution after simulation:\n";
    for (const auto &l : loads)
    {
        cout << "  " << l.first << " => " << l.second << "\n";
    }

    // removal
    cout << "\nRemoving node-b and reassigning:\n";
    ring.removeNode(n2);
    ring.printRingState();

    // simulate more requests after removal
    for (int i = 0; i < 20; ++i)
    {
        Request r{"req-after-rem-" + to_string(20000 + i * 5), "svc-1", methods[i % methods.size()]};
        ring.handleRequest(r);
    }

    cout << "\nFinal loads after removal & handling:\n";
    for (const auto &l : ring.snapshotLoads())
    {
        cout << "  " << l.first << " => " << l.second << "\n";
    }

    return 0;
}