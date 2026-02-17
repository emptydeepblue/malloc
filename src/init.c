/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: everonel <everonel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 16:19:41 by everonel          #+#    #+#             */
/*   Updated: 2026/02/17 16:25:17 by everonel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

t_zone *g_zones = NULL;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;