#include <iostream>
#include <vector>
#include <random>
#include "profiler.hpp"
#define USE_CUSTOM_ALLOCATOR
//#define RESERVE_MEMORY_AHEAD_OF_TIME

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

template<typename type>
using array_type =
#if defined USE_CUSTOM_ALLOCATOR
std::vector<type, circular_buffer_allocator_type<type>>;
#else
std::vector<type>;
#endif

template<typename type>
array_type<type> operator+ ( const array_type<type>& lower, const array_type<type>& upper )
{
	array_type<type> combined;
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
	combined.reserve ( lower.size ( ) + upper.size ( ) );
#endif

	for ( const auto& element : lower )
	{
		combined.push_back ( element );
	}
	for ( const auto& element : upper )
	{
		combined.push_back ( element );
	}

	return combined;
}

template<typename type>
array_type<type> operator+ ( const array_type<type>& array, const type& back )
{
	array_type<type> combined;
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
	combined.reserve ( array.size ( ) + 1 );
#endif

	for ( const auto& element : array )
	{
		combined.push_back ( element );
	}

	combined.push_back ( back );

	return combined;
}
template<typename type>
array_type<type> operator+ ( const type& front, const array_type<type>& array )
{
	array_type<type> combined;
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
	combined.reserve ( array.size ( ) + 1 );
#endif

	combined.push_back ( front );

	for ( const auto& element : array )
	{
		combined.push_back ( element );
	}
}

template<typename type>
array_type<type> quick_sort ( const array_type<type>& array )
{
	if ( array.size ( ) < 2 )
	{
		return array;
	}

	array_type<type> lower;
	array_type<type> upper;
	const type middle = array.front ( );

#if defined RESERVE_MEMORY_AHEAD_OF_TIME
	lower.reserve ( array.size ( ) );
	upper.reserve ( array.size ( ) );
#endif

	for ( auto i = std::size_t { } + 1, size = array.size ( ); i < size; ++i )
	{
		if ( array [ i ] < middle )
		{
			lower.push_back ( array [ i ] );
		}
		else
		{
			upper.push_back ( array [ i ] );
		}
	}

	return quick_sort ( lower ) + middle + quick_sort ( upper );
}

int main ( )
{
	constexpr auto num_tests = 1000;
	constexpr auto num_ints = 1000;
	constexpr auto num_frames = 100;
	constexpr auto lowest_int = int { 12 };
	constexpr auto highest_int = int { 758 };

	auto my_profiler = profiler_type { };

	std::cout << "Circular Buffer Allocator benchmark test\n";
	std::system ( "pause" );
	std::cout << '\n';

	auto test = [ lowest_int, highest_int, num_ints, num_frames ]
	{
		auto rng_machine = std::mt19937 { };
		auto int_generator = std::uniform_int_distribution<int> { lowest_int, highest_int };
		auto ints = array_type<int> { };

		for ( auto i = std::size_t { }; i < num_frames; ++i )
		{
			ints = [ &rng_machine, &int_generator, &ints, num_ints ]
			{
				auto random_ints = array_type<int> { };
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
				random_ints.reserve ( num_ints );
#endif

				for ( auto i = std::size_t { }; i < num_ints; ++i )
				{
					random_ints.push_back ( int_generator ( rng_machine ) );
				}

				return quick_sort ( random_ints );
			} ( );
		}
	};

	for ( auto i = std::size_t { }; i < num_tests; ++i )
	{
		// Warmup
		test ( );

		// Beginning of profile
		my_profiler.start ( );
		test ( );
		my_profiler.end ( );

		std::cout << "Test " << i << '/' << num_tests << " done!\n";
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
	std::cout << "Average: " << my_profile.mean << "ns\n";
	std::cout << "Standard deviation: " << my_profile.standard_deviation << "ns\n";
	std::cout << "Highest: " << my_profile.highest << "ns\n";
	std::cout << "Lowest: " << my_profile.lowest << "ns\n";
	std::cout << "Median: " << my_profile.median << "ns\n";
	std::system ( "pause" );
	return 0;
}
