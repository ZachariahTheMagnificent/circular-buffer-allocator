#pragma once
#include <memory>
#include <mutex>
#include <cassert>

namespace zachariahs_world
{
	namespace custom_allocators
	{
		class global_circular_buffer_allocator
		{
		public:
			struct alignas ( std::max_align_t ) allocation_metadata_type
			{
				allocation_metadata_type* previous_allocation { };
				allocation_metadata_type* next_allocation { };

				allocation_metadata_type ( ) = default;
				constexpr explicit allocation_metadata_type ( allocation_metadata_type*const previous_allocation ) noexcept : previous_allocation { previous_allocation }
				{

				}
			};

			static char* allocate ( const std::size_t size, const std::size_t alignment )
			{
				assert ( alignment >= alignof ( std::max_align_t ) );

				std::lock_guard guard { my_mutex };

				// If we are full
				if ( unoccupied_begin == unoccupied_end )
				{
					throw std::bad_alloc { };
				}

				// If the end is behind us, use end of buffer.
				if ( unoccupied_end < unoccupied_begin )
				{
					{
						const auto [ metadata_pointer, data_pointer ] = get_pointers_for_allocation ( unoccupied_begin, alignment );

						if ( data_in_bounds ( data_pointer, size, &buffer [ buffer_size ] ) )
						{
							back_allocation = new ( metadata_pointer ) allocation_metadata_type { back_allocation };
							return data_pointer;
						}
					}

					// Otherwise, go back to the beginning of the buffer.
					{
						// If there is no space at the beginning of the buffer.
						if ( unoccupied_end == nullptr )
						{
							throw std::bad_alloc { };
						}

						const auto [ metadata_pointer, data_pointer ] = get_pointers_for_allocation ( &buffer [ 0 ], alignment );

						if ( !data_in_bounds ( data_pointer, size, unoccupied_end ) )
						{
							throw std::bad_alloc { };
						}

						back_allocation = new ( metadata_pointer ) allocation_metadata_type { back_allocation };
						unoccupied_begin = &buffer [ 0 ];
						return data_pointer;
					}
				}

				const auto [ metadata_pointer, data_pointer ] = get_pointers_for_allocation ( unoccupied_begin, alignment );

				if ( !data_in_bounds ( data_pointer, size, unoccupied_end ) )
				{
					throw std::bad_alloc { };
				}

				back_allocation = new ( metadata_pointer ) allocation_metadata_type { back_allocation };
				return data_pointer;
			}

			static void deallocate ( char*const allocation_to_deallocate, const std::size_t size, const std::size_t alignment ) noexcept
			{
				assert ( alignment >= alignof ( std::max_align_t ) );
				
				const auto pointer_to_allocation_metadata = align_pointer_by_decrement ( allocation_to_deallocate - sizeof ( allocation_metadata_type ), alignment );
				auto allocation_metadata = allocation_metadata_type { };

				std::lock_guard guard { my_mutex };

				// Fetch the metadata from the buffer.
				std::memcpy ( &allocation_metadata, pointer_to_allocation_metadata, sizeof ( allocation_metadata ) );

				if ( allocation_metadata.previous_allocation == nullptr )
				{
					// If we are the only allocation left.
					if ( allocation_metadata.next_allocation == nullptr )
					{
						//Reset everything
						unoccupied_begin = &buffer [ 0 ];
						unoccupied_end = nullptr;
						back_allocation = nullptr;
					}
					// If we are at the front.
					else
					{
						// Move unoccupied_end in front of this allocation since it is unoccupied.
						unoccupied_end = allocation_to_deallocate + size;

						// The next allocation is the new front.
						allocation_metadata.next_allocation->previous_allocation = nullptr;
					}
				}
				// If we are at the back.
				else if ( allocation_metadata.next_allocation == nullptr )
				{
					// Move unoccupied_begin back to this allocation since it is unoccupied.
					unoccupied_begin = pointer_to_allocation_metadata;

					// The previous allocation is the new back.
					back_allocation = allocation_metadata.previous_allocation;
					allocation_metadata.previous_allocation->next_allocation = nullptr;
				}
				// If we are in the middle.
				else
				{
					allocation_metadata.previous_allocation->next_allocation = allocation_metadata.next_allocation;
					allocation_metadata.next_allocation->previous_allocation = allocation_metadata.previous_allocation;
				}
			}

