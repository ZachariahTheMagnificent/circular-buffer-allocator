#include "circular_buffer_allocator.hpp"

namespace zachariahs_world
{
	namespace custom_allocators
	{
		std::mutex global_circular_buffer_allocator::my_mutex;
		std::unique_ptr<char [ ]> global_circular_buffer_allocator::buffer = std::make_unique<char [ ]> ( buffer_size );
		char* global_circular_buffer_allocator::unoccupied_begin = &buffer [ 0 ];
		char* global_circular_buffer_allocator::unoccupied_end = nullptr;
		global_circular_buffer_allocator::allocation_metadata_type* global_circular_buffer_allocator::back_allocation = nullptr;
	}
}
