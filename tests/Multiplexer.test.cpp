#include <functional>
#include <algorithm>
#include <ostream>
#include <fstream>
#include <unordered_map>

#include "../include/XCSLearner.hpp"
#include <utils.hpp>

using xcs_rc::XCSLearner;

struct TestRow {
	size_t trials;
	double correctness_rate;
	size_t number_of_classifiers;
	size_t exp_classifiers;

	friend std::ostream& operator<<(std::ostream& out, const TestRow& row) {
		const char sep = ';';
		return out << row.trials << sep << row.correctness_rate << sep << row.number_of_classifiers << sep
		           << row.exp_classifiers;
	}
};

using PerformanceResult = std::vector<TestRow>;

struct TestResult {
	PerformanceResult performance;
	ClassifierSet population;
};

bool
save_population(const ClassifierSet& pop, const char* filename);

TestResult
test_multiplexer(unsigned address_bits, int inputMode, unsigned int debugMode) {
	TestResult result;
	int inputLength = address_bits + pow(2, address_bits);
	unsigned correct = 0;

	ActionSpace actions = {0, 1};
	XCSLearner learner(actions);

	int binary_tcombs[] = { 0, 40, 100, 200, 500, 1000 };
	int binary_popsizes[] = { 0, 100, 400, 800, 1000, 2000 };
	int binary_maxtrials[] = { 0, 1000, 10000, 30000, 50000, 100000 };

	int real_tcombs[] = { 0, 40, 100 };
	int real_popsizes[] = { 0, 500, 1000 };
	int real_maxtrials[] = { 0, 1000, 40000 };

	const size_t NUM_OF_TRIALS = (inputMode == 0)? binary_maxtrials[address_bits]:real_maxtrials[address_bits];
	const size_t T_COMB = (inputMode == 0)? binary_tcombs[address_bits]:real_tcombs[address_bits];
	const size_t MAXPOPSIZE = (inputMode == 0)? binary_popsizes[address_bits]:real_popsizes[address_bits];

	learner.combining_period = T_COMB;
	learner.set_maxpopsize(MAXPOPSIZE);

	srand(0);
	for (size_t trials = 1; trials <= NUM_OF_TRIALS; trials++) {
		const ActionMode amode = (trials % 2 == 0) ? ActionMode::Explore : ActionMode::Exploit;

		// GENERATE INPUT STATE
		std::string state = "";
		std::string binaryState = "";

		for (int i=0; i<inputLength; i++) {
			double num = round(1000 * random_number(0, 1)) / 1000;
			binaryState += std::to_string((int)round(num));

			if (inputMode == 1) {
				std::string next = std::to_string(num);
				next.resize(5);
				state += next;
				if (i < inputLength - 1) state += ";";
			}
		}
		if (inputMode == 0) state = binaryState;

		// SEND STATE TO XCS AND RETRIEVE OUTPUT
		Action output = learner.take_action(state, amode);

		// PREPARE REWARD FOR OUTPUT
		int pos = address_bits;
		for (size_t i = 0; i < address_bits; i++) {
			size_t j = (size_t)binaryState[i] - 48;
			pos += j * pow(2, (address_bits - i - 1));
		}
		int correctAnswer = (int)binaryState[pos] - 48;

		// ASSIGN REWARD AND UPDATE SET
		double reward = 0;
		if (output == correctAnswer)
			reward = REWARD_MAX;

		learner.update_with_reward(state, output, reward);

		if (amode == ActionMode::Exploit) {
			if (reward == REWARD_MAX)
				correct++;
		}

		// RECORD WITH T_COMB SLIDING WINDOW
		const auto pop = learner.get_population();
		if (trials % T_COMB == 0) {
			for (auto& c : pop) {
				if (c->numerosity == 0) std::cout << "Num 0: " << c->cond << ":" << c->rule.act << "->" << c->prediction << "; Num: " << c->numerosity << "; Exp: " << c->experience << "; Fit: " << c->fitness << std::endl;
			}

			clexp exps = get_exp_classifiers(pop);
			double correctness_rate = (double)correct / (T_COMB / 2);

			if (debugMode>0) {
				std::cout << "Trial: " << trials << "; Perf: " << correctness_rate << "; Popsize: " << learner.get_population().size() << "; ExpCl: " << exps.cl_exp << "; TotExp: " << exps.tot_exp << std::endl;
				if (debugMode>1) {
					print_pop(pop, true);
					std::cout << std::endl;
				}
			}

			result.performance.push_back(TestRow{learner.trials, correctness_rate, (size_t) pop.size(), exps.cl_exp});
			correct = 0;

			const size_t buflen = 100;
			char filename[buflen];
			std::snprintf(filename, buflen, "mp_pop_trial_%lu.csv", learner.trials);
			save_population(pop, filename);
		}

	}

	result.population = learner.get_population();
	return result;
}

