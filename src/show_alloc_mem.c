/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: everonel <everonel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 00:00:00 by emma              #+#    #+#             */
/*   Updated: 2026/02/17 16:53:28 by everonel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"
#include <unistd.h>  // For write

// Helper to print hex address (simple implementation)
void print_hex(unsigned long long addr) {
    char buf[19];  // 0x + 16 hex + \0
    int i = 17;
    buf[18] = '\0';
    buf[0] = '0';
    buf[1] = 'x';
    if (addr == 0) {
        buf[2] = '0';
        buf[3] = '\0';
        write(1, buf, 3);
        return;
    }
    while (addr > 0 && i > 1) {
        int digit = addr % 16;
        buf[i--] = (digit < 10) ? '0' + digit : 'a' + (digit - 10);
        addr /= 16;
    }
    write(1, buf + i + 1, 18 - i);
}

void show_alloc_mem(void) {
    pthread_mutex_lock(&g_mutex);
    size_t total = 0;
    t_zone *zone = g_zones;
    while (zone) {
        if (zone->type == TINY) {
            write(1, "TINY : ", 7);
            print_hex((unsigned long long)zone->start);
            write(1, "\n", 1);
        } else if (zone->type == SMALL) {
            write(1, "SMALL : ", 8);
            print_hex((unsigned long long)zone->start);
            write(1, "\n", 1);
        } else if (zone->type == LARGE) {
            write(1, "LARGE : ", 8);
            print_hex((unsigned long long)zone->start);
            write(1, "\n", 1);
        }
        t_block *block = zone->blocks;
        while (block) {
            if (!block->free) {
                void *start = (char*)block + sizeof(t_block);
                void *end = (char*)start + block->size;
                print_hex((unsigned long long)start);
                write(1, " - ", 3);
                print_hex((unsigned long long)end);
                write(1, " : ", 3);
                // Print size (use ft_putnbr_fd from libft if available, else simple loop)
                char num[20];
                int len = 0;
                size_t n = block->size;
                if (n == 0) num[len++] = '0';
                while (n > 0) {
                    num[len++] = '0' + (n % 10);
                    n /= 10;
                }
                for (int i = len - 1; i >= 0; i--) write(1, &num[i], 1);
                write(1, " bytes\n", 7);
                total += block->size;
            }
            block = block->next;
        }
        zone = zone->next;
    }
    write(1, "Total : ", 8);
    char num[20];
    int len = 0;
    size_t n = total;
    if (n == 0) num[len++] = '0';
    while (n > 0) {
        num[len++] = '0' + (n % 10);
        n /= 10;
    }
    for (int i = len - 1; i >= 0; i--) write(1, &num[i], 1);
    write(1, " bytes\n", 7);
    pthread_mutex_unlock(&g_mutex);
}