#include "ConsistentHashing.h"

using namespace std;

int main()
{
    ConsistentHashing ring(2); 

    /* struct Node
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
    }; */

    // if I use only a,b,c then it gives b = 1, c = 2 which is not useful (maybe we should change hash function or use more nodes or more multiplier value)
    Node n1{"node-a", "10.0.0.1", 1};
    Node n2{"node-b", "10.0.0.2", 1};
    Node n3{"node-c", "10.0.0.3", 1};

    ring.addNode(n1);
    ring.addNode(n2);
    ring.addNode(n3);

    // show current ring state
    ring.printRingState();

    // Simulation of handleRequest() to show distribution
    cout << "\nSimulating 5 handled requests (varied ids/methods):\n";
    const vector<string> methods = {"GET", "POST", "PUT", "DELETE"};
    for (int i = 0; i < 5; ++i)
    {
        Request r{"req-sim-" + to_string(10000 + i * 7), "svc-1", methods[i % methods.size()]};
        ring.handleRequest(r);
        
    }

    // // show loads after simulation
    auto loads = ring.snapshotLoads();
    cout << "\nLoad distribution after simulation:\n";
    for (const auto &l : loads)
        cout << "  " << l.first << " => " << l.second << "\n";


    // simulation of removal of nOde from ring
    cout << "\nRemoving node-b and reassigning:\n";
    ring.removeNode(n2);
    ring.printRingState();

    // simulate more requests after removal
    for (int i = 0; i < 5; ++i)
    {
        Request r{"req-after-rem-" + to_string(20000 + i * 5), "svc-1", methods[i % methods.size()]};
        ring.handleRequest(r);
    }

    cout << "\nFinal loads after removal & handling:\n";
    for (const auto &l : ring.snapshotLoads())
        cout << "  " << l.first << " => " << l.second << "\n";
    

    return 0;
}