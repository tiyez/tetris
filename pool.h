
#ifndef Pool_H
# define Pool_H

# include <stdlib.h> /* for size_t */

# define Pool_Bucket_Capacity_Log2 (13)
# define Pool_Bucket_Capacity (1 << Pool_Bucket_Capacity_Log2)

/* todo: alignment of allocation */
struct pool_bucket {
	size_t			occupied;
	unsigned char	*data;
};

struct pool {
	size_t				buckets_capacity;
	size_t				buckets_occupied;
	struct pool_bucket	*buckets;
};

struct pool_array_info {
	const size_t	element_size;
	const size_t	max_elements;
	const size_t	next_array;
	const size_t	elements_count;
};

size_t					get_aligned_size (size_t size, size_t alignment);

size_t					allocate_pool_element (struct pool *, size_t size);
size_t					allocate_pool_array (struct pool *, size_t element_size, size_t max_elements);
struct pool_array_info	*get_pool_array_info (struct pool *, size_t array_index);
struct pool_array_info	*get_next_pool_array_info (struct pool *, struct pool_array_info *);
size_t					get_pool_array_element_index (struct pool *, size_t array, void *element);
size_t					get_pool_array_elements_count (struct pool *pool, size_t array_index);
size_t					validate_pool_array_element_index (struct pool *pool, size_t array_index, size_t element_index);
void					*access_pool_element (struct pool *, size_t element_index);
void					*access_pool_array (struct pool *pool, size_t array_index);
void					*access_pool_array_element (struct pool *, size_t array_index, size_t element_index);
void					*access_pool_array_last_element (struct pool *, size_t array_index);
int						expand_pool_array_size (struct pool *, size_t array_index, size_t expand_size);
void					set_pool_array_element_size (struct pool_array_info *, size_t elements_count);
void					allocate_next_pool_array (struct pool *, struct pool_array_info *);
size_t					append_pool_array_element (struct pool *, size_t array_index);

void					deallocate_pool_array (struct pool *, size_t array_index);
void					deallocate_pool_array_last_element (struct pool *pool, size_t array_index);
void					deallocate_pool_array_last_elements (struct pool *pool, size_t array_index, size_t count);

size_t					get_pool_occupied_size (struct pool *);

void					free_pool_resources (struct pool *);

#endif /* Pool_H */
