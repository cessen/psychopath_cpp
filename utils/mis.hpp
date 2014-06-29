#ifndef MIS_HPP
#define MIS_HPP

// Utility functions for multiple importance sampling

template <typename T>
T balance_heuristic(T a, T b)
{
	return a / (a + b);
}

template <typename T>
T power_heuristic(T a, T b)
{
	const auto a2 = a * a;
	const auto b2 = b * b;
	return a2 / (a2 + b2);
}

#endif // MIS_HPP