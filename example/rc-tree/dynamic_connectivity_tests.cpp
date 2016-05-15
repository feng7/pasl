#include <random>
#include <vector>
#include <iostream>

#include "dynamic_connectivity.hpp"

using rng_t = std::mt19937;
using uni_t = std::uniform_int_distribution<>;
using std::vector;

bool connected(vector<vector<bool>> const &g, int u, int v, int p) {
    if (u == v) {
        return true;
    }
    for (int i = 0; i < (int) g.size(); ++i) {
        if (i != p && g[u][i] && connected(g, i, v, u)) {
            return true;
        }
    }
    return false;
}

int main() {
    rng_t rng(1);
    for (int step = 0; step < 1000; ++step) {
        int n = (int) uni_t(1, 50)(rng);
        vector<vector<bool>> g(n, vector<bool>(n));
        link_cut_tree tree;
        for (int i = 0; i < n; ++i) {
            tree.create_vertex();
        }
        for (int query = 0; query < 2000; ++query) {
            unsigned cmd = uni_t(0, 9)(rng);
            int u = (int) uni_t(0, n - 1)(rng);
            int v = (int) uni_t(0, n - 1)(rng);

            bool expected = connected(g, u, v, -1);
            bool found = tree.test_connectivity(u, v);
            if (expected != found) {
                std::cout << "Error: Expected " << expected << " found " << found << std::endl;
                return 1;
            }

            if (cmd == 0 && expected) {
                bool exp2 = g[u][v];
                bool fnd2 = tree.test_link(u, v);
                if (exp2 != fnd2) {
                    std::cout << "Error(2): Expected " << exp2 << " found " << fnd2 << std::endl;
                    return 1;
                }
                if (exp2) {
                    tree.cut(u, v);
                    g[u][v] = false;
                    g[v][u] = false;
                }
            } else if (cmd > 1 && !expected) {
                tree.link(u, v);
                g[u][v] = true;
                g[v][u] = true;
            }
        }
    }
    std::cout << "LCT works!" << std::endl;
    return 0;
}
