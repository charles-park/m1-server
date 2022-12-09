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
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <getopt.h>

//------------------------------------------------------------------------------
// git submodule header include
//------------------------------------------------------------------------------
#include "mac_server_ctrl/mac_server_ctrl.h"
#include "nlp_server_ctrl/nlp_server_ctrl.h"
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

#define	IPERF_SPEED		800
//------------------------------------------------------------------------------
int system_memory (void)
{
	struct 	sysinfo sinfo;

	if (!sysinfo (&sinfo)) {
		if (sinfo.totalram) {
			int mem_size = sinfo.totalram / 1024 / 1024;
			if		((mem_size > 4096) && (mem_size < 8192))
				return 8;
			else if	((mem_size > 2048) && (mem_size < 4096))
				return 4;
			else
				return 0;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
char BoardIP    [20] = {0,};
char NlpServerIP[20] = {0,};

void bootup_test (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	char resp_str[32];
	int speed, retry;

	/* system state display "wait" */
	ui_set_ritem (pfb, ui_grp, 47, COLOR_GRAY, -1);

	{
		int mem = system_memory ();
		memset (resp_str, 0, sizeof(resp_str));
		if (mem) {
			sprintf (resp_str, "%d GB", mem);
			ui_set_sitem (pfb, ui_grp, 8, -1, -1, resp_str);
		} else {
			sprintf (resp_str, "%d GB", mem);
			ui_set_sitem (pfb, ui_grp, 8, -1, -1, resp_str);
			ui_set_ritem (pfb, ui_grp, 8, COLOR_RED, -1);
		}
	}

	{
		memset (resp_str, 0, sizeof(resp_str));
		sprintf (resp_str, "%d x %d", pfb->w, pfb->h);
		ui_set_sitem (pfb, ui_grp, 42, -1, -1, resp_str);
		if ((pfb->w != 1920) || (pfb->h != 1080))
			ui_set_ritem (pfb, ui_grp, 42, COLOR_RED, -1);
		else
			ui_set_ritem (pfb, ui_grp, 42, COLOR_GREEN, -1);
	}

	{
		memset (resp_str, 0, sizeof(resp_str));
		speed = 0;	retry = 3;
		while ((retry--) && (speed < DEV_SPEED_EMMC)) {
			speed = storage_test ("emmc", resp_str);
		}
		ui_set_sitem (pfb, ui_grp, 62, -1, -1, resp_str);
		ui_set_ritem (pfb, ui_grp, 62, speed > DEV_SPEED_EMMC ? COLOR_GREEN : COLOR_RED, -1);
	}

	{
		memset (resp_str, 0, sizeof(resp_str));
		speed = 0;	retry = 3;
		while ((retry--) && (speed < DEV_SPEED_SATA)) {
			speed = storage_test ("sata", resp_str);
		}
		ui_set_sitem (pfb, ui_grp, 82, -1, -1, resp_str);
		ui_set_ritem (pfb, ui_grp, 82, speed > DEV_SPEED_SATA ? COLOR_GREEN : COLOR_RED, -1);
	}

	{
		memset (resp_str, 0, sizeof(resp_str));
		speed = 0;	retry = 3;
		while ((retry--) && (speed < DEV_SPEED_NVME)) {
			speed = storage_test ("nvme", resp_str);
		}
		ui_set_sitem (pfb, ui_grp, 87, -1, -1, resp_str);
		ui_set_ritem (pfb, ui_grp, 87, speed > DEV_SPEED_NVME ? COLOR_GREEN : COLOR_RED, -1);
	}

	memset (BoardIP, 0, sizeof(BoardIP));
	memset (NlpServerIP, 0, sizeof(NlpServerIP));

	if (get_my_ip (BoardIP)) {
		ui_set_sitem (pfb, ui_grp, 4, -1, -1, BoardIP);
		ui_set_ritem (pfb, ui_grp, 4, COLOR_GREEN, -1);
	}
	if (nlp_server_find (NlpServerIP)) {

		ui_set_sitem (pfb, ui_grp, 24, -1, -1, NlpServerIP);
		ui_set_ritem (pfb, ui_grp, 24, COLOR_GREEN, -1);
		// UDP = 3, TCP = 4
		nlp_server_write   (NlpServerIP, 3, "start", 0);
		sleep(1);

		speed = 0;	retry = 3;
		while ((retry--) && (speed < IPERF_SPEED)) {
			speed = iperf3_speed_check (NlpServerIP, 3);
		}
		sleep(1);
		nlp_server_write   (NlpServerIP, 3, "stop", 0);

		memset (resp_str, 0, sizeof(resp_str));
		sprintf (resp_str, "%d MBits/sec", speed);
		ui_set_sitem (pfb, ui_grp, 147, -1, -1, resp_str);
		ui_set_ritem (pfb, ui_grp, 147, (speed > IPERF_SPEED) ? COLOR_GREEN : COLOR_RED, -1);
	}

	{
		char mac_str[20] = {0,};

		memset (mac_str, 0, sizeof(mac_str));
		if (!get_my_mac (mac_str)) {
			char uuid[MAC_SERVER_CTRL_TYPE_UUID_SIZE+1];
			memset (uuid, 0, sizeof(uuid));
			if (get_mac_uuid ("m1", MAC_SERVER_CTRL_TYPE_UUID, uuid, MAC_SERVER_CTRL_DEV_SERVER)) {
				// write efuse...compare uuid in efuse...

				// uuid return : c56d8ba1-14c8-408d-90f0-001e06510029
				memset (mac_str, 0, sizeof(mac_str));
				strncpy (mac_str, &uuid[24], 12);
			}
		}

		printf ("mac str = %s\n", mac_str);

		if (!strncmp (mac_str, "001e06", strlen("001e06"))) {
			memset (resp_str, 0, sizeof(resp_str));
			sprintf (resp_str,"00:1e:06:%c%c:%c%c:%c%c",
				mac_str[6],	mac_str[7],	mac_str[8],	mac_str[9],	mac_str[10], mac_str[11]);
			ui_set_sitem (pfb, ui_grp, 167, -1, -1, resp_str);
			ui_set_ritem (pfb, ui_grp, 167, COLOR_GREEN, -1);
	//#define MSG_TYPE_MAC	1
	//#define MSG_TYPE_ERR	2
			/* mac address print */
			nlp_server_write (NlpServerIP, 1, mac_str, 0);
		} else {
			ui_set_sitem (pfb, ui_grp, 167, -1, -1, "unknown mac");
			ui_set_ritem (pfb, ui_grp, 167, COLOR_RED, -1);
		}
	}
}
#if 0
// change eth speed -> ethtool used
async def set_eth_mode(speed):
    cmd = f"echo odroid | sudo -S ethtool -s eth0 speed {speed} duplex full"
#endif

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
	/* test storage, iperf, mac */
	bootup_test(pfb, ui_grp);

	ui_close(ui_grp);
	fb_close (pfb);

	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
