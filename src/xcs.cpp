#include <cassert>
#include <cmath>
#include <iomanip> // std::setprecision
#include <iterator>
#include <utility> // for std::pair
#include <vector>
#include <stdlib.h>
#include <boost/tokenizer.hpp>

#include <utils.hpp>
#include <xcs.hpp>

using std::stringstream;
using std::chrono::system_clock;
using nonstd::nullopt;
using namespace boost;

bool
within_range(double v1, double v2, double tolerance) {
	return v2 + tolerance >= v1 && v2 <= v1 + tolerance;
}

clexp
get_exp_classifiers(const ClassifierSet& set) {
	clexp value;
	unsigned int num = 0, tot = 0;
	for (const auto& c: set)
		if (c->experience > 0) {
			num++;
			tot += c->experience;
		}
	value.cl_exp = num;
	value.tot_exp = tot;
	return value;
}

/// Returns the string representation of elements
/// TODO: instead of string consider somehting like vector<Either<bool,double>>?
std::string
compose_cond(const vector<double>& elements) {
	std::string covered = "";
	size_t len = elements.size()/2;

	// check binary
	bool isBinary = true;
	for (size_t i=0; i<2*len; i++)
		if (elements[i] != 0.0 && elements[i] != 1.0) isBinary = false;

	for (size_t i=0; i<len; i++) {
		std::string element = "";
		if (isBinary) {
			element = (elements[2*i] == elements[2*i+1])? std::to_string((int) elements[2*i]):"#";
		} else {
			std::string el1 = std::to_string(elements[2*i]);
			std::string el2 = std::to_string(elements[2*i+1]);
			el1.resize(5);
			el2.resize(5);
			element = "["+el1;
			element += (el1 == el2)?"]":".."+el2+"]";
		}
		covered += element;
	}

	return covered;
}

/// Returns true if the condition rule matches on data
bool
elements_match(const vector<double>& elements, const vector<double>& input) {
	// Make sure elements have twice length
	size_t condLen = elements.size();
	size_t len = input.size();
	if (condLen != 2 * len) return false;

	for (size_t i = 0; i < len; i++) {
		if (elements[2*i] > input[i] || elements[2*i+1] < input[i])
			return false;
	}
	return true;
}

/// Returns true if the conditions can match at least one point
int
count_overlap(const vector<double>& elements1, const vector<double>& elements2) {
	// Make sure elements have twice length
	size_t len1 = elements1.size();
	size_t len2 = elements2.size();
	if (len1 != len2) return 0;

	int ovr = len1/2;

	for (size_t i=0; i<len1/2; i++) {
		if (elements1[2*i] > elements2[2*i+1] || elements2[2*i] > elements1[2*i+1])
			ovr--;
	}

	return ovr;
}

/// Returns true if the conditions can match at least one point
bool
elements_overlap(const vector<double>& elements1, const vector<double>& elements2) {
	int count = count_overlap(elements1, elements2);
	int len = elements1.size()/2;

	return (count == len);
}

string
act_str(const Rule& rule) {
	return std::to_string(rule.act);
}

std::string
rule_str(const Rule& rule) {
	return compose_cond(rule.elements) + ":" + act_str(rule);
}

// Was classifier_str before
std::ostream&
operator<<(std::ostream& out, const Classifier& cl) {
	const char sep = ';';

	// Format rule
	out << cl.cond << sep;
	out << act_str(cl.rule) << sep;

	// Print parameters
	out << std::fixed << std::setprecision(3) << cl.prediction << sep;
	out << std::fixed << std::setprecision(3) << cl.fitness << sep;
	out << std::fixed << std::setprecision(3) << cl.prediction_error << sep;
	out << cl.numerosity << sep;
	out << cl.experience;

	return out;
}

vector<double>
transform_input(std::string origInput) {
	size_t inputLen = origInput.size();
	vector<double> input;

	int inputMode = 0;
	for (size_t i=0; i<origInput.size(); i++)
		if (origInput[i] != '0' && origInput[i] != '1') inputMode = 1;

	if (inputMode == 0) {
		input.resize(inputLen);
		for (size_t i=0; i<inputLen; i++) input[i] = 1.0 * (origInput[i] - 48);
	} else {
	    char_separator<char> sep(";");
	    tokenizer<char_separator<char>> tokens(origInput, sep);
	    for (const string& t : tokens) input.push_back(stod(t));
	}

	return input;
}

