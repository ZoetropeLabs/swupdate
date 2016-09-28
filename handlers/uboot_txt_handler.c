/*
 * (C) Copyright 2013
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

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "swupdate.h"
#include "handler.h"
#include "util.h"

#define INI_ANSIONLY
#include "minini/minGlue.h"

static void uboot_txt_handler(void);

static int modify_uboot_script(struct img_type *img,
	void __attribute__ ((__unused__)) *data)
{
	int ret;
	int fdout;
	char buf[64];

	char filename[64];
	struct stat statbuf;

	snprintf(filename, sizeof(filename), "%s%s", TMPDIR, img->fname);
	ret = stat(filename, &statbuf);
	if (ret) {
		fdout = openfileoutput(filename);
		/*
		 * U-Boot environment is set inside sw-description
		 * there is no hash but sw-description was already verified
		 */
		ret = copyimage(&fdout, img, NULL);
		close(fdout);
	}

	if (!strlen(img->path))
    {
        ret = -1;
        ERROR("Need to specify path of existing U-Boot script");
    }
    else
    {
        // no sections
        int section_idx = NULL;
        char * section = NULL;

        int key_idx;
        char key[64];
        char value[128];

        int n_read;

        for (key_idx = 0; init_getkey(section key_idx, key, sizeof(key), filename) > 0; key_idx++)
        {
            n_read = ini_gets(section, key, "", value, 128, filename);

            if (!n_read)
            {
                ERROR("Invalid value detected for key %s", key);
                ret = -1;
                break;
            }

            ini_puts(section, key, value, img->path);
        }
    }

	if (ret < 0)
		snprintf(buf, sizeof(buf), "Error setting U-Boot environment");
	else
		snprintf(buf, sizeof(buf), "U-Boot environment updated");

	notify(RUN, RECOVERY_NO_ERROR, buf);

	return ret;
}

__attribute__((constructor))
static void uboot_txt_handler(void)
{
	register_handler("uboot_txt", modify_uboot_script, NULL);
}

