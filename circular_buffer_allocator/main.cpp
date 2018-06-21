#include <iostream>
#include "profiler.hpp"

int main ( )
{
	constexpr auto num_tests = 1000;

	profiler my_profiler;

	std::cout << "Circular Buffer Allocator benchmark test\n";
	std::system ( "pause" );
	std::cout << '\n';

	for ( auto i = std::size_t { }; i < num_tests; ++i )
	{
		auto test = [ ]
		{

		};

		// Warmup
		test ( );

		// Beginning of profile
		my_profiler.start ( );
		test ( );
		my_profiler.end ( );
	}

	const auto my_profile = my_profiler.flush ( );
	std::cout << "Programmed with: "
#if defined _WIN64
		<< "[_WIN64]"
#endif
		<< '\n';
	std::cout << "Average: " << my_profile.mean << '\n';
	std::cout << "Standard deviation: " << my_profile.standard_deviation << '\n';
	std::cout << "Highest: " << my_profile.highest << '\n';
	std::cout << "Lowest: " << my_profile.lowest << '\n';
	std::cout << "Median: " << my_profile.median << '\n';
	std::system ( "pause" );
	return 0;
}
