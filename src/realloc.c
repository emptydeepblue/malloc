/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: everonel <everonel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/19 20:28:21 by everonel          #+#    #+#             */
/*   Updated: 2026/02/19 20:35:47 by everonel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "ft_malloc.h"

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    size = align_size(size);
    pthread_mutex_lock(&g_mutex);
    t_block *block = (t_block*)((char*)ptr - sizeof(t_block));
    t_zone *zone = find_zone_containing_block(block);
    if (!zone) {
        pthread_mutex_unlock(&g_mutex);
        return NULL;
    }
    
    if (block->size >= size) {
        split_block(block, size);
        pthread_mutex_unlock(&g_mutex);
        return ptr;
    } else {
        pthread_mutex_unlock(&g_mutex);
        void *new_ptr = malloc(size);
        if (!new_ptr) {
            return NULL;
        }
        ft_memcpy(new_ptr, ptr, block->size);
        free(ptr);
        return new_ptr;
    }
}