#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>

#include "constants.h"

using std::vector;
using std::string;
using std::unordered_map;
using std::shared_ptr;

using Action = uint8_t;
using ActionSpace = std::set<Action>;

enum class ActionMode {
	Explore,
	Exploit,
};

/// TODO: Clarify
/// A rule consits of a condition, that must implement
/// the does_match function and an action or class that can
/// be used in a specific environment or classification problem.
struct Rule {
	vector<double> elements; // elements == condition
	size_t act;

	bool operator==(const Rule& rhs) const { return elements == rhs.elements && act == rhs.act; }
	bool operator!=(const Rule& rhs) const { return !(*this == rhs); }
};
using RuleSet = vector<Rule>;

/// The XCS classifier represents knowledge about a problem
/// using a condition-action-prediction rule.
///
/// @see Rule
class Classifier {
  public:
	Rule rule = {.elements = {}, .act = 0};

	string cond = "";
	/// The prediction 'p' estimates(keeps an average of) the payoff
	/// exptected if the classifier matches and its action is taken
	// by the system.
	double prediction = PREDICTION_INIT;

	/// The prediction error 'ε' estimates the errors made in the
	/// predictions.
	double prediction_error = PREDICTION_ERROR_INIT;

	/// The fitness 'F' denotes the classifiers fitness.
	double fitness = FITNESS_INIT;

	/// The experience 'exp' counts the number of times since the creation
	/// that the classifier has belonged to an action set.
	unsigned int experience = 0;

	/// The action set size 'as' estimates the average size of the action
	/// sets this classifier has belonged to.
	double actionset_size = 1;

	/// The numerosity 'n' reflects the number of
	/// micro-classifiers(ordinary classifiers) this classifier
	/// - which is technically called a macro-classifier - represents.
	unsigned int numerosity = 1;

	unsigned int disproving = 0;
	bool disproves = false;

	bool operator==(const Classifier& rhs) const {
		return rule == rhs.rule && prediction == rhs.prediction && prediction_error == rhs.prediction_error &&
		       fitness == rhs.fitness && experience == rhs.experience && numerosity == rhs.numerosity;
	}

	// Compare by action, then condition, then prediction
	bool operator<(const Classifier& rhs) const {
		if (rule.act != rhs.rule.act) {
			return rule.act < rhs.rule.act;
		}
		// assert(rule.act == rhs.rule.act);

		auto cond_ord = [](const string& lhs, const string& rhs) {
			auto cond_to_num = [](const string& cond) {
				size_t num = 0;
				const size_t sz = cond.size() - 1;
				size_t n = 0;
				for (int i = sz; i >= 0; i--, n++) {
					switch (cond[i]) {
					case '0':
						break;
					case '1':
						num += std::pow(2, n);
						break;
					case '#':
						num += std::pow(2, n) + 1; // TODO: Is this right?
						break;
					default:
						break;
					}
				}
				return num;
			};
			return cond_to_num(lhs) < cond_to_num(rhs);
		};

		if (prediction != rhs.prediction) {
			return prediction > rhs.prediction;
		}

		return cond_ord(cond, rhs.cond);
	}
};

using ClassifierPtr = shared_ptr<Classifier>;
using ClassifierSet = vector<ClassifierPtr>;

/// TODO: Think about different Action types like integers
using PredictionArray = unordered_map<Action, double, std::hash<Action>>;