/// Generates a match set using the population and problem input
std::pair<ClassifierSet, bool>
generate_match_set(ClassifierSet& pop, const ActionSpace& as, std::string origInput, const size_t& max_pop_size) {
	ClassifierSet match_set;
	bool modified = false;

	vector<double> input = transform_input(origInput);

	while (match_set.empty()) {
		for (auto it = pop.begin(); it != pop.end(); it++) {
			if (elements_match((*it)->rule.elements, input)) {
				match_set.push_back(*it);
			}
		}

		size_t pop_num = set_numerosity(pop);
		const size_t num_diff_actions = num_different_actions(match_set);
		const size_t num_of_actions = as.size();
		const int space = as.size() - num_diff_actions;
		if (space > 0) {
			if (pop_num + space > max_pop_size)
				do {
					bool deleting = false;
					for (size_t i=0; i<pop.size(); i++)
						if (pop[i]->experience == 0) {
							delete_classifier(pop, pop[i]);
							deleting = true;
						}
					if (!deleting) delete_from_population(pop, input);
					modified |=  deleting;
					pop_num = set_numerosity(pop);
				} while (pop_num + num_of_actions - num_diff_actions > max_pop_size);
			pop.push_back(std::make_shared<Classifier>(generate_covering_classifier(match_set, as, input)));
			match_set.clear(); // try again with added classifier
		}
	}
	return std::make_pair(match_set, modified);
}

/// Calculates the total numerosity of a ClassifierSet
unsigned int
set_numerosity(const ClassifierSet &set) {
	unsigned int total_num = 0;
	for (const auto& c : set) {
		total_num += c->numerosity;
	}
	return total_num;
}

/// Calculates the total fitness of a ClassifierSet
double
set_fitness(const ClassifierSet &set) {
	double total_fitness = 0;
	for (const auto& c : set) {
		total_fitness += c->fitness;
	}
	return total_fitness;
}

/// Returns true if it has removed/deleted a classifier from the population
bool
delete_from_population(ClassifierSet& pop, const vector<double>& input) {
	unsigned int pop_numerosity = set_numerosity(pop);

	//	return false;

	const double pop_fitness = set_fitness(pop);
	const double mean_fitness = pop_fitness / pop_numerosity;

	double vote_sum = 0;
	for (const auto& c : pop) {
		vote_sum += (double) get_del_prop(*c, mean_fitness);
	}

	// Roulete like deletion
	const double choice_point = random_number(0, vote_sum);
	vote_sum = 0;
	int pop_size = pop.size();
	for (int i=0; i<pop_size; i++) {
		ClassifierPtr c = pop[i];
		vote_sum += get_del_prop(*c, mean_fitness);

		if (vote_sum > choice_point) {
			if (elements_match(c->rule.elements, input)) {
				do {
					i++;
					if (i == pop_size) i=0;
					c = pop[i];
				} while (elements_match(c->rule.elements, input));
			}

			c->numerosity--;
			//std::cout << c->cond << ":" << c->rule.act << "->" << c->prediction << "; Num: " << c->numerosity << "; Exp: " << c->experience << "; Fit: " << c->fitness << " is REMOVED." << std::endl;
			if (c->numerosity == 0) delete_classifier(pop, c);
			return true;
		}
	} // for

	return false;
}

double
get_del_prop(const Classifier& cl, const double mean_fitness) {
	assert(cl.numerosity != 0);
	double vote = cl.actionset_size * cl.numerosity;
	if (cl.fitness / cl.numerosity >= DELTA_DELETION * mean_fitness ||
			cl.experience < THETA_DEL) {
		vote = vote * mean_fitness / (cl.fitness / cl.numerosity);
	}
//	std::cout << "Vote: " << cl.fitness << " ... " << cl.numerosity << " ... " << vote << std::endl;
	return vote;
}

set<Action>
actions_diff(const set<Action>& lhs, const set<Action>& rhs) {
	set<Action> diff;
	std::set_difference(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::inserter(diff, diff.end()));
	return diff;
}

