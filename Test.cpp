#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

const long long INF = numeric_limits<long long>::max() / 4;

struct Edge {
    int to;
    int weight;
};

struct FlatEdge {
    int from;
    int to;
    int weight;
};

struct Result {
    string algorithm;
    int vertices = 0;
    int edges = 0;
    double runtimeMs = 0.0;
    size_t graphMemoryBytes = 0;
    size_t extraMemoryBytes = 0;
    size_t totalMemoryBytes = 0;
    string note;
};

class Graph {
public:
    Graph() : vertexCount_(0) {
    }

    explicit Graph(int vertexCount) : vertexCount_(vertexCount), adjacency(vertexCount) {
    }

    static Graph loadDIMACS(const string& fileName) {
        ifstream file(fileName);
        if (!file) {
            throw runtime_error("Could not open file: " + fileName);
        }

        Graph graph;
        string line;

        while (getline(file, line)) {
            if (line.empty()) {
                continue;
            }

            istringstream iss(line);
            char type;
            iss >> type;

            if (type == 'c') {
                continue;
            }

            if (type == 'p') {
                string sp;
                int vertices;
                int expectedEdges;
                iss >> sp >> vertices >> expectedEdges;
                graph = Graph(vertices);
                graph.edges.reserve(expectedEdges);
            } else if (type == 'a') {
                int from;
                int to;
                int weight;
                iss >> from >> to >> weight;
                graph.addEdge(from - 1, to - 1, weight);
            }
        }

        return graph;
    }

    void addEdge(int from, int to, int weight) {
        adjacency[from].push_back({to, weight});
        edges.push_back({from, to, weight});
    }

    Graph subgraph(int limit) const {
        if (limit > vertexCount_) {
            limit = vertexCount_;
        }

        Graph smaller(limit);
        for (int from = 0; from < limit; ++from) {
            for (const Edge& edge : adjacency[from]) {
                if (edge.to < limit) {
                    smaller.addEdge(from, edge.to, edge.weight);
                }
            }
        }
        return smaller;
    }

    int vertices() const {
        return vertexCount_;
    }

    int edgeCount() const {
        return static_cast<int>(edges.size());
    }

    const vector<vector<Edge>>& getAdjacency() const {
        return adjacency;
    }

    const vector<FlatEdge>& getEdges() const {
        return edges;
    }

    size_t estimateGraphMemory() const {
        return adjacency.size() * sizeof(vector<Edge>) +
               edges.size() * sizeof(FlatEdge) +
               edges.size() * sizeof(Edge);
    }

private:
    int vertexCount_;
    vector<vector<Edge>> adjacency;
    vector<FlatEdge> edges;
};

vector<int> parseSizes(const string& text) {
    vector<int> sizes;
    string token;
    stringstream ss(text);

    while (getline(ss, token, ',')) {
        if (!token.empty()) {
            sizes.push_back(stoi(token));
        }
    }

    return sizes;
}

Result runDijkstra(const Graph& graph, int source) {
    vector<long long> distance(graph.vertices(), INF);
    priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<pair<long long, int>>> pq;

    auto start = chrono::high_resolution_clock::now();

    distance[source] = 0;
    pq.push({0, source});

    while (!pq.empty()) {
        long long currentDistance = pq.top().first;
        int node = pq.top().second;
        pq.pop();

        if (currentDistance != distance[node]) {
            continue;
        }

        for (const Edge& edge : graph.getAdjacency()[node]) {
            if (distance[node] + edge.weight < distance[edge.to]) {
                distance[edge.to] = distance[node] + edge.weight;
                pq.push({distance[edge.to], edge.to});
            }
        }
    }

    auto finish = chrono::high_resolution_clock::now();
    double runtime = chrono::duration<double, milli>(finish - start).count();

    Result result;
    result.algorithm = "Dijkstra";
    result.vertices = graph.vertices();
    result.edges = graph.edgeCount();
    result.runtimeMs = runtime;
    result.graphMemoryBytes = graph.estimateGraphMemory();
    result.extraMemoryBytes = distance.size() * sizeof(long long) +
                              graph.edgeCount() * sizeof(pair<long long, int>);
    result.totalMemoryBytes = result.graphMemoryBytes + result.extraMemoryBytes;
    return result;
}

