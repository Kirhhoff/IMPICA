/*
 * LLUBENCHMARK 
 * Craig Zilles (zilles@cs.wisc.edu)
 * http://www.cs.wisc.edu/~zilles/llubenchmark.html
 *
 * This program is a linked list traversal micro-benchmark, which can
 * be used (among other things) to approximate the non-benchmark
 * Health.
 * 
 * The benchmark executes for a proscribed number of iterations (-i),
 * and on every iteration the lists are traversed and potentially
 * extended.  The number of lists can be specified (-n) as well as the
 * size of the elements in the list (-s).  The initial length of the
 * lists can be set (-l) as well as the growth rate (-g).  The growth
 * rate must be non-negative, but can be a floating point number, in
 * which case random numbers are used to determine whether a list is
 * extended on a particular cycle (all lists are extended
 * independently).  If the -t option is specified, the insertion
 * occurs at the tail, otherwise at the head.  If the -d option is
 * specified, the elements are dirtied during the traversal (which
 * will necessitate a write-back when the data is evicted from the
 * cache).
 *
 * To approximate the non-benchmark Health, use the options:
 *     -i <num iterations> -g .333 -d -t -n 341
 * 
 * (the growth rate of the lists in health is different for different
 * levels of the hierarchy and the constant .333 is just my
 * approximation of the growth rate).
 *  
 */

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include "m5op.h"

// #define PIM

#ifdef PIM

#include "pim_list_node.hpp"	
#include "pim_ops.hpp"

typedef pim_list_node<int> element;

static inline void set_next(element* e, element* next) {
	e->next(next);
}

static inline void traverse_list(element* list_first_element) {
	element* result = pim_find(list_first_element, (int)0xFFFFFFFF);
	if (result != nullptr)
		printf("found at %p val = %d\n", result, result->data());
	// assert(pim_find(list_first_element, 1) == nullptr);
} 

#else

typedef struct _element {
  	_element *next;
  	int count;
} element;

inline void set_next(element* e, element* next) {
	e->next = next;
}

inline void traverse_list(element* list_first_element) {
	
	int dummpy_count_sum = 0;
	element* cur_traverse_element = list_first_element;
	
	while (cur_traverse_element != NULL) {
		dummpy_count_sum += cur_traverse_element->count;
		cur_traverse_element = cur_traverse_element->next;
	}	
} 

#endif

void usage(char *name) {
  	printf("%s:\n", name);
  	printf("-i <number of (I)terations>\n");
  	printf("[-l <initial (L)ength of list, in elements>] (default 1)\n");
  	printf("[-n <(N)umber of lists>] (default 1 list)\n");
  	printf("[-s <(S)ize of element>] (default 32 bytes)\n");
  	printf("[-g <(G)rowth rate per list, in elements per iteration>] (default 0)\n");
  	printf("[-d] ((D)irty each element during traversal, default off)\n");
  	printf("[-t] (insert at (T)ail of list, default off)\n");
}

constexpr int ALLOC_SIZE = 10230; /* pick wierd num to break strides */
element *free_list = NULL;
int next_free = ALLOC_SIZE;
int element_size = sizeof(element) > 64 ? sizeof(element) : 64;
int num_allocated = 0;

element* allocate() {
	if (next_free == ALLOC_SIZE) {
		next_free = 0;

		uint64_t mem_pool_start = reinterpret_cast<uint64_t>(malloc((ALLOC_SIZE + 1) * element_size));	
		assert(mem_pool_start != 0);
	 	free_list = reinterpret_cast<element*>(((mem_pool_start + 7) >> 3) << 3);
	 	memset(free_list, ALLOC_SIZE * element_size, 0);
	}
  
  	element* element_ptr_allocated = reinterpret_cast<element*>(reinterpret_cast<uint64_t>(free_list) + next_free * element_size);
  	num_allocated++;
  	next_free++;
#ifdef PIM

  	return ::new(element_ptr_allocated) element;

#else

	return element_ptr_allocated;

#endif
}

int dirty = 0;
int max_traverse_repeat_itrs = 0;
int num_lists = 1;
int tail = 0;
int initial_length = 1;
float growth_rate = 0.0;
float growth = 0.0;

int parse_cmd_args(int argc, char* argv[]) {
  	char c = 0;
  	int arg = 1;
  	while (arg < argc) {
	 	if ((argv[arg][0] != '-') || (argv[arg][2] != 0)) {
			printf("parse error in %s\n", argv[arg]);
			usage(argv[0]);
			return -1;
	 	}
	 	c = argv[arg][1];
	 	arg ++;
	 	switch(c) {
	 	case 'd': 		dirty = 1; break;
	 	case 'g': 		growth_rate = atof(argv[arg++]);  break;
	 	case 'i': 		max_traverse_repeat_itrs = atoi(argv[arg++]); break;
	 	case 'l': 		initial_length = atoi(argv[arg++]); break;
	 	case 'n': 		num_lists = atoi(argv[arg++]); break;
	 	case 's': 		element_size = atoi(argv[arg++]); break;
	 	case 't': 		tail = 1; break;
	 	default:
			printf("unrecognized option: %c\n", c);
			usage(argv[0]);
		return -1;
		}
	}

	assert(initial_length > 0);

  	return 0;
}

element** build_benchmark_lists() {
	printf("[LOG:] begin build lists\n");
  	
	element** lists = NULL;
	size_t allocate_size = num_lists * sizeof(element*);
	lists = reinterpret_cast<element**>(malloc(allocate_size));
  	assert(lists != nullptr);  	
	memset(lists, allocate_size, 0);  	
	
	int milestone = 0;
  	for (int cur_list_length = 0; cur_list_length < initial_length; cur_list_length++){
		milestone++;
		if (milestone >= initial_length / 10) {
			milestone = 0;
			printf("[LOG:] finished %d/10\n", cur_list_length / (initial_length / 10));
		}	  
		for (int inc_list_idx = 0; inc_list_idx < num_lists; inc_list_idx++) { 
			auto& inc_list_first_element = lists[inc_list_idx];
			element *inc_list_new_first_element = allocate();
			set_next(inc_list_new_first_element, inc_list_first_element);
			inc_list_first_element = inc_list_new_first_element;
	 	}
  	}

	printf("[LOG:] end build lists\n");

	return lists;
}

int main(int argc, char *argv[]) {

	if (parse_cmd_args(argc, argv) < 0){
		return -1;
	}
		
	element** lists = build_benchmark_lists();

	// m5_checkpoint(0, 0);

	m5_reset_stats(0, 0);

  	/* iterate */
  	for (int traverse_repeat_itr = 0; traverse_repeat_itr < max_traverse_repeat_itrs; traverse_repeat_itr++) { 
	 	if ((traverse_repeat_itr % 10) == 0) {
			printf("%d\n", traverse_repeat_itr);
	 	}
	 	/* traverse lists */
	 	for (int traverse_list_idx = 0; traverse_list_idx < num_lists; traverse_list_idx++) {
		 	traverse_list(lists[traverse_list_idx]);
	 	}
  	}
	printf("finished\n");

	m5_dump_stats(0, 0);
}
	 