Classifier
generate_classifier(const vector<double>& input) {
	Classifier cl_new;
	size_t len = input.size();
	cl_new.rule.elements.resize(2*len);

	// check binary
	bool isBinary = true;
	for (size_t i=0; i<len; i++)
		if (input[i] != 0.0 && input[i] != 1.0) isBinary = false;

	std::string covered = "";
	for (size_t i=0; i<len; i++) {
		cl_new.rule.elements[2*i] = input[i];
		cl_new.rule.elements[2*i+1] = input[i];
		std::string the_input = std::to_string(input[i]);
		std::string element = "";
		if (isBinary) {
			the_input.resize(1);
			element = the_input;
		} else
		{
			the_input.resize(5);
			element = "["+the_input+"]";
		}
		covered += element;
	}
	cl_new.cond = covered;

	cl_new.rule.act = 0;

	// Random action not present in [M]
	cl_new.prediction = PREDICTION_INIT;
	cl_new.prediction_error = PREDICTION_ERROR_INIT;
	cl_new.fitness = FITNESS_INIT;

	cl_new.experience = 0;
	cl_new.numerosity = 1;
	//cl_new.timestamp = std::chrono::system_clock::now();
	cl_new.actionset_size = 1;
	cl_new.disproving = 0;

	return cl_new;
}

Classifier
generate_covering_classifier(ClassifierSet& match_set, const ActionSpace& as, const vector<double>& input) {
	Classifier cl_new = generate_classifier(input);

	// calculates all remaining actions = actions - already present actions
	auto present_a = present_actions(classifer_set_rules(match_set));
	auto remaining_actions = actions_diff(as, present_a);

	cl_new.rule.act = random_action(remaining_actions).value_or(*all_actions().cbegin());

	return cl_new;
}

set<Action>
present_actions(const RuleSet& rules) {
	set<Action> actions;

	for (Rule r : rules) {
		const Action a = r.act;
		actions.insert(a);
	}

	return actions;
}

optional<Action>
random_action(const set<Action>& actions) {
	if (actions.size() < 1)
		return nullopt;
	auto randi = random_uint(0, actions.size() - 1);
	set<Action>::const_iterator it(actions.begin());
	std::advance(it, randi);
	return optional<Action>(*it);
}

// TODO: Probably remove
RuleSet
classifer_set_rules(const ClassifierSet& classifiers) {
	RuleSet rules;
	for (const auto& c : classifiers)
		rules.push_back(c->rule);
	return rules;
}

unsigned int
num_different_actions(const RuleSet& rules) {
	return present_actions(rules).size();
}

unsigned int
num_different_actions(const ClassifierSet& classifiers) {
	const auto rules = classifer_set_rules(classifiers);
	return num_different_actions(rules);
}

set<Action>
all_actions() {
	return set<Action> {0, 1};
}

PredictionArray
generate_prediction_array(const ClassifierSet& match_set) {
	PredictionArray pa;
	PredictionArray fsa;
	for (const auto& cl : match_set) {
		const auto& act = cl->rule.act;
		if (pa.find(act) == pa.end()) {
			pa[act] = cl->prediction * cl->fitness;
		} else {
			pa[act] = pa[act] + cl->prediction * cl->fitness;
		}
		fsa[act] = fsa[act] + cl->fitness;
	}

	for (const auto& act : all_actions()) {
		if (fsa.find(act) != fsa.end() && fsa[act] != 0) {
			pa[act] = pa[act] / fsa[act];
		}
	}
	return pa;
}

using pair_type = PredictionArray::value_type;
int
select_action(const PredictionArray& pa, ActionMode mode) {
	if (mode == ActionMode::Explore || pa.size() < 1) {
		// Explore using a random action not present in pa
		auto ra = all_actions();
		for (const auto& kv : pa) {
			ra.erase(kv.first);
		}
		return random_action(ra).value_or(random_action(all_actions()).value());
	} else {
		// Pure exploitation using the best action in pa
		assert(pa.size() > 0);
		auto best_action = std::max_element(std::begin(pa), std::end(pa), [](const pair_type& p1, const pair_type& p2) {
			return p1.second < p2.second;
		});
		return best_action->first;
	}
}

ClassifierSet
generate_action_set(const ClassifierSet& match_set, const Action act) {
	ClassifierSet action_set;
	for (const auto& cl : match_set) {
		if (cl->rule.act == act)
			action_set.push_back(cl);
	}
	return action_set;
}

