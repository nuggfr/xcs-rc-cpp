#pragma once

/* Utils.cpp */
double
random_number(const double min, const double max);
unsigned
random_uint(const unsigned int min, const unsigned max);

/// Returns true with a probability of the parameter prob_true,
/// false otherwise.
bool
random_choice(const double& prob_true);
