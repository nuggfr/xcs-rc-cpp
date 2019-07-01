#include <XCSLearner.hpp>
#include <algorithm>

namespace xcs_rc {

Action XCSLearner::take_action(std::string state, ActionMode mode) {
	auto ms = generate_match_set(pop, action_space, state, max_pop_size);
	match_set = ms.first;
	dirty |= ms.second;

	PredictionArray pa = generate_prediction_array(match_set);

	Action output = select_action(pa, mode);
	action_set = generate_action_set(match_set, output);

	trials++;

	return output;
}

void XCSLearner::update_with_reward(std::string origInput, const Action act, double reward) {
	dirty |= update_set(origInput, act, reward, action_set, pop);
	if ((trials % combining_period == 0) && dirty) {
		std::sort(pop.begin(), pop.end(), [](const ClassifierPtr& l, const ClassifierPtr& r) { return *l < *r; });
		dirty |= combine_set(action_space, pop);
		// TODO: intentional?
		dirty = false;
	}
}

void XCSLearner::reset() {
	this->pop.clear();
	this->match_set.clear();
	this->action_set.clear();
	trials = 0;
}

}
