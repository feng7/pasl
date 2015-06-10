#include "simpleDSU.hpp"

void simple_dsu::add_vertex() {
	rank.push_back(0);
	parent.push_back(parent.size());
}

int simple_dsu::size() const {
	return rank.size();
}

int simple_dsu::get(int v) const {
	if (parent[v] == v) {
		return v;
	} else {
		return parent[v] = get(parent[v]);
	}
}

void simple_dsu::unite(int v1, int v2) {
	v1 = get(v1);
	v2 = get(v2);
	if (v1 != v2) {
		if (rank[v1] == rank[v2]) {
			++rank[v1];
		}
		if (rank[v1] < rank[v2]) {
			parent[v1] = v2;
		} else {
			parent[v2] = v1;
		}
	}
}
