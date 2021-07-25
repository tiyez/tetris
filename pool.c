

#include "pool.h"
#include <stdlib.h> /* for malloc, free */
#include <string.h> /* for memset */

# define Invalid_Index ((size_t)-1)

struct pool_array_info_nonconst {
	size_t		element_size;
	size_t		max_elements;
	size_t		next_array;
	size_t		elements_count;
};

static struct pool_array_info_nonconst	*to_nonconst (struct pool_array_info *info) {
	return ((struct pool_array_info_nonconst *) info);
}

size_t		get_aligned_size (size_t size, size_t alignment) {
	return ((size + (alignment - 1)) & ~(alignment - 1));
}

size_t		get_pool_bucket_index_with_that_amount_of_available_space (struct pool *pool, size_t size) {
	size_t	index = Invalid_Index;

	if (!pool->buckets) {
		static const size_t start_amount = 32;

		pool->buckets = malloc (sizeof (struct pool_bucket) * start_amount);
		if (pool->buckets) {
			memset (pool->buckets, 0, sizeof (struct pool_bucket) * start_amount);
			pool->buckets_capacity = start_amount;
			pool->buckets_occupied = 0;
		}
	}
	if (pool->buckets) {
		index = 0;
		while (index < pool->buckets_occupied) {
			if ((Pool_Bucket_Capacity - pool->buckets[index].occupied) >= size) {
				break ;
			}
			index += 1;
		}
		if (index >= pool->buckets_occupied && size <= Pool_Bucket_Capacity) {
			if (pool->buckets_occupied >= pool->buckets_capacity) {
				struct pool_bucket	*new_buckets = realloc (pool->buckets, sizeof (struct pool_bucket) * pool->buckets_capacity * 2);
				if (new_buckets) {
					pool->buckets = new_buckets;
					pool->buckets_capacity *= 2;
				}
			}
			if (pool->buckets_occupied < pool->buckets_capacity) {
				struct pool_bucket	*bucket = &pool->buckets[pool->buckets_occupied];

				bucket->occupied = 0;
				bucket->data = malloc (Pool_Bucket_Capacity);
				if (bucket->data) {
					index = pool->buckets_occupied;
					pool->buckets_occupied += 1;
				}
			}
		}
		if (index >= pool->buckets_occupied) {
			index = Invalid_Index;
		}
	}
	return (index);
}

#include <stdio.h>
size_t					allocate_pool_element (struct pool *pool, size_t size) {
	size_t	element_index = Invalid_Index;
	size_t	bucket_index = get_pool_bucket_index_with_that_amount_of_available_space (pool, size);

	if (bucket_index != Invalid_Index) {
		struct pool_bucket	*bucket = &pool->buckets[bucket_index];

		element_index = bucket->occupied + (bucket_index << Pool_Bucket_Capacity_Log2);
		memset (bucket->data + bucket->occupied, 0, size);
		bucket->occupied += size;
	}
	return (element_index);
}

size_t					allocate_pool_array (struct pool *pool, size_t element_size, size_t max_elements) {
	size_t	array_index = Invalid_Index;
	size_t	bucket_index = get_pool_bucket_index_with_that_amount_of_available_space (pool, element_size * max_elements + sizeof (struct pool_array_info));

	if (bucket_index != Invalid_Index) {
		struct pool_bucket	*bucket = &pool->buckets[bucket_index];
		struct pool_array_info_nonconst	*info = (struct pool_array_info_nonconst *) (bucket->data + bucket->occupied);

		array_index = bucket->occupied + (bucket_index << Pool_Bucket_Capacity_Log2);
		memset (info + 1, 0, max_elements * element_size);
		bucket->occupied += max_elements * element_size + sizeof (struct pool_array_info);
		info->element_size = element_size;
		info->max_elements = max_elements;
		info->next_array = Invalid_Index;
		info->elements_count = 0;
	}
	return (array_index);
}

struct pool_array_info	*get_pool_array_info (struct pool *pool, size_t array_index) {
	size_t					bucket_index = array_index >> Pool_Bucket_Capacity_Log2;
	size_t					index = array_index & (Pool_Bucket_Capacity - 1);
	struct pool_bucket		*bucket;
	struct pool_array_info	*info = 0;

	if (array_index != Invalid_Index && bucket_index < pool->buckets_occupied) {
		bucket = &pool->buckets[bucket_index];
		info = (struct pool_array_info *) (bucket->data + index);
	}
	return (info);
}

