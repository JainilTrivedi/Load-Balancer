# ConsistentHashing (C++ single-file demo)

Single-file C++ implementation of a consistent-hashing based load balancer (demo).  
Implements virtual nodes, weighted nodes, deterministic 64-bit FNV-1a hashing, and thread-safe add/remove/lookup operations.

## Files
- `ConsistantHashing.c++` — self-contained program with:
  - Node and Request structs
  - `ConsistentHashing` class (addNode, removeNode, getAssignedNode)
  - demo `main()` showing add/remove and lookups

## Features
- Deterministic 64-bit FNV-1a hashing for stable mapping
- Virtual nodes: `pointMultiplier * node.weight` virtual points per node
- Weighted distribution (node.weight)
- Thread-safety via `std::mutex` (coarse-grained)
- Lookup returns `std::optional<Node>` (empty if ring is empty)
- Unit Tests to make sure functions work as expected! 

## Build (Windows / WSL / Linux)
Requirements: g++ with C++17 support.

From repository folder containing `ConsistantHashing.c++`:

Windows (MinGW/MSYS/Git Bash / PowerShell):
```bash
g++ main.c++ ConsistentHashing.c++
# then
.\a.out
```

To run  tests:
```bash
g++ ConsistentHashingTest.cpp ConsistentHashing.c++ -lgtest -lgtest_main -pthread -o tests
# then
./tests
```
