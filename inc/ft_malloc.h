/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: everonel <everonel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 00:00:00 by emma              #+#    #+#             */
/*   Updated: 2026/01/23 13:25:43 by everonel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_MALLOC_H
# define FT_MALLOC_H

# include <stdlib.h>
# include <unistd.h>
# include <sys/mman.h>
# include <pthread.h>

/*
** Memory allocation functions
*/
void	*malloc(size_t size);
void	free(void *ptr);
void	*realloc(void *ptr, size_t size);

/*
** Debug/utility functions
*/
void	show_alloc_mem(void);

#endif
