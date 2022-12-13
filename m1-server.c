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
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/input.h>
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
#define	RUN_BOX_ON	RGB_TO_UINT(204, 204, 0)
#define	RUN_BOX_OFF	RGB_TO_UINT(153, 153, 0)

enum {
	eSTATUS_WAIT = 0,
	eSTATUS_RUNNING,
	eSTATUS_FINISH,
	eSTATUS_STOP,
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TEST_RETRY_COUNT	5

char BoardIP    [20] = {0,};
char NlpServerIP[20] = {0,};
char MacStr     [20] = {0,};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const char	*efuse_uuid_file = "/sys/class/efuse/uuid";

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const char	*USB_DEVICE_DIR = "/sys/bus/usb/devices";
const char	*USB_PLATFORM_DIR = "/sys/devices/platform";
const char	USB_DEVICE_NAME[][4] = {
	"8-1",	/* usb3.0 port up : detect usb 3.0*/
	"7-1",	/* usb3.0 port up : detect usb 2.0*/
	"6-1",	/* usb3.0 port dn : detect usb 3.0*/
	"5-1",	/* usb3.0 port dn : detect usb 2.0*/
	"1-1",	/* usb2.0 port up : detect usb 2.0*/
	"3-1",	/* usb2.0 port dn : detect usb 1.1*/
	"2-1",	/* usb2.0 port up : detect usb 2.0*/
	"4-1",	/* usb2.0 port dn : detect usb 1.1*/
};

//------------------------------------------------------------------------------
#define	USB30_MASS_SPEED	60
#define	USB20_MASS_SPEED	20

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const char *OPT_DEVICE_NAME = "/dev/fb0";
const char *OPT_FBUI_CFG = "fbui.cfg";

//------------------------------------------------------------------------------
#define	DEV_SPEED_EMMC	150
#define	DEV_SPEED_SDMMC	50
#define	DEV_SPEED_NVME	1000
#define	DEV_SPEED_SATA	400

#define	IPERF_SPEED		800

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// function prototype define
//------------------------------------------------------------------------------
int		system_memory		(void);
int		macaddr_print		(void);
int		get_efuse_mac		(char *mac_str);
int		write_efuse			(char *uuid);
int		change_eth_speed	(int speed);
int		test_efuse_uuid		(fb_info_t *pfb, ui_grp_t *ui_grp);
int		test_board_mem		(fb_info_t *pfb, ui_grp_t *ui_grp);
int		test_fb_size		(fb_info_t *pfb, ui_grp_t *ui_grp);
int		test_emmc_speed		(fb_info_t *pfb, ui_grp_t *ui_grp);
int		test_sata_speed		(fb_info_t *pfb, ui_grp_t *ui_grp);
int		test_nvme_speed		(fb_info_t *pfb, ui_grp_t *ui_grp);
int		test_iperf_speed	(fb_info_t *pfb, ui_grp_t *ui_grp);
int		test_ir_ethled		(fb_info_t *pfb, ui_grp_t *ui_grp);
int		test_hp_detect		(fb_info_t *pfb, ui_grp_t *ui_grp);
int		test_spi_button		(fb_info_t *pfb, ui_grp_t *ui_grp);
int		test_usb_speed		(fb_info_t *pfb, ui_grp_t *ui_grp);
int		bootup_test			(fb_info_t *pfb, ui_grp_t *ui_grp);
int		run_interval_check	(struct timeval *t, double interval_ms);
void	test_status 		(fb_info_t *pfb, ui_grp_t *ui_grp, int status);
int		main				(int argc, char **argv);

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
int macaddr_print (void)
{
	/* mac address print */
	return nlp_server_write (NlpServerIP, NLP_SERVER_MSG_TYPE_MAC, MacStr, 0);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int get_efuse_mac (char *mac_str)
{
	FILE *fp;
	char cmd_line[128];

	if (access (efuse_uuid_file, F_OK) != 0)
		return 0;

	if ((fp = fopen(efuse_uuid_file, "r")) != NULL) {
		if (fgets (cmd_line, sizeof(cmd_line), fp) != NULL) {
			char *ptr;
			int i;
			if ((ptr = strstr(cmd_line, "001e06")) != NULL) {
				for (i = 0; i < 12; i++)
					mac_str[i] = *(ptr + i);
				printf ("%s : mac str = %s\n", __func__, mac_str);
				fclose (fp);
				return 1;
			} else {
				/* display hex value */
				int i, len = strlen(cmd_line);
				for (i = 0; i < len; i++)
					printf ("%d - 0x%02x\n", i, cmd_line[i]);
			}
		}
		fclose(fp);
	}
	return 0;
}

//------------------------------------------------------------------------------
// 처음 1회만 기록 가능하게 함.
// 문제가 있는 경우 mac_server_ctrl의 mac_server_ctrl.py를 통하여 uuid를 얻어
// efuse의 efuse_ctl.py로 offset을 주어 기록하도록 한다.
// uuid offset은 efuse_crl.py의 아래의 옵션 후 dmesg를 통하여 ff가 기록된 지점으로 부터 시작한다.
//
// python3 efuse_ctl.py -d 0
// dmesg
//
// 기록 후 확인은 /sys/class/efuse/uuid를 통하여 확인한다.
//
// python3 mac_server_ctrl.py -r m1 -f
// python3 efuse_ctl.py -w {uuid} {offsest:0,32,64,96}
//
// *************** Warning *****************************************************
// mac_server_ctrl의 마지막 옵션의 -f는 서버를 설정하는 것으로 없는 경우 dev-server에서
// uuid를 가지고 오며, dev-server의 uuid는 증가하지 않는다. 주의 요망
// 또안 efuse의 기록은 4번까지만 할 수 있으므로 신중하게 확인 후 진행하도록 한다.
// ******************************************************************************
//------------------------------------------------------------------------------
int write_efuse (char *uuid)
{
	FILE *fp;
	char cmd_line[128], offset = 0;

	if (access ("/dev/efuse", F_OK) != 0)
		return 0;

	memset (cmd_line, 0x00, sizeof(cmd_line));
	sprintf(cmd_line,"python3 efuse/efuse_ctl.py -w %s %d", uuid, offset);

	if ((fp = popen(cmd_line, "r")) != NULL) {
		memset (cmd_line, 0x00, sizeof(cmd_line));
		while (fgets (cmd_line, sizeof(cmd_line), fp) != NULL) {
			if (strstr (cmd_line, "efuse write success") != NULL) {
				printf ("%s : efuse write success! uuid = %s \n", __func__, uuid);
				pclose (fp);
				return 1;
			}
			memset (cmd_line, 0x00, sizeof(cmd_line));
		}
		pclose(fp);
	}
/* efuse mac offset control */
#if 0
	for (offset = 0; offset < 128; offset += 32) {
		memset (cmd_line, 0x00, sizeof(cmd_line));
		sprintf(cmd_line,"python3 efuse/efuse_ctl.py -w %s %d", uuid, offset);

		if ((fp = popen(cmd_line, "r")) != NULL) {
			memset (cmd_line, 0x00, sizeof(cmd_line));
			while (fgets (cmd_line, sizeof(cmd_line), fp) != NULL) {
				// msg ("fgets = %s\n", cmd);
				if (strstr (cmd_line, "efuse write success") != NULL) {
					pclose (fp);
					return 1;
				}
				memset (cmd_line, 0x00, sizeof(cmd_line));
			}
			pclose(fp);
		}
		memset (cmd_line, 0x00, sizeof(cmd_line));
		sprintf(cmd_line,"python3 efuse/efuse_ctl.py -c %d", uuid, offset);

		if ((fp = popen(cmd_line, "r")) != NULL) {
			memset (cmd_line, 0x00, sizeof(cmd_line));
			while (fgets (cmd_line, sizeof(cmd_line), fp) != NULL) {
				// msg ("fgets = %s\n", cmd);
				if (strstr (cmd_line, "efuse erase success") != NULL) {
					pclose (fp);
				}
				memset (cmd_line, 0x00, sizeof(cmd_line));
			}
			pclose(fp);
		}
	}
#endif
	return 0;
}

//------------------------------------------------------------------------------
int test_board_mem (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	int mem = system_memory ();
	char resp_str[32];

	memset (resp_str, 0, sizeof(resp_str));
	if (mem) {
		sprintf (resp_str, "%d GB", mem);
		ui_set_sitem (pfb, ui_grp, 8, -1, -1, resp_str);
	} else {
		sprintf (resp_str, "%d GB", mem);
		ui_set_sitem (pfb, ui_grp, 8, -1, -1, resp_str);
		ui_set_ritem (pfb, ui_grp, 8, COLOR_RED, -1);
	}
	return mem ? 1 : 0;
}

//------------------------------------------------------------------------------
int test_fb_size (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	char resp_str[32];

	memset (resp_str, 0, sizeof(resp_str));
	sprintf (resp_str, "%d x %d", pfb->w, pfb->h);
	ui_set_sitem (pfb, ui_grp, 42, -1, -1, resp_str);

	if ((pfb->w != 1920) || (pfb->h != 1080)) {
		ui_set_ritem (pfb, ui_grp, 42, COLOR_RED, -1);
		return 0;
	} else {
		ui_set_ritem (pfb, ui_grp, 42, COLOR_GREEN, -1);
		return 1;
	}
}

//------------------------------------------------------------------------------
int test_emmc_speed (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	char resp_str[32];
	int speed = 0, retry = TEST_RETRY_COUNT;

	memset (resp_str, 0, sizeof(resp_str));
	while ((retry--) && (speed < DEV_SPEED_EMMC))
		speed = storage_test ("emmc", resp_str);

	ui_set_sitem (pfb, ui_grp, 62, -1, -1, resp_str);
	ui_set_ritem (pfb, ui_grp, 62, speed > DEV_SPEED_EMMC ? COLOR_GREEN : COLOR_RED, -1);
	return retry ? 1 : 0;
}

//------------------------------------------------------------------------------
int test_sata_speed (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	char resp_str[32];
	int speed = 0, retry = TEST_RETRY_COUNT;

	memset (resp_str, 0, sizeof(resp_str));
	while ((retry--) && (speed < DEV_SPEED_SATA))
		speed = storage_test ("sata", resp_str);

	ui_set_sitem (pfb, ui_grp, 82, -1, -1, resp_str);
	ui_set_ritem (pfb, ui_grp, 82, speed > DEV_SPEED_SATA ? COLOR_GREEN : COLOR_RED, -1);
	return retry ? 1 : 0;
}

//------------------------------------------------------------------------------
int test_nvme_speed (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	char resp_str[32];
	int speed = 0, retry = TEST_RETRY_COUNT;

	memset (resp_str, 0, sizeof(resp_str));
	while ((retry--) && (speed < DEV_SPEED_NVME))
		speed = storage_test ("nvme", resp_str);

	ui_set_sitem (pfb, ui_grp, 87, -1, -1, resp_str);
	ui_set_ritem (pfb, ui_grp, 87, speed > DEV_SPEED_NVME ? COLOR_GREEN : COLOR_RED, -1);
	return retry ? 1 : 0;
}

//------------------------------------------------------------------------------
int test_iperf_speed (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	char resp_str[32];
	int speed = 0, retry = TEST_RETRY_COUNT * 2;

	// UDP = 3, TCP = 4
	nlp_server_write   (NlpServerIP, 3, "start", 0);
	sleep(1);
	memset (resp_str, 0, sizeof(resp_str));
	while ((retry--) && (speed < IPERF_SPEED)) {
		speed = iperf3_speed_check (NlpServerIP, 3);
		printf ("iperf result : retry = %d, speed = %d Mbits/s\n", retry, speed);
	}
	sleep(1);
	nlp_server_write   (NlpServerIP, 3, "stop", 0);

	sprintf (resp_str, "%d MBits/sec", speed);
	ui_set_sitem (pfb, ui_grp, 147, -1, -1, resp_str);
	ui_set_ritem (pfb, ui_grp, 147, (speed > IPERF_SPEED) ? COLOR_GREEN : COLOR_RED, -1);

	return retry ? 1 : 0;
}

//------------------------------------------------------------------------------
int test_efuse_uuid (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	char resp_str[32];

	if (!get_efuse_mac (MacStr)) {
		char uuid[MAC_SERVER_CTRL_TYPE_UUID_SIZE+1];
		memset (uuid, 0, sizeof(uuid));
		// get mac from server (mac server : FACTORY_SERVER | DEV_SERVER)
		printf ("request uuid from factory server\n");
		if (get_mac_uuid ("m1", MAC_SERVER_CTRL_TYPE_UUID, uuid, MAC_SERVER_CTRL_FACTORY_SERVER)) {
			// write efuse...compare uuid in efuse...
			printf ("write uuid to efuse\n");
			if (write_efuse (uuid)) {
				// uuid return : c56d8ba1-14c8-408d-90f0-001e06510029
				memset (MacStr, 0, sizeof(MacStr));
				strncpy (MacStr, &uuid[24], 12);
				printf ("efuse write success. efuse = %s\n", uuid);
			}
			else
				printf ("efuse write fail.");
		}
	}
	if (!strncmp (MacStr, "001e06", strlen("001e06"))) {
		memset (resp_str, 0, sizeof(resp_str));
		sprintf (resp_str,"00:1e:06:%c%c:%c%c:%c%c",
			MacStr[6],	MacStr[7],	MacStr[8],	MacStr[9],	MacStr[10], MacStr[11]);
		ui_set_sitem (pfb, ui_grp, 167, -1, -1, resp_str);
		ui_set_ritem (pfb, ui_grp, 167, COLOR_GREEN, -1);
		return 1;
	} else {
		ui_set_sitem (pfb, ui_grp, 167, -1, -1, "unknown mac");
		ui_set_ritem (pfb, ui_grp, 167, COLOR_RED, -1);
		return 0;
	}
}

//------------------------------------------------------------------------------
int bootup_test (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	int err = 0, ret, retry;

	memset (BoardIP, 0, sizeof(BoardIP));
	memset (NlpServerIP, 0, sizeof(NlpServerIP));
	memset (MacStr, 0, sizeof(MacStr));

	/* default status */
	ui_set_sitem (pfb, ui_grp, 47, -1, -1, "WAIT");
	ui_set_ritem (pfb, ui_grp, 47, COLOR_GRAY, -1);

	retry = TEST_RETRY_COUNT;
	while (((ret = get_my_ip (BoardIP)) == 0) && (retry--))
		sleep(1);

	if (ret) {
		ui_set_sitem (pfb, ui_grp, 4, -1, -1, BoardIP);
		ui_set_ritem (pfb, ui_grp, 4, COLOR_GREEN, -1);
	} else {
		memset (BoardIP, 0, sizeof(BoardIP));
		sprintf(BoardIP, "%s", "Network Error!");
		ui_set_sitem (pfb, ui_grp, 4, -1, -1, BoardIP);
		ui_set_ritem (pfb, ui_grp, 4, COLOR_RED, -1);
		while (1)	sleep(1);
	}

	retry = TEST_RETRY_COUNT;
	while (((ret = nlp_server_find (NlpServerIP)) == 0) && (retry--))
		sleep(1);

	if (ret) {
		ui_set_sitem (pfb, ui_grp, 24, -1, -1, NlpServerIP);
		ui_set_ritem (pfb, ui_grp, 24, COLOR_GREEN, -1);
		err += test_iperf_speed (pfb, ui_grp) ? 0 : 1;
	} else {
		memset (NlpServerIP, 0, sizeof(NlpServerIP));
		sprintf(NlpServerIP, "%s", "Network Error!");
		ui_set_sitem (pfb, ui_grp, 24, -1, -1, NlpServerIP);
		ui_set_ritem (pfb, ui_grp, 24, COLOR_RED, -1);
		while (1)	sleep(1);
	}

	err += test_efuse_uuid (pfb, ui_grp) ? 0 : 1;
	err += test_board_mem  (pfb, ui_grp) ? 0 : 1;
	err += test_fb_size    (pfb, ui_grp) ? 0 : 1;
	err += test_emmc_speed (pfb, ui_grp) ? 0 : 1;
	err += test_sata_speed (pfb, ui_grp) ? 0 : 1;
	err += test_nvme_speed (pfb, ui_grp) ? 0 : 1;
	return err;
}

//------------------------------------------------------------------------------
int run_interval_check (struct timeval *t, double interval_ms)
{
	struct timeval base_time;
	double difftime;

	gettimeofday(&base_time, NULL);

	if (interval_ms) {
		/* 현재 시간이 interval시간보다 크면 양수가 나옴 */
		difftime = (base_time.tv_sec - t->tv_sec) +
					((base_time.tv_usec - (t->tv_usec + interval_ms * 1000)) / 1000000);

		if (difftime > 0) {
			t->tv_sec  = base_time.tv_sec;
			t->tv_usec = base_time.tv_usec;
			return 1;
		}
		return 0;
	}
	/* 현재 시간 저장 */
	t->tv_sec  = base_time.tv_sec;
	t->tv_usec = base_time.tv_usec;
	return 1;
}

//------------------------------------------------------------------------------
/*
	find /sys/devices/platform -name *sd* | grep 8-1 2<&1
	strstr "/block/sd"
	usb detect speed = /sys/bus/usb/devices/{usb_device_name}/speed
	get node (ptr + 9) = 'a,b,c,d,....'
	apt install usbutils (lsusb -t...)
*/
//------------------------------------------------------------------------------
int test_usb_speed (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	static int usb30_up = -1, usb30_dn = -1, usb20_up = -1, usb20_dn = -1, item_cnt, i;
	static int usb_prev_check = -1;
	char fname[256];

	/* test complete */
	if ((usb30_up == 1) && (usb30_dn == 1) && (usb20_up == 1) && (usb20_dn == 1))
		return 1;

	item_cnt = sizeof(USB_DEVICE_NAME) / sizeof(USB_DEVICE_NAME[0]);

	for (i = 0; i < item_cnt; i++) {
		memset (fname, 0, sizeof(fname));
		sprintf (fname, "%s/%s", USB_DEVICE_DIR, USB_DEVICE_NAME[i]);
		if (access (fname, F_OK) == 0) {
			FILE *fp;
			int usb_det_speed = 0;
			/* get detect usb speed */
			memset (fname, 0, sizeof(fname));
			sprintf (fname, "%s/%s/speed", USB_DEVICE_DIR, USB_DEVICE_NAME[i]);
			if ((fp = fopen(fname, "r")) != NULL) {
				memset (fname, 0, sizeof(fname));
				if (fgets (fname, sizeof(fname), fp) != NULL)
					usb_det_speed = atoi(fname);
				fclose (fp);
			}
			/* find block node for usb mass */
			memset (fname, 0, sizeof(fname));
			sprintf (fname, "find %s -name *sd* | grep %s 2<&1",
					USB_PLATFORM_DIR, USB_DEVICE_NAME[i]);

			 if ((fp = popen(fname, "r")) != NULL) {
				int speed = -1, ui_id = 0, usb_pass = 0;

				memset (fname, 0x00, sizeof(fname));
				if (fgets (fname, sizeof(fname), fp) != NULL) {
					char *str = strstr (fname, "/block/sd"), node = str[9];
					memset (fname, 0x00, sizeof(fname));
					sprintf (fname, "/dev/sd%c", node);
				}
				pclose (fp);
				if (strncmp(fname, "/dev/sd", strlen("/dev/sd")))
					continue;

				if (usb_prev_check == i)
					continue;

				usb_prev_check = i;
				switch (i) {
					case 0:	case 1:
						ui_id = 102;
						ui_set_ritem (pfb, ui_grp, 102, COLOR_YELLOW, -1);
						if (usb_det_speed > 12)
							speed = storage_read_test (fname, usb_det_speed > 480 ? 5 : 1);
						usb30_up = (speed > USB30_MASS_SPEED) ? 1 : 0;
						usb_pass = usb30_up;
					break;
					case 2:	case 3:
						ui_id = 122;
						ui_set_ritem (pfb, ui_grp, 122, COLOR_YELLOW, -1);
						if (usb_det_speed > 12)
							speed = storage_read_test (fname, usb_det_speed > 480 ? 5 : 1);
						usb30_dn = (speed > USB30_MASS_SPEED) ? 1 : 0;
						usb_pass = usb30_dn;
					break;
					case 4:	case 5:
						ui_id = 107;
						ui_set_ritem (pfb, ui_grp, 107, COLOR_YELLOW, -1);
						if (usb_det_speed > 12)
							speed = storage_read_test (fname, usb_det_speed > 12 ? 5 : 1);
						usb20_up = (speed > USB20_MASS_SPEED) ? 1 : 0;
						usb_pass = usb20_up;
					break;
					case 6:	case 7:
						ui_id = 127;
						ui_set_ritem (pfb, ui_grp, 127, COLOR_YELLOW, -1);
						if (usb_det_speed > 12)
							speed = storage_read_test (fname, usb_det_speed > 12 ? 5 : 1);
						usb20_dn = (speed > USB20_MASS_SPEED) ? 1 : 0;
						usb_pass = usb20_dn;
					break;
					default :
					break;
				}
				if (speed != -1) {
					memset (fname, 0, sizeof(fname));
					sprintf (fname, "%dM - %d MB/s", usb_det_speed, speed);
					ui_set_sitem (pfb, ui_grp, ui_id, -1, -1, fname);
					ui_set_ritem (pfb, ui_grp, ui_id, usb_pass ? COLOR_GREEN : COLOR_RED, -1);
					ui_id = 0;
				}
			 }
		}
	}
	if ((usb30_up == -1) || (usb30_dn == -1) || (usb20_up == -1) || (usb20_dn == -1))
		return -1;

	return	(usb30_up & usb30_dn & usb20_up & usb20_dn);
}

//------------------------------------------------------------------------------
// change eth speed -> ethtool used
// eth speed change cmd (apt install ethtool), speed {100|1000}
// ethtool -s eth0 speed {speed} duplex full
// check eth link speed "/sys/class/net/eth0/speed"
//------------------------------------------------------------------------------
int change_eth_speed (int speed)
{
	FILE *fp;
	char cmd_line[128];

	if (access ("/sys/class/net/eth0/speed", F_OK) != 0)
		return 0;

	memset (cmd_line, 0x00, sizeof(cmd_line));
	if ((fp = fopen ("/sys/class/net/eth0/speed", "r")) != NULL) {
		memset (cmd_line, 0x00, sizeof(cmd_line));
		if (NULL != fgets (cmd_line, sizeof(cmd_line), fp)) {
			if (speed == atoi(cmd_line)) {
				fclose (fp);
				return -1;
			}
		}
		fclose (fp);
	}
	memset (cmd_line, 0x00, sizeof(cmd_line));
	sprintf(cmd_line,"ethtool -s eth0 speed %d duplex full 2<&1", speed);
	if ((fp = popen(cmd_line, "w")) != NULL)
		pclose(fp);

	sleep(3);
	if ((fp = fopen ("/sys/class/net/eth0/speed", "r")) != NULL) {
		memset (cmd_line, 0x00, sizeof(cmd_line));
		if (NULL != fgets (cmd_line, sizeof(cmd_line), fp)) {
			fprintf (stderr, "%s : change speed = %d, read speed = %d\n",
					__func__, speed, atoi(cmd_line));
			if (speed == atoi(cmd_line)) {
				fclose (fp);
				return 1;
			}
		}
		fclose (fp);
	}
	return 0;
}

//------------------------------------------------------------------------------
// apt install evetest
// /dev/input/event0, /sys/class/input/input0/name = fdd70030.pwm
// int change_eth_speed (int speed)
//------------------------------------------------------------------------------
int test_ir_ethled (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	static int fd = -1, led_orange = 1, led_green = 1;
	struct input_event event;

	struct timeval  timeout;
	fd_set readFds;

	/* Test complete */
	if (!led_orange && !led_green)
		return 1;

	// recive time out config
	// Set 1ms timeout counter
	timeout.tv_sec  = 0;
	// timeout.tv_usec = timeout_ms*1000;
	timeout.tv_usec = 100000;

	if (fd == -1) {
		fd = open ("/dev/input/event0", O_RDONLY);
		printf("%s fd = %d\n", __func__, fd);
	}
    FD_ZERO(&readFds);
    FD_SET(fd, &readFds);
    select(fd+1, &readFds, NULL, NULL, &timeout);

    if(FD_ISSET(fd, &readFds))
    {
		if(fd && read(fd, &event, sizeof(struct input_event))) {
			int changed = 0;
			switch (event.type) {
				case	EV_SYN:
					break;
				case	EV_KEY:
					switch (event.code) {
						case	KEY_ENTER:	case	KEY_HOME:	case	KEY_UP:
						case	KEY_LEFT:	case	KEY_RIGHT:	case	KEY_DOWN:
						case	KEY_MUTE:	case	KEY_MENU:	case	KEY_BACK:
						case	KEY_VOLUMEDOWN:		case	KEY_VOLUMEUP:
						case	KEY_POWER:	case	0:
							printf("event.type = %d, event.code = %d, event.value = %d\n",
								event.type, event.code, event.value);
							if ((event.code == KEY_VOLUMEDOWN) && event.value && led_green) {
								led_green = 0;
								ui_set_ritem (pfb, ui_grp, 162, COLOR_YELLOW, -1);
								if ((changed = change_eth_speed (100)) != -1)
									ui_set_ritem (pfb, ui_grp, 162,
												changed ? COLOR_GREEN : COLOR_RED, -1);
							}
							if ((event.code == KEY_VOLUMEUP) && event.value && led_orange && !led_green) {
								led_orange = 0;
								ui_set_ritem (pfb, ui_grp, 163, COLOR_YELLOW, -1);
								if ((changed = change_eth_speed (1000)) != -1)
									ui_set_ritem (pfb, ui_grp, 163,
												changed ? COLOR_GREEN : COLOR_RED, -1);
							}
							ui_set_sitem (pfb, ui_grp, 142,	-1, -1, "PASS");
							ui_set_ritem (pfb, ui_grp, 142,	COLOR_GREEN, -1);
						break;
						default :
							printf("unknown event\n");
						break;
					}
					break;
				default	:
					break;
			}
		}
    }
	return -1;
}

//------------------------------------------------------------------------------
// apt install evetest
// /dev/input/event2, /sys/class/input/input2/name = ODROID-M1-FRONT Headphones
//------------------------------------------------------------------------------
int test_hp_detect (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	static int fd = -1, jack_insert = 1, jack_remove = 1;
	struct input_event event;

	struct timeval  timeout;
	fd_set readFds;

	/* Test complete */
	if (!jack_insert && !jack_remove)
		return 1;

	// recive time out config
	// Set 1ms timeout counter
	timeout.tv_sec  = 0;
	// timeout.tv_usec = timeout_ms*1000;
	timeout.tv_usec = 100000;

	if (fd == -1) {
		fd = open ("/dev/input/event2", O_RDONLY);
		printf("%s fd = %d\n", __func__, fd);
	}
    FD_ZERO(&readFds);
    FD_SET(fd, &readFds);
    select(fd+1, &readFds, NULL, NULL, &timeout);

    if(FD_ISSET(fd, &readFds)) {
		if(fd && read(fd, &event, sizeof(struct input_event))) {
			switch (event.type) {
				case	EV_SYN:
					break;
				case	EV_SW:
					switch (event.code) {
						case	SW_HEADPHONE_INSERT:
							if ((event.value == 0) && jack_remove) {
								jack_remove = 0;
								ui_set_ritem (pfb, ui_grp, 183,	COLOR_GREEN, -1);
							}
							if ((event.value == 1) && jack_insert) {
								jack_insert = 0;
								ui_set_ritem (pfb, ui_grp, 182,	COLOR_GREEN, -1);
							}
						break;
						default :
							printf("unknown event\n");
						break;
					}
					break;
				default	:
					break;
			}
		}
    }
	return -1;
}

//------------------------------------------------------------------------------
int test_spi_button (fb_info_t *pfb, ui_grp_t *ui_grp)
{
	char mac_str[20];
	static int state = 0;

	/* test complete */
	if (state == 2)
		return 1;

	/* spi button press */
	if ((state == 0) && (get_efuse_mac(mac_str) == 0)) {
		state = 1;
		ui_set_ritem (pfb, ui_grp, 187, COLOR_GREEN, -1);
	}

	/* spi button release */
	if ((state == 1) && (get_efuse_mac(mac_str) == 1)) {
		state = 2;
		ui_set_ritem (pfb, ui_grp, 188, COLOR_GREEN, -1);
	}

	return -1;
}

//------------------------------------------------------------------------------
void test_status (fb_info_t *pfb, ui_grp_t *ui_grp, int status)
{
	static struct timeval t;
	static char blink = 0, timeover = 100;

	/* 1sec update */
	if (run_interval_check(&t, 1000)) {
		char msg[20];
		if (timeover == 0)
			status = eSTATUS_STOP;
		else
			timeover--;
		/* system state display "wait" */
		switch (status) {
			case	eSTATUS_WAIT:
				ui_set_sitem (pfb, ui_grp, 47, -1, -1, "WAIT");
				ui_set_ritem (pfb, ui_grp, 47, COLOR_GRAY, -1);
			break;
			case	eSTATUS_RUNNING:
				blink = !blink;
				memset (msg, 0, sizeof(msg));
				sprintf (msg, "RUNNING - %d", timeover);
				ui_set_sitem (pfb, ui_grp, 47, -1, -1, msg);
				ui_set_ritem (pfb, ui_grp, 47, blink ? RUN_BOX_ON : RUN_BOX_OFF, -1);
			break;
			case	eSTATUS_FINISH:
				macaddr_print ();
				ui_set_sitem (pfb, ui_grp, 47, -1, -1, "FINISH");
				ui_set_ritem (pfb, ui_grp, 47, COLOR_GREEN, -1);
				// Test complete.
				while (1)	sleep(1);
			break;
			default :
			case	eSTATUS_STOP:
				ui_set_sitem (pfb, ui_grp, 47, -1, -1, "STOP");
				ui_set_ritem (pfb, ui_grp, 47, COLOR_RED, -1);
				// Test complete.
				while (1)	sleep(1);
			break;
		}
		ui_update (pfb, ui_grp, -1);
	}
}

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	fb_info_t	*pfb;
	ui_grp_t 	*ui_grp;
	int			test_usb = 0, test_ir = 0, test_hp = 0, test_bt = 0;

	if ((pfb = fb_init (OPT_DEVICE_NAME)) == NULL) {
		fprintf(stdout, "ERROR: frame buffer init fail!\n");
		exit(1);
	}
    fb_cursor (0);

	if ((ui_grp = ui_init (pfb, OPT_FBUI_CFG)) == NULL) {
		fprintf(stdout, "ERROR: User interface create fail!\n");
		exit(1);
	}
	/* UI Init string */
	ui_update(pfb, ui_grp, -1);

	/* test storage, iperf, mac */
	test_status (pfb, ui_grp, bootup_test(pfb, ui_grp) ? eSTATUS_STOP : eSTATUS_RUNNING);

	while (1) {
		/* Test 중인경우 -1, Test 완료의 경우 0 or 1 */
		/* 각각의 테스트 함수는 static변수를 가지고 있고 테스트 완료시 테스트 skip함. */
		// usb speed, ir, eth led, headphone jack, spi bt
		if (test_usb != 1)	test_usb = test_usb_speed  (pfb, ui_grp);
		if (test_ir  != 1)	test_ir  = test_ir_ethled  (pfb, ui_grp);
		if (test_hp  != 1)	test_hp  = test_hp_detect  (pfb, ui_grp);
		if (test_bt  != 1)	test_bt  = test_spi_button (pfb, ui_grp);

		/* 일정시간 대기시까지 완료가 안되는 경우 강제로 완료 (2 MIN) */
		if ((test_usb == 1) && (test_ir == 1) && (test_hp == 1) && (test_bt == 1))
			test_status (pfb, ui_grp, eSTATUS_FINISH);
		else
			test_status (pfb, ui_grp, eSTATUS_RUNNING);

		/* 100ms loop delay */
		usleep(100000);
	}
	ui_close(ui_grp);
	fb_close (pfb);

	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
