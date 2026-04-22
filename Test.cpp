#include <iostream>
#include <vector>
#include <queue>
#include <limits>

using namespace std;

class Graph {
private:
    int V;
    vector<vector<pair<int, int>>> adj; // (neighbor, weight)

public:
    Graph(int V) {
        this->V = V;
        adj.resize(V);
    }

    void addEdge(int u, int v, int w) {
        adj[u].push_back({v, w});
    }

    void dijkstra(int src) {
        vector<int> dist(V, numeric_limits<int>::max());

        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;

        dist[src] = 0;
        pq.push({0, src});

        while (!pq.empty()) {
            int d = pq.top().first;
            int u = pq.top().second;
            pq.pop();

            for (auto &edge : adj[u]) {
                int v = edge.first;
                int weight = edge.second;

                if (dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                    pq.push({dist[v], v});
                }
            }
        }

        // print result
        cout << "Dijkstra distances from node " << src << ":\n";
        for (int i = 0; i < V; i++) {
            cout << "Node " << i << " -> " << dist[i] << endl;
        }
    }

    void bellmanFord(int src) {
    vector<int> dist(V, numeric_limits<int>::max());
    dist[src] = 0;

    // edge list oluştur
    vector<tuple<int, int, int>> edges;
    for (int u = 0; u < V; u++) {
        for (auto &p : adj[u]) {
            edges.push_back({u, p.first, p.second});
        }
    }

    // V-1 kez relax
    for (int i = 0; i < V - 1; i++) {
        for (auto &e : edges) {
            int u, v, w;
            tie(u, v, w) = e;

            if (dist[u] != numeric_limits<int>::max() && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
            }
        }
    }

    // negative cycle kontrol (opsiyonel ama iyi puan)
    for (auto &e : edges) {
        int u, v, w;
        tie(u, v, w) = e;

        if (dist[u] != numeric_limits<int>::max() && dist[u] + w < dist[v]) {
            cout << "Negative cycle detected!\n";
            return;
        }
    }

    // print
    cout << "\nBellman-Ford distances from node " << src << ":\n";
    for (int i = 0; i < V; i++) {
        cout << "Node " << i << " -> " << dist[i] << endl;
    }
}
};

int main() {
    Graph g(5);

    // test graph
    g.addEdge(0, 1, 2);
    g.addEdge(0, 2, 4);
    g.addEdge(1, 2, 1);
    g.addEdge(1, 3, 7);
    g.addEdge(2, 4, 3);

    g.dijkstra(0);

    g.bellmanFord(0);

    return 0;
}