struct pool_array_info	*get_next_pool_array_info (struct pool *pool, struct pool_array_info *info) {
	struct pool_array_info	*next;

	if (info && info->next_array != Invalid_Index) {
		next = get_pool_array_info (pool, info->next_array);
	} else {
		next = 0;
	}
	return (next);
}

struct pool_array_info	*get_last_pool_array_info (struct pool *pool, struct pool_array_info *info) {
	while (info && info->next_array != Invalid_Index) {
		info = get_pool_array_info (pool, info->next_array);
	}
	return (info);
}

struct pool_array_info	*get_last_nonempty_pool_array_info (struct pool *pool, struct pool_array_info *info) {
	struct pool_array_info	*prev = info;

	while (info && info->elements_count > 0) {
		prev = info;
		info = get_next_pool_array_info (pool, info);
	}
	return (prev);
}

size_t					get_pool_array_element_index (struct pool *pool, size_t array_index, void *element) {
	struct pool_array_info	*info = get_pool_array_info (pool, array_index);
	size_t					offset = 0;
	size_t					array_element_index = Invalid_Index;

	while (info && array_element_index == Invalid_Index) {
		if ((size_t) element >= (size_t) (info + 1) &&
			((size_t) element - (size_t) (info + 1)) < info->element_size * info->elements_count &&
			((size_t) element - (size_t) (info + 1)) % info->element_size == 0) {
			array_element_index = offset + ((size_t) element - (size_t) (info + 1)) / info->element_size;
		} else {
			offset += info->elements_count;
			info = get_next_pool_array_info (pool, info);
		}
	}
	return (array_element_index);
}

size_t					get_pool_array_elements_count (struct pool *pool, size_t array_index) {
	size_t	elements_count = 0;
	struct pool_array_info	*info = get_pool_array_info (pool, array_index);

	while (info) {
		elements_count += info->elements_count;
		info = get_next_pool_array_info (pool, info);
	}
	return (elements_count);
}

size_t					validate_pool_array_element_index (struct pool *pool, size_t array_index, size_t element_index) {
	struct pool_array_info	*info = get_pool_array_info (pool, array_index);

	while (info && element_index >= info->elements_count) {
		element_index -= info->elements_count;
		info = get_next_pool_array_info (pool, info);
	}
	return (!!info ? element_index : Invalid_Index);
}

void					*access_pool_element (struct pool *pool, size_t element_index) {
	size_t				bucket_index = element_index >> Pool_Bucket_Capacity_Log2;
	size_t				index = element_index & (Pool_Bucket_Capacity - 1);
	void				*element = 0;

	if (element_index != Invalid_Index) {
		element = pool->buckets[bucket_index].data + index;
	}
	return (element);
}

void					*access_pool_array (struct pool *pool, size_t array_index) {
	struct pool_array_info	*info = get_pool_array_info (pool, array_index);
	void					*array = 0;

	if (info) {
		array = info + 1;
	}
	return (array);
}

void					*access_pool_array_element (struct pool *pool, size_t array_index, size_t element_index) {
	struct pool_array_info	*info = get_pool_array_info (pool, array_index);
	void					*element = 0;

	if (info && element_index != Invalid_Index) {
		while (info && element_index >= info->elements_count) {
			element_index -= info->elements_count;
			info = get_next_pool_array_info (pool, info);
		}
		if (info) {
			element = (unsigned char *) (info + 1) + element_index * info->element_size;
		}
	}
	return (element);
}

void					*access_pool_array_last_element (struct pool *pool, size_t array_index) {
	struct pool_array_info	*info = get_last_nonempty_pool_array_info (pool, get_pool_array_info (pool, array_index));
	void	*result = 0;

	if (info && info->elements_count > 0) {
		result = (unsigned char *) (info + 1) + (info->elements_count - 1) * info->element_size;
	}
	return (result);
}

