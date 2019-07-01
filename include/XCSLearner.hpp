#pragma once

#include <xcs.hpp>

namespace xcs_rc {

// TODO: would be cool to have user defined type

class XCSLearner {
	public:
		XCSLearner(ActionSpace as) {
			action_space = as;
		}

		Action take_action(std::string state, ActionMode mode);
		void update_with_reward(std::string state, const Action act, double reward);

		/// Returns read only reference to population
		const ClassifierSet& get_population() const {
			return this->pop;
		}
		void set_maxpopsize(size_t size) {
			max_pop_size = size;
		}

		size_t combining_period = 0;
		size_t trials = 0;
		void reset();
	private:
		int input_mode;
		ClassifierSet pop;
		ClassifierSet match_set; // possible to remove?
		ClassifierSet action_set; // possible to remove?
		ActionSpace action_space;
		size_t max_pop_size = MAX_POP_SIZE;

		bool dirty = false; // was MODIFIED
};

} // namespace


