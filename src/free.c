/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: everonel <everonel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 17:50:03 by everonel          #+#    #+#             */
/*   Updated: 2026/02/17 17:52:39 by everonel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "ft_malloc.h"

void free(void *ptr) {
    if (!ptr)
        return;
    // Implementation of free:
    // 1. Lock mutex.
    // 2. Get block metadata from ptr (ptr - sizeof(t_block)).
    // 3. Mark block as free.
    // 4. Merge with adjacent free blocks if possible.
    // 5. If LARGE block, munmap the entire zone.
    // 6. Unlock mutex.
    (void)ptr; // To avoid unused parameter warning
    return; // Placeholder implementation
}