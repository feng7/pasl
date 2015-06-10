#ifndef __RC_TREE_HPP__
#define __RC_TREE_HPP__

#include <functional>
#include <stdexcept>
#include <vector>

template<typename VertexData, typename EdgeData>
class rc_forest_builder;

template<typename VertexData, typename EdgeData>
class rc_forest {
public:
	int 		vertex_count() const;
	bool 		has_edge(int v1, int v2) const;
	bool		has_path(int v1, int v2) const;
	EdgeData 	edge_data(int v1, int v2) const;
	VertexData	vertex_data(int v) const;
	VertexData 	aggregate_vertex_data(int v1, int v2) const;
	EdgeData	aggregate_edge_data(int v1, int v2) const;
private:
	friend class rc_forest_builder<VertexData, EdgeData>;
	// implementation
};

template<typename VertexData, typename EdgeData>
class rc_forest_builder {
public:
	/* Constructors */
	rc_forest_builder(
		std::function<VertexData(VertexData const&, VertexData const&)> vertexAggregate,
		std::function<EdgeData(EdgeData const&, EdgeData const&)> edgeAggregate
	);

	/* Build methods */
	rc_forest<VertexData, EdgeData> build();

	/* Vertex-oriented methods */
	int	vertex_count() const {
		return vertices.size();
	}
	void add_vertex(VertexData const &data) {
		vertices.push_back(data);
		edges.emplace_back();
	}
	VertexData vertex_data(int v) const {
		check_vertex_index(v);
		return vertices[v];
	}

	/* Edge-oriented methods */
	void add_edge(int v1, int v2, EdgeData const &data) {
		check_vertex_index(v1);
		check_vertex_index(v2);
		edges[v1].emplace_back(v2, data);
		edges[v2].emplace_back(v1, data);
	}

	// TODO either implement or forget about it
	bool 		has_edge(int v1, int v2) const;
	EdgeData 	edge_data(int v1, int v2) const;
	void 		cut(int v1, int v2);
private:
	// asserts
	void check_vertex_index(int v) {
		if (v < 0 || v >= vertices.size()) {
			throw std::invalid_argument("wrong vertex index");
		}
	}
	// fields
	std::vector<VertexData> vertices;
	std::vector<std::vector<std::pair<int, EdgeData>>> edges;
};

#endif
