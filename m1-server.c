//------------------------------------------------------------------------------
/**
 * @file m1-server.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief user interface with framebuffer test app.
 * @version 0.1
 * @date 2022-12-05
 *
 * @copyright Copyright (c) 2022
 *
 */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <getopt.h>

#include "lib_fbui/lib_fb.h"
#include "lib_fbui/lib_ui.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const char *OPT_DEVICE_NAME = "/dev/fb0";
const char *OPT_FBUI_CFG = "fbui.cfg";

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	fb_info_t	*pfb;
	ui_grp_t 	*ui_grp;

	if ((pfb = fb_init (OPT_DEVICE_NAME)) == NULL) {
		fprintf(stdout, "ERROR: frame buffer init fail!\n");
		exit(1);
	}
    fb_cursor (0);

	if ((ui_grp = ui_init (pfb, OPT_FBUI_CFG)) == NULL) {
		fprintf(stdout, "ERROR: User interface create fail!\n");
		exit(1);
	}
	ui_update(pfb, ui_grp, -1);

	ui_close(ui_grp);
	fb_close (pfb);

	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
