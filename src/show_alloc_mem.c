/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: everonel <everonel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 00:00:00 by emma              #+#    #+#             */
/*   Updated: 2026/02/19 22:23:02 by everonel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

// Helper to print hex address (simple implementation)
void print_hex(unsigned long long addr) {
    char buffer[20];
    int i = 0;
    const char *hex = "0123456789abcdef";

    buffer[i++] = '0';
    buffer[i++] = 'x';
    for (int shift = (sizeof(addr) * 8) - 4; shift >= 0; shift -= 4) {
        buffer[i++] = hex[(addr >> shift) & 0xF];
    }
    buffer[i] = '\0';
    write(1, buffer, i);
}

void show_alloc_mem(void) {
    pthread_mutex_lock(&g_mutex);
    t_zone *zone = g_zones;
    size_t total = 0;
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
                // Print size
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