int						expand_pool_array_size (struct pool *pool, size_t array_index, size_t expand_size) {
	int						is_expanded = 0;

	if (array_index != Invalid_Index) {
		struct pool_array_info	*info = get_last_pool_array_info (pool, get_pool_array_info (pool, array_index));

		if (info) {
			size_t				bucket_index = array_index >> Pool_Bucket_Capacity_Log2;
			size_t				index = array_index & (Pool_Bucket_Capacity - 1);
			struct pool_bucket	*bucket;

			if (bucket_index < pool->buckets_occupied) {
				bucket = &pool->buckets[bucket_index];
				info = (void *) (bucket->data + index);
				if (bucket->occupied == index + (info->element_size * info->max_elements + sizeof (struct pool_array_info))) {
					if (Pool_Bucket_Capacity - bucket->occupied >= expand_size * info->element_size) {
						to_nonconst (info)->max_elements += expand_size;
						bucket->occupied += expand_size * info->element_size;
						is_expanded = 1;
					}
				}
			}
		}
	}
	return (is_expanded);
}

void					set_pool_array_element_size (struct pool_array_info *info, size_t elements_count) {
	elements_count = elements_count < info->max_elements ? elements_count : info->max_elements;
	to_nonconst (info)->elements_count = elements_count;
}

void					allocate_next_pool_array (struct pool *pool, struct pool_array_info *info) {
	size_t	array_index;

	array_index = allocate_pool_array (pool, info->element_size, info->max_elements);
	to_nonconst (info)->next_array = array_index;
}

size_t					append_pool_array_element (struct pool *pool, size_t array_index) {
	size_t					bucket_index = array_index >> Pool_Bucket_Capacity_Log2;
	size_t					index = array_index & (Pool_Bucket_Capacity - 1);
	struct pool_array_info	*info;
	size_t					element_index = Invalid_Index;

	if (array_index != Invalid_Index && index < pool->buckets[bucket_index].occupied) {
		info = get_last_pool_array_info (pool, get_pool_array_info (pool, array_index));
		if (info) {
			if (info->elements_count >= info->max_elements) {
				if (!expand_pool_array_size (pool, array_index, 1)) {
					if (info->max_elements > 0) {
						allocate_next_pool_array (pool, info);
						info = get_next_pool_array_info (pool, info);
					}
				}
			}
			if (info && info->elements_count < info->max_elements) {
				element_index = get_pool_array_elements_count (pool, array_index);
				to_nonconst (info)->elements_count += 1;
			}
		}
	}
	return (element_index);
}

void					deallocate_pool_array (struct pool *pool, size_t array_index) {
	size_t					bucket_index = array_index >> Pool_Bucket_Capacity_Log2;
	size_t					index = array_index & (Pool_Bucket_Capacity - 1);
	struct pool_array_info	*info;
	struct pool_bucket		*bucket;

	if (array_index != Invalid_Index) {
		bucket = &pool->buckets[bucket_index];
		info = (void *) (bucket->data + index);
		if (info->next_array != Invalid_Index) {
			deallocate_pool_array (pool, info->next_array);
			to_nonconst (info)->next_array = Invalid_Index;
		}
		if (bucket->occupied == (size_t) (info + 1) + info->max_elements * info->element_size) {
			bucket->occupied -= info->max_elements * info->element_size + sizeof *info;
		}
	}
}

void					deallocate_pool_array_last_element (struct pool *pool, size_t array_index) {
	struct pool_array_info	*info = get_pool_array_info (pool, array_index), *prev = info;

	while (info && info->next_array != Invalid_Index) {
		info = get_next_pool_array_info (pool, info);
		if (info->elements_count == 0) {
			deallocate_pool_array (pool, prev->next_array);
			to_nonconst (prev)->next_array = Invalid_Index;
			break ;
		}
		prev = info;
	}
	info = prev;
	if (info && info->elements_count > 0) {
		to_nonconst (info)->elements_count -= 1;
	}
}

void					deallocate_pool_array_last_elements (struct pool *pool, size_t array_index, size_t count) {
	/* todo: optimize */
	while (count > 0) {
		deallocate_pool_array_last_element (pool, array_index);
		count -= 1;
	}
}

size_t					get_pool_occupied_size (struct pool *pool) {
	size_t	size = 0;
	size_t	index = 0;

	while (index < pool->buckets_occupied) {
		size += pool->buckets[index].occupied;
		index += 1;
	}
	return (size);
}

void					free_pool_resources (struct pool *pool) {
	size_t	index = 0;

	while (index < pool->buckets_occupied) {
		free (pool->buckets[index].data);
		index += 1;
	}
	free (pool->buckets);
	pool->buckets = 0;
	pool->buckets_capacity = 0;
	pool->buckets_occupied = 0;
}