bool
save_performance(const PerformanceResult& result, const char* filename) {
	std::ofstream file;

	file.open(filename);
	if (!file.is_open()) {
		std::cerr << "Error opening file " << filename << " for writing." << std::endl;
		return false;
	}

	file << "sep=;" << std::endl;
	for (auto& row : result) {
		file << row << std::endl;
	}

	file.close();

	return true;
}

bool
save_population(const ClassifierSet& pop, const char* filename) {
	std::ofstream file;

	file.open(filename);
	if (!file.is_open()) {
		std::cerr << "error opening file " << filename << " for writing" << std::endl;
		return false;
	}

	file << "sep=;" << std::endl;
	file << "No;Cond;Act;Pred;Fit;PredErr;Num;Exp" << std::endl;
	size_t i = 0;
	for (auto& cl : pop) {
		if (cl->experience > 0)
			file << ++i << ";" << *cl << std::endl;
	}
	for (auto& cl : pop) {
		if (cl->experience == 0)
			file << ++i << ";" << *cl << std::endl;
	}

	file.close();

	return true;
}

PerformanceResult
average_performance(const std::vector<PerformanceResult*> results) {
	const size_t n_res = results.size();
	assert(n_res > 0);
	PerformanceResult average;

	const size_t n_rows = results.at(0)->size();
	assert(n_rows > 0);

	for (size_t cur_row = 0; cur_row < n_rows; cur_row++) {
		TestRow average_row;
		average_row.trials = (*results[0])[cur_row].trials;

		double av_correct = 0;
		for (auto& res : results) {
			av_correct += (*res)[cur_row].correctness_rate;
		}
		av_correct /= (double)n_res;
		average_row.correctness_rate = av_correct;

		double av_pop = 0;
		for (auto& res : results) {
			av_pop += (*res)[cur_row].number_of_classifiers;
		}
		av_pop /= (double)n_res;
		average_row.number_of_classifiers = av_pop;

		double av_exp_pop = 0;
		for (auto& res : results) {
			av_exp_pop += (*res)[cur_row].exp_classifiers;
		}
		av_exp_pop /= (double)n_res;
		average_row.exp_classifiers = av_exp_pop;

		average.push_back(average_row);
	}

	return average;
}

int main() {
	int inputMode = 0; // 0 binary, 1 real
	const unsigned debugMode = 1; // 0 no debug, 1 summary, 2 print pop
	const unsigned ADDRESS_BITS = 3; // max binary 5, real 2
	const unsigned MUX_LEN = ADDRESS_BITS + std::pow(2, ADDRESS_BITS);
	const int SIMULATIONS = 20; // normally 20
	std::string display = (inputMode==0)?"Binary":"Real";

	std::cout << display << " MP" << MUX_LEN << "; Sims = " << SIMULATIONS << std::endl;

	std::vector<TestResult> results;
	for (size_t i=0; i<SIMULATIONS; i++) {
		results.push_back(test_multiplexer(ADDRESS_BITS, inputMode, debugMode));

		std::cout << "SIM " << (i+1) << " COMPLETED" << std::endl;
		if (debugMode>1) {
			std::cout << "FINAL POPULATION" << std::endl;
			print_pop(results[i].population, false);
			std::cout << "END OF SIM " << (i+1) << std::endl << std::endl;
		}
	}

	std::vector<PerformanceResult*> performances;
	for (auto& res : results) {
		performances.push_back(&(res.performance));
	}

	size_t i = 1;
	const size_t bufsize = 100;
	char filename[100];
	for (auto& result : results) {
		std::snprintf(filename, bufsize, "MP%d_Perf_%03lu.csv", MUX_LEN, i);
		assert(save_performance(result.performance, filename));

		std::snprintf(filename, bufsize, "MP%d_Pop_%03lu.csv", MUX_LEN, i);
		assert(save_population(result.population, filename));

		i++;
	}

	auto average = average_performance(performances);
	std::snprintf(filename, bufsize, "MP%d_Perf_avr.csv", MUX_LEN);
	assert(save_performance(average, filename));

	return 0;
}