void
update_fitness(ClassifierSet& action_set) {
	double accuracy_sum = 0;
	const auto ass = action_set.size();

	vector<double> k;
	k.resize(ass, 0.0);
	for (size_t i = 0; i < ass; i++) {
		const auto& cl = action_set.at(i);
		if (cl->prediction_error < EPSILON_ZERO)
			k[i] = 1;
		else
			k[i] = ALPHA * pow((cl->prediction_error / EPSILON_ZERO), -POWER_PARAMETER);
		accuracy_sum += k[i] * cl->numerosity;
	}
	for (size_t i = 0; i < ass; i++) {
		auto& cl = action_set.at(i);
		cl->fitness += BETA * ((k[i] * cl->numerosity) / accuracy_sum - cl->fitness);
	}
}

/// Returns true if the population was modified
bool
update_set(std::string origInput, const Action act, double reward, ClassifierSet& action_set, ClassifierSet& pop) {
	bool modified = false;
	unsigned int total_numerosity = 0;
	for (const auto& cl : action_set) {
		total_numerosity += cl->numerosity;
	}

	for (auto& cl : action_set) {
		cl->experience++;
		if (cl->experience == MIN_EXP)
			modified = true;

		// Update prediction
		if (cl->experience < 1 / BETA)
			cl->prediction = cl->prediction + (reward - cl->prediction) / cl->experience;
		else
			cl->prediction = cl->prediction + BETA * (reward - cl->prediction);

		// Update action set size estimate
		if (cl->experience < 1 / BETA)
			cl->actionset_size = cl->actionset_size + (total_numerosity - cl->actionset_size) / cl->experience;
		else
			cl->actionset_size = cl->actionset_size + (total_numerosity - cl->actionset_size);

		double oldPredErr = cl->prediction_error;
		// Update prediction_error
		if (cl->experience < 1 / BETA) {
			assert(cl->experience != 0);
			cl->prediction_error =
			    cl->prediction_error + (std::abs(reward - cl->prediction) - cl->prediction_error) / cl->experience;
		} else
			cl->prediction_error =
			    cl->prediction_error + BETA * (std::abs(reward - cl->prediction) - cl->prediction_error);

		if (cl->experience >= 2*MIN_EXP && oldPredErr <= PRED_ERR_TOL && cl->prediction_error > PRED_ERR_TOL) {
			// std::cout << "cl*: " << cl->cond << ":" << cl-> rule.act << "->" <<
			// cl->prediction << " is REMOVED " << std::endl;
			delete_classifier(pop, cl);

			// insert new classifier to population based on the current state
			ClassifierPtr cl_new = std::make_shared<Classifier>(generate_classifier(transform_input(origInput)));
			cl_new->rule.act = act;
			cl_new->prediction = reward;
			cl_new->experience = 1;
			cl_new->prediction_error = std::abs(reward - PREDICTION_INIT);
			pop.push_back(cl_new);

			modified = true;
		}
	}
	update_fitness(action_set);
	return modified;
}

void
insert_into_population(ClassifierSet& pop, const ClassifierPtr& cl) {
	assert(cl->rule.elements.size() > 0);
	for (auto& pcl : pop) {
		if (pcl->rule == cl->rule) {
			pcl->numerosity++;
			return;
		}
	}
	pop.push_back(cl);
}

bool
is_more_general(const Classifier& cl_gen, const Classifier& cl_spc) {
	vector<double> cl1 = cl_gen.rule.elements;
	vector<double> cl2 = cl_spc.rule.elements;

	if (cl1.size() != cl2.size()) return false;

	size_t i = 0;
	const size_t len = cl1.size() / 2;
	do {
		if (cl1[2*i] > cl2[2*i] || cl1[2*i+1] < cl2[2*i+1]) return false;
		i++;
	} while (i < len);

	return true;
}

bool
is_subsumable(ClassifierPtr cl, ClassifierPtr subsumer) {
	vector<double> cl1 = subsumer->rule.elements;
	vector<double> cl2 = cl->rule.elements;

	if (subsumer->rule.act != cl->rule.act) return false;
	if (cl1.size() != cl2.size()) return false;

	const size_t len = cl1.size() / 2;
	for (size_t i=0; i<len; i++)
		if (cl1[2*i] > cl2[2*i] || cl1[2*i+1] < cl2[2*i+1]) return false;

	return true;
}

