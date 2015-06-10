#include <functional>
#include <string>

#include "rcTree.hpp"

struct nothing {} nothing_v;
nothing aggregate_nothing(nothing const &a, nothing const &b) {
	return nothing_v;
}

void real_assert(std::string const &msg, bool value) {
	if (!value) throw msg;
}

void testConnectivity() {
	rc_forest_builder<nothing, nothing> builder(aggregate_nothing, aggregate_nothing);

	const int SIZE = 6;
	for (int i = 0; i < SIZE; ++i) {
		builder.add_vertex(nothing_v);
	}

	bool checkup[SIZE][SIZE];
	for (int i = 0; i < SIZE; ++i) {
		for (int j = 0; j < SIZE; ++j) {
			checkup[i][j] = false;
		}
	}

	builder.add_edge(0, 5, nothing_v); checkup[0][5] = checkup[5][0] = true;
	builder.add_edge(4, 5, nothing_v); checkup[4][5] = checkup[5][4] = true;
	builder.add_edge(2, 3, nothing_v); checkup[2][3] = checkup[3][2] = true;

	rc_forest<nothing, nothing> result = builder.build();

	for (int k = 0; k < SIZE; ++k) {
		for (int i = 0; i < SIZE; ++i) {
			for (int j = 0; j < SIZE; ++j) {
				checkup[i][j] |= checkup[i][k] && checkup[k][j];
			}
		}
	}

	for (int i = 0; i < SIZE; ++i) {
		for (int j = 0; j < SIZE; ++j) {
			real_assert("connectivity is wrong", checkup[i][j] == result.has_path(i, j));
		}
	}
}

int main() {
	testConnectivity();
	return 0;
}