Result runBellmanFord(const Graph& graph, int source) {
    vector<long long> distance(graph.vertices(), INF);

    auto start = chrono::high_resolution_clock::now();

    distance[source] = 0;

    for (int i = 0; i < graph.vertices() - 1; ++i) {
        bool updated = false;

        for (const FlatEdge& edge : graph.getEdges()) {
            if (distance[edge.from] == INF) {
                continue;
            }

            if (distance[edge.from] + edge.weight < distance[edge.to]) {
                distance[edge.to] = distance[edge.from] + edge.weight;
                updated = true;
            }
        }

        if (!updated) {
            break;
        }
    }

    bool negativeCycle = false;
    for (const FlatEdge& edge : graph.getEdges()) {
        if (distance[edge.from] != INF &&
            distance[edge.from] + edge.weight < distance[edge.to]) {
            negativeCycle = true;
            break;
        }
    }

    auto finish = chrono::high_resolution_clock::now();
    double runtime = chrono::duration<double, milli>(finish - start).count();

    Result result;
    result.algorithm = "Bellman-Ford";
    result.vertices = graph.vertices();
    result.edges = graph.edgeCount();
    result.runtimeMs = runtime;
    result.graphMemoryBytes = graph.estimateGraphMemory();
    result.extraMemoryBytes = distance.size() * sizeof(long long);
    result.totalMemoryBytes = result.graphMemoryBytes + result.extraMemoryBytes;
    if (negativeCycle) {
        result.note = "negative cycle detected";
    }
    return result;
}

void writeCSV(const string& fileName, const vector<Result>& results) {
    filesystem::path outputPath(fileName);
    if (outputPath.has_parent_path()) {
        filesystem::create_directories(outputPath.parent_path());
    }

    ofstream file(fileName);
    file << "algorithm,vertices,edges,runtime_ms,graph_memory_bytes,extra_memory_bytes,total_memory_bytes,note\n";

    for (const Result& result : results) {
        file << result.algorithm << ','
             << result.vertices << ','
             << result.edges << ','
             << result.runtimeMs << ','
             << result.graphMemoryBytes << ','
             << result.extraMemoryBytes << ','
             << result.totalMemoryBytes << ','
             << result.note << '\n';
    }
}

void printResults(const vector<Result>& results) {
    cout << "\nResults\n";
    cout << "-----------------------------------------------\n";

    for (const Result& result : results) {
        cout << result.algorithm
             << " | V = " << result.vertices
             << " | E = " << result.edges
             << " | runtime = " << result.runtimeMs << " ms"
             << " | total memory = " << result.totalMemoryBytes << " bytes";

        if (!result.note.empty()) {
            cout << " | " << result.note;
        }

        cout << '\n';
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: ./sssp_benchmark <input.gr> <output.csv> <source> <sizes>\n";
        cerr << "Example: ./sssp_benchmark data/sample_dimacs.gr results/sample.csv 1 3,5\n";
        return 1;
    }

    try {
        string inputFile = argv[1];
        string outputFile = argv[2];
        int source = stoi(argv[3]) - 1;
        vector<int> sizes = parseSizes(argv[4]);

        Graph fullGraph = Graph::loadDIMACS(inputFile);
        vector<Result> results;

        for (int size : sizes) {
            Graph currentGraph = fullGraph.subgraph(size);

            if (source < 0 || source >= currentGraph.vertices()) {
                throw runtime_error("Source vertex is outside one of the requested graph sizes.");
            }

            results.push_back(runDijkstra(currentGraph, source));
            results.push_back(runBellmanFord(currentGraph, source));
        }

        writeCSV(outputFile, results);
        printResults(results);

        cout << "\nCSV file written to " << outputFile << '\n';
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
