#ifndef __PASL_RC_TREE_BUILDER_HPP__
#define __PASL_RC_TREE_BUILDER_HPP__

#include <functional>
#include <stdexcept>
#include <vector>

#include "simpleDSU.hpp"

/**
 * An rc_forest_builder class which supports adding edges and vertices.
 * An instance of rc_forest_builder can be used to build an rc_forest.
 */
template<typename VData, typename EData>
class rc_forest_builder {
public:
	/* Constructors */
	rc_forest_builder(
		std::function<VData(VData const&, VData const&)> vertex_aggregator = [](VData const &, VData const &) -> VData { return VData(); },
		std::function<EData(EData const&, EData const&)> edge_aggregator   = [](EData const &, EData const &) -> EData { return EData(); }
	) : vertex_aggregator(vertex_aggregator), edge_aggregator(edge_aggregator) {}

	/* Vertex-oriented methods */
	size_t vertex_count() const {
		return vertices.size();
	}
	void add_vertex(VData const &data = VData()) {
		vertices.push_back(data);
		edges.emplace_back();
		connectivity.add_vertex();
	}
	VData vertex_data(size_t v) const {
		check_vertex_index(v);
		return vertices[v];
	}

	/* Edge-oriented methods */
	void add_edge(size_t v1, size_t v2, EData const &data = EData()) {
		check_vertex_index(v1);
		check_vertex_index(v2);
		if (connectivity.get(v1) == connectivity.get(v2)) {
			throw std::invalid_argument("trying to make a loop in the forest");
		}
		edges[v1].emplace_back(v2, data);
		edges[v2].emplace_back(v1, data);
		connectivity.unite(v1, v2);
	}

	/* Other methods */
	bool has_path(size_t v1, size_t v2) const {
		check_vertex_index(v1);
		check_vertex_index(v2);
		return connectivity.get(v1) == connectivity.get(v2);
	}

	/* Data getters */
	std::vector<VData> const &get_vertices() const {
		return vertices;
	}
	std::vector<std::vector<std::pair<size_t, EData>>> const &get_edges() const {
		return edges;
	}
	std::function<VData(VData const&, VData const&)> const &get_vertex_aggregator() const {
		return vertex_aggregator;
	}
	std::function<EData(EData const&, EData const&)> const &get_edge_aggregator() const {
		return edge_aggregator;
	}
private:
	// asserts
	void check_vertex_index(size_t v) const {
		if (v >= vertices.size()) {
			throw std::invalid_argument("wrong vertex index");
		}
	}
	// fields
	std::vector<VData> vertices;
	std::vector<std::vector<std::pair<size_t, EData>>> edges;
	std::function<VData(VData const&, VData const&)> vertex_aggregator;
	std::function<EData(EData const&, EData const&)> edge_aggregator;
	simple_dsu connectivity;
};

#endif
