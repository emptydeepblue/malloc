/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: everonel <everonel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 17:50:03 by everonel          #+#    #+#             */
/*   Updated: 2026/02/19 20:57:47 by everonel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "ft_malloc.h"

void free(void *ptr) {
    if (!ptr)
        return;
    pthread_mutex_lock(&g_mutex);
    t_block *block = (t_block*)((char*)ptr - sizeof(t_block));
    t_zone *zone = find_zone_containing_block(block);
    if (!zone) {
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    block->free = 1;
    merge_adjacent_free_blocks(block);
    if (zone->type == LARGE && block->free) {
        if (g_zones == zone) {
            g_zones = zone->next;
        } else {
            t_zone *prev = g_zones;
            while (prev && prev->next != zone) prev = prev->next;
            if (prev) prev->next = zone->next;
        }
        munmap(zone->start, zone->size);
    }
    pthread_mutex_unlock(&g_mutex);
}

t_zone *find_zone_containing_block(t_block *block) {
    t_zone *current = g_zones;
    while (current) {
        t_block *b = current->blocks;
        while (b) {
            if (b == block) return current;
            b = b->next;
        }
        current = current->next;
    }
    return NULL;
}

void merge_adjacent_free_blocks(t_block *block) {
    if (block->next && block->next->free) {
        block->size += sizeof(t_block) + block->next->size;
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }
    if (block->prev && block->prev->free) {
        block->prev->size += sizeof(t_block) + block->size;
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
    }
}