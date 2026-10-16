/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: everonel <everonel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 00:00:00 by emma              #+#    #+#             */
/*   Updated: 2026/10/16 00:00:00 by everonel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

static void	print_hex(unsigned long long addr)
{
	char		buffer[20];
	int			i;
	const char	*hex = "0123456789abcdef";
	int			shift;

	i = 0;
	buffer[i++] = '0';
	buffer[i++] = 'x';
	shift = (sizeof(addr) * 8) - 4;
	while (shift >= 0)
	{
		buffer[i++] = hex[(addr >> shift) & 0xF];
		shift -= 4;
	}
	write(1, buffer, i);
}

static void	print_size(size_t n)
{
	char	num[20];
	int		len;

	len = 0;
	if (n == 0)
		num[len++] = '0';
	while (n > 0)
	{
		num[len++] = '0' + (n % 10);
		n /= 10;
	}
	while (--len >= 0)
		write(1, &num[len], 1);
}

static void	print_zone(t_zone *zone, size_t *total)
{
	t_block	*block;

	if (zone->type == TINY)
		write(1, "TINY : ", 7);
	else if (zone->type == SMALL)
		write(1, "SMALL : ", 8);
	else
		write(1, "LARGE : ", 8);
	print_hex((unsigned long long)zone->start);
	write(1, "\n", 1);
	block = zone->blocks;
	while (block)
	{
		if (!block->free)
		{
			void	*start = (char *)block + sizeof(t_block);
			void	*end = (char *)start + block->size;
			print_hex((unsigned long long)start);
			write(1, " - ", 3);
			print_hex((unsigned long long)end);
			write(1, " : ", 3);
			print_size(block->size);
			write(1, " bytes\n", 7);
			*total += block->size;
		}
		block = block->next;
	}
}

void	show_alloc_mem(void)
{
	void	*last;
	t_zone	*zone;
	t_zone	*min;
	size_t	total;

	pthread_mutex_lock(&g_mutex);
	total = 0;
	last = NULL;
	while (1)
	{
		zone = g_zones;
		min = NULL;
		while (zone)
		{
			if (zone->start > last && (!min || zone->start < min->start))
				min = zone;
			zone = zone->next;
		}
		if (!min)
			break ;
		last = min->start;
		print_zone(min, &total);
	}
	write(1, "Total : ", 8);
	print_size(total);
	write(1, " bytes\n", 7);
	pthread_mutex_unlock(&g_mutex);
}
