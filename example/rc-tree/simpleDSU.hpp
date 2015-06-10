#ifndef __PASL_SIMPLE_DSU_HPP__
#define __PASL_SIMPLE_DSU_HPP__

#include <vector>

class simple_dsu {
private:
	mutable std::vector<int> parent;
	std::vector<int> rank;
public:
	void add_vertex();
	int size() const;
	int get(int v) const;
	void unite(int v1, int v2);
};

#endif
