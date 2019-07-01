#pragma once

#include <chrono>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include <optional.hpp>

#include "xcs_types.hpp"

using std::set;
using std::string;

using nonstd::optional;

void
set_num_of_actions(int num);

void
set_comb_period(int T_comb);

struct clexp { unsigned int cl_exp; unsigned int tot_exp; };

/// Returns number of experienced classifiers of a ClassifierSet
clexp
get_exp_classifiers(const ClassifierSet& set);

/// Returns all possible actions
set<Action>
all_actions();

/// Returns a string representation of an action
string
action_str(const Action& act);

/// Returns true if the condition rule matches on data
bool
elements_match(const vector<double>& elements, const vector<double>& input);

std::string
compose_cond(const vector<double>& elements);

/// Returns true if the conditions can match at least one point
bool
elements_overlap(const vector<double>& elements1, const vector<double>& elements2);

std::ostream&
operator<<(std::ostream& out, const Classifier& cl);

/// Calculates the total numerosity of a ClassifierSet
unsigned int
set_numerosity(const ClassifierSet &set);

/// Calculates the total fitness of a ClassifierSet
double
set_fitness(const ClassifierSet &set);

/// Inserts a classifier into the given population by either increasing
/// a matching classifiers numerosity or adding it itself.
void
insert_into_population(ClassifierSet& pop, const ClassifierPtr& cl);

/// Generates a match set by matching all classifiers to the Condition sigma
std::pair<ClassifierSet, bool>
generate_match_set(ClassifierSet& pop, const ActionSpace& as,
				   std::string origInput, const size_t& max_pop_size);

/// Returns the number of different actions of a set of classifiers
unsigned int
num_different_actions(const ClassifierSet& classifiers);
unsigned int
num_different_actions(const ClassifierSet& classifiers);

/// Returns the number of different actions of a set of rules
unsigned int
num_different_actions(const RuleSet& rules);

/// Generates a new classifier that covers the situation sigma
Classifier
generate_covering_classifier(ClassifierSet& match_set, const ActionSpace& as, const vector<double>& input);

/// Returns all rules from a set of classifiers
RuleSet
classifer_set_rules(const ClassifierSet& classifiers);
RuleSet
classifer_set_rules(const ClassifierSet& classifiers);

/// Returns all actions that are present in a given RuleSet
set<Action>
present_actions(const RuleSet& rules);

/// Returns the set difference lhs - rhs.
set<Action>
actions_diff(const set<Action>& lhs, const set<Action>& rhs);

/// Chooses a random action out of a given set of actions
///
/// Returns nullopt if the set is empty
optional<Action>
random_action(const set<Action>& actions);

/// Deletes classifiers from population, that are low in their fitness.
/// It also assures an approximately equal number of classifiers
/// in each action set.
/// You can maybe think of it as garbage collection to make sure
/// we only have the best classifiers, and we dont exceed our classifier
/// limit.
///
/// Returns true if it removed a classifier from the given population, false
/// if it left the population untouched
bool
delete_from_population(ClassifierSet& pop, const vector<double>& input);

/// Returns a deletion sore, based on fitness, numerosity, ...
double
get_del_prop(const Classifier& cl, const double avg_fitness);

/// Returns a prediction array as a map which assigns each action
/// out of the match_set a fitness value.
PredictionArray
generate_prediction_array(const ClassifierSet& match_set);

/// Selects an action either randomly when explore is set, or
/// an optimal one from the pa.
///
/// When choosing random actions it prefers actions that are not
/// present yet in the prediction array.
int
select_action(const PredictionArray& pa, ActionMode mode);

/// Returns all classifiers out of match_set that are using the
/// given action act.
ClassifierSet
generate_action_set(const ClassifierSet& match_set, const Action act);

/// Updates the fitness of the given action_set.
void
update_fitness(ClassifierSet& action_set);

/// Updates the given action_set.
/// Returns true if population was modified
bool
update_set(std::string origInput, const Action act, double reward, ClassifierSet& action_set, ClassifierSet& pop);

/// Returns true if cl_gen is more general than cl_spec
bool
is_more_general(const Classifier& cl_gen, const Classifier& cl_spc);

/// Deletes a given classifier from a ClassiferSet.
///
/// Returns true if deletion was sucessfull
bool
delete_classifier(ClassifierSet& clset, ClassifierPtr cl);

bool
is_subsumable(ClassifierPtr cl, ClassifierPtr subsumer);

/// Returns true if it has modified the population
bool
remove_outlier(ClassifierPtr& pop);

/// Returns true if it has modified the population
bool
combine_set(const ActionSpace& as, ClassifierSet& pop);

/// Print classifier set to the console
void
print_pop(ClassifierSet pop, bool onlyExp);
