/*
 * (C) Copyright 2008-2013
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsdqueue.h"
#include "util.h"

struct notify_elem {
	notifier client;
	STAILQ_ENTRY(notify_elem) next;
};

STAILQ_HEAD(notifylist, notify_elem);

static struct notifylist clients;

int register_notifier(notifier client)
{

	struct notify_elem *newclient;

	if (!client)
		return -1;

	newclient = (struct notify_elem *)calloc(1, sizeof(struct notify_elem));
	newclient->client = client;

	STAILQ_INSERT_TAIL(&clients, newclient, next);

	return 0;
}

void notify(RECOVERY_STATUS status, int error, const char *msg)
{

	struct notify_elem *elem;

	STAILQ_FOREACH(elem, &clients, next)
		(elem->client)(status, error, msg);

}

static void console_notifier (RECOVERY_STATUS status, int error, const char *msg)
{
	char current[80];
	switch(status) {
	case IDLE:
		strncpy(current, "No SWUPDATE running : ", sizeof(current));
		break;
	case DOWNLOAD:
		strncpy(current, "SWUPDATE downloading : ", sizeof(current));
		break;
	case START:
		strncpy(current, "SWUPDATE started : ", sizeof(current));
		break;
	case RUN:
		strncpy(current, "SWUPDATE running : ", sizeof(current));
		break;
	case SUCCESS:
		strncpy(current, "SWUPDATE successful !", sizeof(current));
		break;
	case FAILURE:
		snprintf(current, sizeof(current), "SWUPDATE failed [%d]", error);
		break;
	case DONE:
		strncpy(current, "SWUPDATE done : ", sizeof(current));
		break;
	}

	fprintf(stdout, "[NOTIFY] : %s %s\n", current, msg ? msg : "");
	fflush(stdout);
}

void notify_init(void)
{
	STAILQ_INIT(&clients);
	register_notifier(console_notifier);
}
