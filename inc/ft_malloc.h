/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: everonel <everonel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 00:00:00 by emma              #+#    #+#             */
/*   Updated: 2026/02/19 20:55:27 by everonel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_MALLOC_H
# define FT_MALLOC_H

# include <sys/mman.h>
# include <pthread.h>
# include <stdint.h>
# include "libft.h"

/*
** Constants
*/
# define ALIGNMENT 16
# define TINY_MAX 64
# define SMALL_MAX 1024
# define TINY_ZONE_SIZE (4 * 1024 * 1024)  // 4 MB
# define SMALL_ZONE_SIZE (8 * 1024 * 1024) // 8 MB

/*
** Enums
*/
typedef enum e_zone_type
{
    TINY,
    SMALL,
    LARGE
}	t_zone_type;

/*
** Structures
*/
typedef struct s_block
{
    size_t          size;
    int             free;
    struct s_block  *prev;
    struct s_block  *next;
}	t_block;

typedef struct s_zone
{
    t_zone_type type;
    size_t          size;
    void            *start;
    t_block         *blocks;
    struct s_zone   *next;
}	t_zone;

/*
** Global variables (allowed: one for allocations, one for mutex)
*/
extern t_zone *g_zones;
extern pthread_mutex_t g_mutex;

/*
** Prototypes for main functions
*/
void	*malloc(size_t size);
void	free(void *ptr);
void	*realloc(void *ptr, size_t size);

/*
** Debug/utility functions
*/
void	show_alloc_mem(void);

/*
** Internal helper functions (not exposed in the public API)
*/
size_t  align_size(size_t size);
t_zone  *find_or_create_zone(t_zone_type type, size_t size);
t_block *find_suitable_block(t_zone *zone, size_t size);
t_zone  *create_zone(t_zone_type type, size_t size);
void    split_block(t_block *block, size_t size);
t_zone *find_zone_containing_block(t_block *block);
void    merge_adjacent_free_blocks(t_block *block);
#endif
