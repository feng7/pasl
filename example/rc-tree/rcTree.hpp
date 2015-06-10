#ifndef __PASL_RC_TREE_HPP__
#define __PASL_RC_TREE_HPP__

#include <functional>
#include <stdexcept>
#include <vector>

#include "simpleDSU.hpp"

/** A forward declaration of the rc_forest_builder class **/
template<typename VData, typename EData>
class rc_forest_builder;

/** An rc_forest class
 * which supports querying aggregates
 * of edge and vertex data **/
template<typename VData, typename EData>
class rc_forest {
public:
	int		vertex_count() const;
	bool	has_edge(int v1, int v2) const;
	bool 	has_path(int v1, int v2) const;
	EData 	edge_data(int v1, int v2) const;
	EData	aggregate_edge_data(int v1, int v2) const;
	VData	vertex_data(int v) const;
	VData 	aggregate_vertex_data(int v1, int v2) const;
private:
	friend class rc_forest_builder<VData, EData>;
	// implementation
};

/** An rc_forest_builder class
 * which builds an rc_forest instance
 * from given edges, vertex infos and edge infos
 **/
template<typename VData, typename EData>
class rc_forest_builder {
public:
	/* Constructors */
	rc_forest_builder(
		std::function<VData(VData const&, VData const&)> vertexAggregate = [](VData const &, VData const &) -> VData { return VData(); },
		std::function<EData(EData const&, EData const&)> edgeAggregate	 = [](EData const &, EData const &) -> EData { return EData(); }
	);

	/* Build methods */
	rc_forest<VData, EData> build();

	/* Vertex-oriented methods */
	int	vertex_count() const {
		return vertices.size();
	}
	void add_vertex(VData const &data = VData()) {
		vertices.push_back(data);
		edges.emplace_back();
		connectivity.add_vertex();
	}
	VData vertex_data(int v) const {
		check_vertex_index(v);
		return vertices[v];
	}

	/* Edge-oriented methods */
	void add_edge(int v1, int v2, EData const &data = EData()) {
		check_vertex_index(v1);
		check_vertex_index(v2);
		if (connectivity.get(v1) == connectivity.get(v2)) {
			throw std::invalid_argument("trying to make a loop in the forest");
		}
		edges[v1].emplace_back(v2, data);
		edges[v2].emplace_back(v1, data);
		connectivity.unite(v1, v2);
	}
	// Too slow, but do we need it at all?
	bool has_edge(int v1, int v2) const {
		check_vertex_index(v1);
		check_vertex_index(v2);
		for (int i = 0, last = edges[v1].size(); i < last; ++i) {
			if (edges[v1][i].first == v2) {
				return true;
			}
		}
		return false;
	}
	// Too slow, but do we need it at all?
	EData edge_data(int v1, int v2) const {
		check_vertex_index(v1);
		check_vertex_index(v2);
		for (int i = 0, last = edges[v1].size(); i < last; ++i) {
			if (edges[v1][i].first == v2) {
				return edges[v1][i].second;
			}
		}
		throw std::invalid_argument("no such edge");
	}

	/* Other methods */
	bool has_path(int v1, int v2) const {
		check_vertex_index(v1);
		check_vertex_index(v2);
		return connectivity.get(v1) == connectivity.get(v2);
	}
private:
	// asserts
	void check_vertex_index(int v) {
		if (v < 0 || v >= vertices.size()) {
			throw std::invalid_argument("wrong vertex index");
		}
	}
	// fields
	std::vector<VData> vertices;
	std::vector<std::vector<std::pair<int, EData>>> edges;
	simple_dsu connectivity;
};

#endif
