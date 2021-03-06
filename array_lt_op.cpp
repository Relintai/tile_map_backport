
#include "array_lt_op.h"

bool operator<(const Array &p_array_a, const Array &p_array_b) {
	int a_len = p_array_a.size();
	int b_len = p_array_b.size();

	int min_cmp = MIN(a_len, b_len);

	for (int i = 0; i < min_cmp; i++) {
		if (p_array_a.operator[](i) < p_array_b[i]) {
			return true;
		} else if (p_array_b[i] < p_array_a.operator[](i)) {
			return false;
		}
	}

	return a_len < b_len;
}