bool
delete_classifier(ClassifierSet& clset, ClassifierPtr cl) {
	for (auto it = clset.begin(); it != clset.end();) {
		if (**it == *cl) {
			clset.erase(it);
			return true;
		} else {
			it++;
		}
	}
	return false;
}

int
count_cl_action(ClassifierSet& clSet, size_t act) {
	int num = 0;
	for (auto& cl : clSet)
		if (cl->rule.act == act) num++;
	return num;
}

/// Returns true if it has modified the population
bool
remove_outlier(ClassifierSet& pop) {
	bool modified = false;
	vector<ClassifierPtr> clSet;

	for (ClassifierPtr& cl : pop)
		clSet.push_back(cl);

	for (size_t i=0; i<clSet.size(); i++) {
		if (clSet[i]->experience > 0 && clSet[i]->disproving / clSet[i]->experience > pow(10, MAX_DISP_RATE)) {
				/*
				std::cout << "DEL: " << clSet[i]->cond << ":" << clSet[i]->rule.act << "->" << clSet[i]->prediction << std::endl;
				//*/
				delete_classifier(pop, clSet[i]);
				modified = true;
		}
	}
	return modified;
}

bool
combine_set(const ActionSpace& as, ClassifierSet& pop) {
	bool modified = false;
	vector<ClassifierPtr> clCombSet;
	int not_combined = 0;
	size_t combSetSize = 0;

	std::sort(pop.begin(), pop.end(), [](const ClassifierPtr& l, const ClassifierPtr& r) { return *l < *r; });

	for (size_t action = 0; action < as.size() ; action++) {

		// recruiting
		clCombSet.clear();
		for (ClassifierPtr cl : pop) {
			size_t cl_a = cl->rule.act;
			if (cl_a == action) {
				clCombSet.push_back(cl);
			}
		}

		not_combined = 0;

		do {
			combSetSize = clCombSet.size();
			not_combined++;

			for (size_t i=0; i<combSetSize; i++)
				for (size_t j=i+1; j<combSetSize; j++) {

					vector<double> el1 = clCombSet[i]->rule.elements;
					vector<double> el2 = clCombSet[j]->rule.elements;

					if (clCombSet[i]->experience >= MIN_EXP && clCombSet[j]->experience >= MIN_EXP &&
					    within_range(clCombSet[i]->prediction, clCombSet[j]->prediction, PRED_TOL)) {

						auto cl_star = std::make_shared<Classifier>(Classifier());
						double cl_star_pred = 0.0;

						size_t len = el1.size();
						cl_star->rule.elements.resize(len);

						for (size_t k=0; k<len/2; k++) {
							cl_star->rule.elements[2*k]  = (el1[2*k]<el2[2*k])?el1[2*k]:el2[2*k];
							cl_star->rule.elements[2*k+1]= (el1[2*k+1]>el2[2*k+1])?el1[2*k+1]:el2[2*k+1];
						}
						cl_star->rule.act = clCombSet[i]->rule.act;
						cl_star_pred = (clCombSet[i]->prediction * clCombSet[i]->numerosity +
						                clCombSet[j]->prediction * clCombSet[j]->numerosity) /
						               (clCombSet[i]->numerosity + clCombSet[j]->numerosity);

						/*
						std::cout << "cl*: " << compose_cond(cl_star->rule.elements) << ":";
						std::cout << cl_star-> rule.act << "->" << cl_star_pred << " ... ";
						//*/

						// examination
						bool disproved = false;
						for (size_t k=0; k<combSetSize; k++) {

							if (k!=i && k!=j && clCombSet[k]->experience > 0)
								if (elements_overlap(cl_star->rule.elements, clCombSet[k]->rule.elements) &&
									!within_range(cl_star_pred, clCombSet[k]->prediction, PRED_TOL)) {
									disproved = true;
									/*
									std::cout << "DISPROVED by " << compose_cond(clCombSet[k]->rule.elements) << ":";
									std::cout << clCombSet[k]->rule.act << "->" << clCombSet[k]->prediction << std::endl;
									//*/
									if (MAX_DISP_RATE > 0) clCombSet[k]->disproves = true;
										else k = combSetSize;
								}
						}
						if (!disproved) {
							/*
							std::cout << "APPROVED!!!" << std::endl;
							//*/

							// add parents' attribute
							cl_star->cond = compose_cond(cl_star->rule.elements);
							cl_star->experience = clCombSet[i]->experience + clCombSet[j]->experience;
							cl_star->numerosity = clCombSet[i]->numerosity + clCombSet[j]->numerosity;
							cl_star->prediction = cl_star_pred * cl_star->numerosity;

							delete_classifier(pop, clCombSet[j]);
							delete_classifier(pop, clCombSet[i]);
							//std::cout << "P2: " << compose_cond(clCombSet[j]->rule.elements) << ":" << clCombSet[j]->rule.act << "->" << clCombSet[j]->prediction << " is DELETED." << std::endl;
							delete_classifier(clCombSet, clCombSet[j]);
							//std::cout << "P1: " << compose_cond(clCombSet[i]->rule.elements) << ":" << clCombSet[i]->rule.act << "->" << clCombSet[i]->prediction << " is DELETED." << std::endl;
							delete_classifier(clCombSet, clCombSet[i]);
							combSetSize = clCombSet.size();

							vector<int> deletions;
							for (size_t d=0; d<combSetSize; d++) {
								auto& cl = clCombSet[d];
								bool in_range = within_range(cl_star_pred, cl->prediction, PRED_TOL);

								if (is_subsumable(cl, cl_star) && (in_range || cl->experience == 0)) {

									if (cl->experience > 0) {
										// subsume
										cl_star->experience += cl->experience;
										cl_star->numerosity += cl->numerosity;
										cl_star->prediction += cl->prediction * cl->numerosity;
									}
									deletions.push_back(d);
								}
							}
							cl_star->prediction = cl_star->prediction / cl_star->numerosity;

							int del_size = deletions.size();
							if (del_size >0) {
								//std::cout << "Cl*: (" << cl_star->numerosity << ") " << cl_star->cond << ":" << cl_star->rule.act << "->" << cl_star->prediction << std::endl;
								for (int m = del_size-1; m >= 0; m--) {
									//std::cout << compose_cond(clCombSet[deletions[m]]->rule.elements) << ":" << clCombSet[deletions[m]]->rule.act << "->" << clCombSet[deletions[m]]->prediction << " is DELETED." << std::endl;
									delete_classifier(pop, clCombSet[deletions[m]]);
									delete_classifier(clCombSet, clCombSet[deletions[m]]);
								}
							}

							const double exp_lim = 1 / BETA;
							cl_star->prediction_error =
							    (cl_star->experience <= floor(exp_lim))
							        ? std::abs(cl_star->prediction - PREDICTION_INIT) / cl_star->experience
							        : (std::abs(cl_star->prediction - PREDICTION_INIT) / exp_lim) *
							              pow(1 - BETA, cl_star->experience - floor(exp_lim));
							cl_star->fitness = (FITNESS_INIT - 1) * pow(1 - BETA, cl_star->experience) + 1;
							cl_star->disproving = 0;
							//std::cout << "Comb Pred: " << cl_star->prediction << " Exp: " << cl_star->experience << "; Fitness: " << cl_star->fitness << std::endl;

							// adds cl_star to combining set
							clCombSet.push_back(cl_star);
							combSetSize = clCombSet.size();

							insert_into_population(pop, cl_star);

							not_combined = 0;
							j=i;
						}
					}
				}
		} while (not_combined < 2);

	}

	if (MAX_DISP_RATE > 0) { // zero means no outlier detection
		for (auto& cl : pop)
			if (cl->disproves) {
				cl->disproving++;
				cl->disproves = false;
			}
		modified |= remove_outlier(pop);
	}

	return modified;
}

void print_pop(ClassifierSet pop, bool onlyExp) {
	//std::sort(pop.begin(), pop.end(), [](const ClassifierPtr& l, const ClassifierPtr& r) { return *l < *r; });

	std::cout << "No;Cond;Act;Pred;Fit;PredErr;Num;Exp" << std::endl;
	size_t j = 0;
	for (auto& cl : pop) {
		if (cl->experience > 0)
			std::cout << ++j << ";" << *cl << std::endl;
	}
	if (!onlyExp)
		for (auto& cl : pop) {
			if (cl->experience == 0)
				std::cout << ++j << ";" << *cl << std::endl;
		}
}
