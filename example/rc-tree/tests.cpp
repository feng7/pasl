#include <stdexcept>
#include <functional>
#include <string>

#include "rcForest.hpp"
#include "rcForestBuilder.hpp"

struct nothing {};

void real_assert(std::string const &msg, bool value) {
	if (!value) throw std::runtime_error(msg);
}

void testConnectivity() {
	rc_forest_builder<nothing, nothing> builder;

	const int SIZE = 6;
	for (int i = 0; i < SIZE; ++i) {
		builder.add_vertex();
	}

	bool checkup[SIZE][SIZE];
	for (int i = 0; i < SIZE; ++i) {
		for (int j = 0; j < SIZE; ++j) {
			checkup[i][j] = i == j;
		}
	}

	builder.add_edge(0, 5); checkup[0][5] = checkup[5][0] = true;
	builder.add_edge(4, 5); checkup[4][5] = checkup[5][4] = true;
	builder.add_edge(2, 3); checkup[2][3] = checkup[3][2] = true;

	naive_rc_forest<nothing, nothing> result = naive_rc_forest<nothing, nothing>(builder);

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
