#ifndef __PASL_RC_TREE_HPP__
#define __PASL_RC_TREE_HPP__

#include <functional>
#include <vector>

#include "rcForestBuilder.hpp"

using std::fill;
using std::function;
using std::invalid_argument;
using std::pair;
using std::vector;

/**
 * An rc_forest class
 * which supports querying aggregates
 * of edge and vertex data.
 * This is a base class for all implementations.
 */
template<typename VData, typename EData>
class rc_forest {
public:
	size_t	vertex_count() const;
	bool	has_edge(size_t v1, size_t v2) const;
	bool 	has_path(size_t v1, size_t v2) const;
	EData 	edge_data(size_t v1, size_t v2) const;
	EData	aggregate_edge_data(size_t v1, size_t v2) const;
	VData	vertex_data(size_t v) const;
	VData 	aggregate_vertex_data(size_t v1, size_t v2) const;
};

template<typename VData, typename EData>
class naive_rc_forest : rc_forest<VData, EData> {
public:
	naive_rc_forest(rc_forest_builder<VData, EData> const &builder) :
		vertices(builder.get_vertices()),
		edges(builder.get_edges()),
		used(builder.get_vertices().size(), false) {
	}
	size_t vertex_count() const {
		return vertices.size();
	}
	bool has_edge(size_t v1, size_t v2) const {
		return get_edge(v1, v2) != nullptr;
	}
	bool has_path(size_t v1, size_t v2) const {
		check_vertex_index(v1);
		check_vertex_index(v2);
		std::fill(used.begin(), used.end(), false);
		return has_path_dfs(v1, v2);
	}
	EData const &edge_data(size_t v1, size_t v2) const {
		EData const *ptr = get_edge(v1, v2);
		if (!ptr) {
			throw invalid_argument("no such edge, please check has_edge before calling this method");
		}
		return *ptr;
	}
	EData const &aggregate_edge_data(size_t v1, size_t v2) const {
		check_vertex_index(v1);
		check_vertex_index(v2);
		EData *rec = aggregate_edge_data_dfs(v1, v2, nullptr);
		if (rec == nullptr) {
			throw invalid_argument("aggregating edge data over empty path produces empty data which is not supported");
		}
		EData rv = *rec;
		delete rec;
		return rv;
	}
	VData const &vertex_data(size_t v) const {
		check_vertex_index(v);
		return vertices[v];
	}
	VData const &aggregate_vertex_data(size_t v1, size_t v2) const {
		check_vertex_index(v1);
		check_vertex_index(v2);
		VData *rec = aggregate_vertex_data_dfs(v1, v2, vertices[v1]);
		VData rv = *rec;
		delete rec;
		return rv;
	}
private:
	// A helper for functions which check data for edges
	EData const *get_edge(size_t v1, size_t v2) const {
		check_vertex_index(v1);
		check_vertex_index(v2);
		for (pair<size_t, EData> const &p : edges[v1]) {
			if (p.first == v2) {
				return &(p.second);
			}
		}
		return nullptr;
	}
	// DFS implementations for various methods
	bool has_path_dfs(size_t v1, size_t v2) const {
		if (v1 == v2) {
			return true;
		} else if (used[v1]) {
			return false;
		} else {
			used[v1] = true;
			for (pair<size_t, EData> const &p : edges[v1]) {
				if (has_path_dfs(p.first, v2)) {
					return true;
				}
			}
			return false;
		}
	}
	VData *aggregate_vertex_data_dfs(size_t v1, size_t v2, VData soFar) const {
		if (v1 == v2) {
			return new VData(soFar);
		} else if (used[v1]) {
			return nullptr;
		} else {
			used[v1] = true;
			for (pair<size_t, EData> const &p : edges[v1]) {
				VData *rv = aggregate_vertex_data_dfs(p.first, v2, vertex_aggregator(soFar, vertices[v2]));
				if (rv != nullptr) {
					return rv;
				}
			}
			return nullptr;
		}
	}
	EData *aggregate_edge_data_dfs(size_t v1, size_t v2, EData *soFar) const {
		if (v1 == v2) {
			return soFar;
		} else if (used[v1]) {
			delete soFar;
			return nullptr;
		} else {
			used[v1] = true;
			for (pair<size_t, EData> const &p : edges[v1]) {
				EData *rv = aggregate_edge_data_dfs(p.first, v2, soFar == nullptr ? new EData(p.second) : new EData(edge_aggregator(*soFar, p.second)));
				if (rv != nullptr) {
					delete soFar;
					return rv;
				}
			}
			delete soFar;
			return nullptr;
		}
	}
    // asserts
    void check_vertex_index(size_t v) const {
        if (v >= vertices.size()) {
            throw invalid_argument("wrong vertex index");
        }
    }
    // fields
	vector<VData> vertices;
	mutable vector<bool> used;
	vector<vector<pair<size_t, EData>>> edges;
	function<VData(VData const&, VData const&)> vertex_aggregator;
	function<EData(EData const&, EData const&)> edge_aggregator;
};

#endif
