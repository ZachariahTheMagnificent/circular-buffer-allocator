#include <iostream>
#include <vector>
#include "profiler.hpp"
#define USE_CUSTOM_ALLOCATOR
#define RESERVE_MEMORY_AHEAD_OF_TIME

#if defined USE_CUSTOM_ALLOCATOR
#include "circular_buffer_allocator.hpp"
using zachariahs_world::custom_allocators::circular_buffer_allocator_type;

inline auto allocator1 = circular_buffer_allocator_type<char> { };
inline auto allocator2 = circular_buffer_allocator_type<int> { };
inline auto allocator3 = circular_buffer_allocator_type<float> { };
constexpr auto test1 = allocator1 != allocator2;
constexpr auto test3 = allocator2 != allocator1;
constexpr auto test2 = allocator2 != allocator3;
constexpr auto test4 = allocator1 == allocator2;
constexpr auto test5 = allocator2 == allocator1;
constexpr auto test6 = allocator2 == allocator3;
constexpr auto test7 = decltype ( allocator2 ) { allocator1 };
constexpr auto test8 = decltype ( allocator1 ) { allocator2 };
constexpr auto test9 = decltype ( allocator2 ) { allocator3 };
constexpr auto test10 = decltype ( allocator2 ) { std::move ( allocator1 ) };
constexpr auto test11 = decltype ( allocator1 ) { std::move ( allocator2 ) };
constexpr auto test12 = decltype ( allocator2 ) { std::move ( allocator3 ) };
constexpr auto test13 = allocator1.alignment;
constexpr auto test14 = allocator2.alignment;
constexpr auto test15 = allocator3.alignment;
constexpr auto test16 = [ ]
{
	allocator1 = allocator2;
	allocator2 = allocator1;
	allocator3 = allocator2;
	allocator1 = std::move ( allocator2 );
	allocator2 = std::move ( allocator1 );
	allocator3 = std::move ( allocator2 );
	return 0;
} ( );
#endif

array function ( )
{
	// Array1 gets added to the stack.
	auto array1 = array { };
	// Array2 gets added to the stack.
	auto array2 = array { };

	// Array3 gets created by combining array1 and array2 together
	auto array3 = array1 + array2;

	// Array3's lifespan gets extended beyond function ( )
	return std::move ( array3 );
}
// Array1 and 2's destruction gets blocked by array3

template<typename type>
using array_type =
#if defined USE_CUSTOM_ALLOCATOR
std::vector<type, circular_buffer_allocator_type<type>>;
#else
std::vector<type>;
#endif

int main ( )
{
	constexpr auto num_tests = 1000;
	constexpr auto num_ints = 10'000;
	constexpr auto num_frames = 1000;

	auto my_profiler = profiler_type { };

	std::cout << "Circular Buffer Allocator benchmark test\n";
	std::system ( "pause" );
	std::cout << '\n';

	auto test = [ num_ints, num_frames ]
	{
		auto ints = array_type<int> { };
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
		ints.reserve ( num_ints );
#endif

		for ( auto i = int { }; i < num_ints; ++i )
		{
			ints.push_back ( i );
		}

		for ( auto i = std::size_t { }; i < num_frames; ++i )
		{
			ints = [ &ints ]
			{
				auto results = array_type<int> { };
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
				results.reserve ( num_ints );
#endif

				for ( const auto integer : ints )
				{
					results.push_back ( integer + 1 );
				}

				return results;
			} ( );
		}
	};

	// Warmup
	test ( );

	for ( auto i = std::size_t { }; i < num_tests; ++i )
	{
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
#if defined USE_CUSTOM_ALLOCATOR
		<< "[USE_CUSTOM_ALLOCATOR]"
#endif
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
		<< "[RESERVE_MEMORY_AHEAD_OF_TIME]"
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
