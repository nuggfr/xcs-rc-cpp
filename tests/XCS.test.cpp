#include <xcs.hpp>
#include <utils.hpp>

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

TEST_CASE( "does_match", "[rule]" ) {
	Condition rule {One, Zero, DontCare, DontCare};

	// These should match
	Condition matches1 {One, Zero, One, One};
	Condition matches2 {One, Zero, Zero, Zero};
	Condition matches3 {One, Zero, Zero, One};
	Condition matches4 {One, Zero, One, Zero};

	// These should NOT match
	Condition nomatch1 {One, One , One, One};
	Condition nomatch2 {Zero, One , Zero, DontCare};

	REQUIRE(does_match(rule, matches1));
	REQUIRE(does_match(rule, matches2));
	REQUIRE(does_match(rule, matches3));
	REQUIRE(does_match(rule, matches4));

	REQUIRE_FALSE(does_match(rule, nomatch1));
	REQUIRE_FALSE(does_match(rule, nomatch2));
}

TEST_CASE( "num_different_actions", "[action]" ) {
	Rule cl1 { {One, Zero, DontCare} , 0 };
	Rule cl2 { {One, Zero, Zero}, 1};
	Rule cl3 { {DontCare, Zero, Zero}, 1};

	RuleSet set1 {cl1};
	RuleSet set2 {cl1, cl2};
	RuleSet set2_doubleaction {cl1, cl2, cl3};
	RuleSet set_empty { };

	REQUIRE(num_different_actions(set1) == 1);
	REQUIRE(num_different_actions(set2) == 2);
	REQUIRE(num_different_actions(set2_doubleaction) == 2);
	REQUIRE(num_different_actions(set_empty) == 0);

}

TEST_CASE( "generate_covering_classifier", "[classifier]" ) {
	ClassifierSet cs;
	Condition to_cover {One, One, Zero, Zero, One};

	Classifier covers = generate_covering_classifier(cs, to_cover);

	REQUIRE(does_match(covers.rule.condition, to_cover));
}

TEST_CASE( "deletion_vote", "[classifier]" ) {
	Classifier cl;

	cl.experience = 10;
	cl.fitness = 10;
	cl.numerosity = 2;
	cl.actionset_size = 10;
	auto score = deletion_vote(cl, 100);

	Classifier over_thresh = cl;
	over_thresh.experience = 30;
	over_thresh.fitness = 10;
	over_thresh.numerosity = 100;
	auto score_over_thresh = deletion_vote(over_thresh, 10);

	REQUIRE(score_over_thresh > score);

	// TODO: Probably add more tests
}

TEST_CASE( "no cleanup", "[classifier]" ) {
	ClassifierSet pop;
	Classifier cl;

	pop.push_back(cl);
	// this shouldnt do anything, as there is only one classifier which is < MAX_POPULATION
	REQUIRE(pop.size() < MAX_POPULATION);
	REQUIRE(pop.size() == 1);
	REQUIRE_FALSE(delete_from_population(pop));
	REQUIRE(pop.size() == 1);
}

TEST_CASE( "removing cleanup", "[classifier]" ) {
	ClassifierSet pop;
	Classifier cl;
	for (unsigned int i = 0; i < MAX_POPULATION+1; i++) {
		pop.push_back(cl);
	}
	auto size_before = pop.size();
	// Now delete_from_popluation has to cleanup
	REQUIRE(delete_from_population(pop));
	REQUIRE_FALSE(delete_from_population(pop));
	auto size_after = pop.size();
	REQUIRE(size_after < size_before);
	REQUIRE(size_after <= MAX_POPULATION);
}

TEST_CASE( "decreasing numerosity cleanup", "[classifier]" ) {
	ClassifierSet pop;
	Classifier cl;
	cl.numerosity = 3;
	for (unsigned int i = 0; i < MAX_POPULATION/2; i++) {
		pop.push_back(cl);
	}
	unsigned int num;
	// Cleanup, by repeatitly decrasign the numerosity of the classifiers
	while ((num = population_numerosity(pop)) > MAX_POPULATION) {
		auto size_before = population_numerosity(pop);
		REQUIRE(delete_from_population(pop));
		auto size_after = population_numerosity(pop);
		REQUIRE(size_after < size_before);
	}

	REQUIRE_FALSE(delete_from_population(pop));
	REQUIRE(population_numerosity(pop) <= MAX_POPULATION);
}