		private:
			static constexpr auto buffer_size = 1000;

			static inline std::mutex my_mutex;
			static inline std::unique_ptr<char [ ]> buffer = std::make_unique<char [ ]> ( buffer_size );
			static inline char* unoccupied_begin = &buffer [ 0 ];
			static inline char* unoccupied_end = nullptr;
			static inline allocation_metadata_type* back_allocation = nullptr;

			static constexpr char* align_pointer_by_increment ( char*const pointer, const std::size_t alignment ) noexcept
			{
				const auto pointer_as_integer = reinterpret_cast<std::size_t> ( pointer );
				const auto aligned_pointer_as_integer = ( ( pointer_as_integer + alignment - 1 ) / alignment ) * alignment;

				return reinterpret_cast<char*> ( aligned_pointer_as_integer );
			}
			static constexpr char* align_pointer_by_decrement ( char*const pointer, const std::size_t alignment ) noexcept
			{
				const auto pointer_as_integer = reinterpret_cast<std::size_t> ( pointer );
				const auto aligned_pointer_as_integer = ( pointer_as_integer / alignment ) * alignment;

				return reinterpret_cast<char*> ( aligned_pointer_as_integer );
			}

			static constexpr auto get_pointers_for_allocation ( char*const begin, std::size_t alignment ) noexcept -> decltype ( std::tuple { begin, begin } )
			{
				const auto metadata_pointer = align_pointer_by_increment ( begin, alignment );
				const auto data_pointer = align_pointer_by_increment ( metadata_pointer + sizeof ( allocation_metadata_type ), alignment );

				return std::tuple { metadata_pointer, data_pointer };
			}
			static constexpr bool data_in_bounds ( char*const data_pointer, std::size_t size, char*const end )
			{
				if ( data_pointer > end )
				{
					return false;
				}

				const auto space_left = reinterpret_cast<std::size_t> ( end ) - reinterpret_cast<std::size_t> ( data_pointer );

				return space_left < size;
			}
		};

		template<typename type>
		class circular_buffer_allocator_type
		{
		public:
			using value_type = type;

			template<typename a, typename b>
			using strictest_alignment_type = std::conditional_t<( alignof ( a ) > alignof ( b ) ), a, b>;
			using best_alignment_type_for_allocation = strictest_alignment_type<value_type, std::max_align_t>;

			static constexpr auto alignment = alignof ( best_alignment_type_for_allocation );

			circular_buffer_allocator_type ( ) = default;
			template<typename other>
			constexpr circular_buffer_allocator_type ( const circular_buffer_allocator_type<other>& ) noexcept
			{
			}
			template<typename other>
			constexpr circular_buffer_allocator_type ( circular_buffer_allocator_type<other>&& ) noexcept
			{
			}

			template<typename other>
			constexpr circular_buffer_allocator_type& operator= ( const circular_buffer_allocator_type<other>& ) noexcept
			{
				return *this;
			}
			template<typename other>
			constexpr circular_buffer_allocator_type& operator= ( circular_buffer_allocator_type<other>&& ) noexcept
			{
				return *this;
			}

			template<typename other>
			constexpr bool operator== ( const circular_buffer_allocator_type<other>& rhs ) const noexcept
			{
				return true;
			}
			template<typename other>
			constexpr bool operator!= ( const circular_buffer_allocator_type<other>& rhs ) const noexcept
			{
				return false;
			}

			static type* allocate ( const std::size_t size )
			{
				return reinterpret_cast<type*> ( global_circular_buffer_allocator::allocate ( size, alignment ) );
			}
			static void deallocate ( type*const allocation_to_deallocate, const std::size_t size ) noexcept
			{
				global_circular_buffer_allocator::deallocate ( reinterpret_cast<char*> ( allocation_to_deallocate ), size, alignment );
			}
		};

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
	}
}
