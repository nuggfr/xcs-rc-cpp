#include <utils.hpp>

#include <random>

double
random_number(const double min, const double max) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<double> dis(min, max);

	return dis(gen);
}

unsigned
random_uint(const unsigned min, const unsigned max) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> dis(min, max);

	return dis(gen);
}

bool
random_choice(const double& prob_true) {
	if (random_number(0, 1) < prob_true)
		return true;
	return false;
}