TEST_CASE( "single situation", "generate_match_set()" ) {
	Condition situation = {One, One, Zero, One};
	ClassifierSet population;
	ClassifierSet match_set = generate_match_set(population, situation);

	REQUIRE(match_set.size() > 1);
}

TEST_CASE( "test", "present_actions()" ) {
	RuleSet ruleset { {{One, One, Zero, DontCare}, 0} ,{{One, One, Zero, DontCare}, 1}};
	Condition situation = {One, One, Zero, One};
	auto actions = present_actions(ruleset);
	REQUIRE(actions.count(0) == 1);
	REQUIRE(actions.count(1) == 1);

	RuleSet ruleset2 { {{One, One, Zero, DontCare}, 0} };
	actions = present_actions(ruleset2);
	REQUIRE(actions.count(0) == 1);
	REQUIRE(actions.count(1) == 0);

	RuleSet redundant { {{One, One, Zero, DontCare}, 0}, {{Zero, One, Zero, DontCare}, 0}};
	actions = present_actions(redundant);
	REQUIRE(actions.count(0) == 1);
	REQUIRE(actions.count(1) == 0);
}

TEST_CASE ("right number of actions", "all_actions()") {
	//REQUIRE(all_actions().size() == 256);
}

TEST_CASE ("empty match set", "generate_prediction_array()") {
	ClassifierSet match_set; // emtpy match set
	auto pa = generate_prediction_array(match_set);
	REQUIRE(pa.size() == 0);
}

TEST_CASE ("small match set", "generate_prediction_array()") {
		ClassifierSet match_set;
		Classifier cl;
		cl.rule.action = 0;
		cl.fitness = 10;
		cl.prediction = 10;
		match_set.push_back(cl);
		match_set.push_back(cl);
		match_set.push_back(cl);
		auto pa = generate_prediction_array(match_set);
		REQUIRE(pa.size() == 1);
		cl.rule.action = 1;
		cl.fitness = 1;
		cl.prediction = 2;
		match_set.push_back(cl);
		match_set.push_back(cl);

		pa = generate_prediction_array(match_set);
		REQUIRE(pa.size() == 2);

		REQUIRE(pa[0] > pa[1]);
}

TEST_CASE ("match set filled using situation", "generate_prediction_array()") {
		Condition situation = {One, Zero, One, One};
		ClassifierSet pop;
		auto match_set = generate_match_set(pop, situation);

		REQUIRE(match_set.size() > 0);
		auto pa = generate_prediction_array(match_set);
		REQUIRE(pa.size() > 0);
		/*for (const auto& kv : pa) {
				printf("%s => %f\n", action_str(kv.first).c_str(), kv.second);
		}*/
}

TEST_CASE ("non empty set", "random_action()") {
	auto all = all_actions();
	REQUIRE(all.size() > 0);
	auto a = random_action(all);
	REQUIRE(a.has_value());
	REQUIRE(all.count(a.value()) > 0);
}

TEST_CASE ("empty set", "random_action()") {
	set<Action> actions;
	auto a = random_action(actions);
	REQUIRE_FALSE(a.has_value());
}

ClassifierSet test_match_set(size_t size) {
		ClassifierSet match_set;
		Classifier cl;
		cl.rule.action = 0;
		cl.fitness = 10;
		cl.prediction = 10;

		for (size_t i = 0; i < size; i++)
			match_set.push_back(cl);

		return match_set;
}

TEST_CASE ("exploration", "select_action()") {
	auto pa = generate_prediction_array(test_match_set(3));
	auto a = select_action(pa, true);
	REQUIRE(a != 0);
}

TEST_CASE ("exploitation", "select_action()") {
	auto ms = test_match_set(3);
	Classifier cl;
	cl.rule.action = 1;
	cl.prediction = 2;
	cl.fitness = 4;
	ms.push_back(cl);

	auto pa = generate_prediction_array(ms);
	auto a = select_action(pa, false);
	REQUIRE(a == 0);
}

TEST_CASE ("update_fitness()", "[update]") {
	auto match_set = test_match_set(3);
	const double fitness_before = 0;

	for (auto &cl : match_set) {
		cl.fitness = fitness_before;
		cl.numerosity = 100;
	}

	update_fitness(match_set);

	for (auto &cl : match_set) {
		REQUIRE(cl.fitness > fitness_before);
	}
}

