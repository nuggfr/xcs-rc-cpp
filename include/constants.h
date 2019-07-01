#pragma once

#include <ctime>

/// Constants/Parameters

/// Discount factor used in multi-step problems and to update
/// classifier predictions.
const double GAMMA = 0.71;

/// Probability, that a covering classifiers condition
/// gets a don't care assigned.
const double P_DONTCARE = 0.1;

// The intialization paremeters should be taken very small - esentially zero.
const double PREDICTION_INIT = 500.0;
const double PREDICTION_ERROR_INIT = 0;
const double FITNESS_INIT = 10.0;

/// If experience is greater than this threshold, it may be considered
/// for deletion
const double THETA_DEL = 25;

/// Fraction of mean fitness in population below which fitness
/// of a classifier maybe considered in its probability of deletion
const double DELTA_DELETION = 0.1;

/// Maximum size of population, which is the sum of all
/// numerosities of the classifers
const unsigned MAX_POP_SIZE = 2000;

/// Probability of which an random action for exploration is chosen.
const double PROBABILITY_EXPLORE = 0.5;

// Paremeters for fitness update
const double REWARD_MAX = 1000;
const double ALPHA = 0.1;
const double BETA = 0.15;
const double EPSILON_ZERO = 0.01;

/// Used for fitness update.
const double POWER_PARAMETER = 5.0;

// Larger values may be better for some problems.
const unsigned SUBSUMPTION_THRESHOLD = 50;

// COMBINING
const int MIN_EXP = 1;
const int MAX_DISP_RATE = 2;
const double PRED_TOL = 10;
const double PRED_ERR_TOL = 260;

