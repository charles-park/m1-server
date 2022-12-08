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

//------------------------------------------------------------------------------
// git submodule header include
//------------------------------------------------------------------------------
#include "storage_test/storage_test.h"
#include "lib_fbui/lib_fb.h"
#include "lib_fbui/lib_ui.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const char *OPT_DEVICE_NAME = "/dev/fb0";
const char *OPT_FBUI_CFG = "fbui.cfg";

//------------------------------------------------------------------------------
#define	DEV_SPEED_EMMC	150
#define	DEV_SPEED_SDMMC	50
#define	DEV_SPEED_NVME	1000
#define	DEV_SPEED_SATA	500

//------------------------------------------------------------------------------
void bootup_test (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	char resp_str[32];
	int speed;

	/* system state display "wait" */
	ui_set_ritem (pfb, ui_grp, 47, COLOR_GRAY, -1);

	memset (resp_str, 0, sizeof(resp_str));
	sprintf (resp_str, "%d x %d", pfb->w, pfb->h);
	ui_set_sitem (pfb, ui_grp, 42, -1, -1, resp_str);
	if ((pfb->w != 1920) || (pfb->h != 1080))
		ui_set_ritem (pfb, ui_grp, 42, COLOR_RED, -1);

	memset (resp_str, 0, sizeof(resp_str));
	speed = storage_test ("emmc", resp_str);
	ui_set_sitem (pfb, ui_grp, 62, -1, -1, resp_str);
	ui_set_ritem (pfb, ui_grp, 62, speed > DEV_SPEED_EMMC ? COLOR_GREEN : COLOR_RED, -1);

	memset (resp_str, 0, sizeof(resp_str));
	speed = storage_test ("sata", resp_str);
	ui_set_sitem (pfb, ui_grp, 82, -1, -1, resp_str);
	ui_set_ritem (pfb, ui_grp, 82, speed > DEV_SPEED_SATA ? COLOR_GREEN : COLOR_RED, -1);

	memset (resp_str, 0, sizeof(resp_str));
	speed = storage_test ("nvme", resp_str);
	ui_set_sitem (pfb, ui_grp, 87, -1, -1, resp_str);
	ui_set_ritem (pfb, ui_grp, 87, speed > DEV_SPEED_NVME ? COLOR_GREEN : COLOR_RED, -1);
}

//------------------------------------------------------------------------------
// iperf3 -c 192.168.20.37 -t 1 -u -b G | grep MBytes | grep %
// iperf3 -c 192.168.20.37 -t 1 -u -b G | grep Mbits/sec | grep %
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
	/* test storage, iperf, mac */
	bootup_test(pfb, ui_grp);

	ui_close(ui_grp);
	fb_close (pfb);

	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
