/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: everonel <everonel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 00:00:00 by emma              #+#    #+#             */
/*   Updated: 2026/02/19 22:46:04 by everonel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

/*
** Global variables (allowed: one for allocations, one for mutex)
*/
t_zone *g_zones = NULL;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void	*malloc(size_t size) {
	if (size == 0 || size > SIZE_MAX - ALIGNMENT) return (NULL);
	t_zone_type zone_type;
	t_zone 		*zone;
	t_block 	*block;

	pthread_mutex_lock(&g_mutex);
	size = align_size(size);
	zone_type = (size <= TINY_MAX) ? TINY : (size <= SMALL_MAX) ? SMALL : LARGE;
	zone = find_or_create_zone(zone_type, size);
	if (!zone) {
		pthread_mutex_unlock(&g_mutex); 
		return (NULL);
	}
	
	if (zone_type == LARGE) block = zone->blocks;
	else block = find_suitable_block(zone, size);
	if (!block) {
		pthread_mutex_unlock(&g_mutex); 
		return (NULL);
	}
	
	if (zone_type != LARGE) split_block(block, size);
	block->free = 0;
	pthread_mutex_unlock(&g_mutex);
	return ((void*)((char *)block + sizeof(t_block)));
}

size_t align_size(size_t size) {
	return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

t_zone *find_or_create_zone(t_zone_type type, size_t size) {
	if (type == LARGE) return create_zone(type, size); 
	t_zone 	*current = g_zones;
	t_block *block;

	while (current) {
		if (current->type == type) {
			block = find_suitable_block(current, size);
			if (block) return (current);
		}
		current = current->next;
	}
	return (create_zone(type, size));
}

t_block *find_suitable_block(t_zone *zone, size_t size) {
	t_block *current = zone->blocks;
	while (current) {
		if (current->free && current->size >= size) return (current);
		current = current->next;
	}
	return (NULL);
}

t_zone *create_zone(t_zone_type type, size_t size) {
	size_t 	page_size = sysconf(_SC_PAGESIZE);
	if (type == LARGE && size > SIZE_MAX - sizeof(t_zone) - sizeof(t_block) - ALIGNMENT - page_size)
		return (NULL);
    size_t 	zone_size = (type == TINY) ? TINY_ZONE_SIZE : (type == SMALL) ? SMALL_ZONE_SIZE : ((size + sizeof(t_zone) + sizeof(t_block) + page_size - 1) / page_size) * page_size;
    void 	*zone_start = mmap(NULL, zone_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (zone_start == MAP_FAILED) return (NULL);

	t_zone 		*new_zone = (t_zone*)zone_start;
	uintptr_t 	blocks_addr = (uintptr_t)(zone_start + sizeof(t_zone));
	new_zone->type = type;
	new_zone->size = zone_size;
	new_zone->start = zone_start;
	blocks_addr = (blocks_addr + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
	new_zone->blocks = (t_block*)blocks_addr;

	if (type == LARGE) {
		new_zone->blocks->size = size;
		new_zone->blocks->free = 0;
		new_zone->blocks->prev = NULL;
	} else {
		size_t available_size = zone_size - (blocks_addr - (uintptr_t)zone_start) - sizeof(t_block);
		new_zone->blocks->size = available_size;
		new_zone->blocks->free = 1;
		new_zone->blocks->prev = NULL;
	}
	new_zone->blocks->next = NULL;
	
	new_zone->next = g_zones;
	g_zones = new_zone;
	return (new_zone);
}

void split_block(t_block *block, size_t size) {
	if (block->size >= size + sizeof(t_block) + ALIGNMENT) {
		t_block *new_block = (t_block*)((char*)block + sizeof(t_block) + size);
		new_block->size = block->size - size - sizeof(t_block);
		new_block->free = 1;
		new_block->prev = block;
		new_block->next = block->next;
		if (block->next) block->next->prev = new_block;
		block->size = size;
		block->next = new_block;
	}
}
	
