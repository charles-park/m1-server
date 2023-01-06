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
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
/* UI Test ITEM & Result */
enum {
	eUI_IPERF_SPEED = 0,
	eUI_EFUSE_UUIDD,
	eUI_BOARD_MEM,
	eUI_FB_SIZE,
	eUI_EMMC_SPEED,
	eUI_SATA_SPEED,
	eUI_NVME_SPEED,
	eUI_USB30_UP,
	eUI_USB30_DN,
	eUI_USB20_UP,
	eUI_USB20_DN,
	eUI_ETH_GREEN,
	eUI_ETH_ORANGE,
	eUI_HP_IN,
	eUI_HP_OUT,
	eUI_SPIBT_DN,
	eUI_SPIBT_UP,
	eUI_IR_INPUT,
	eUI_ITEM_END
};

#define	TEST_RETRY_COUNT	5

#define	SETBIT_UI_ITEM(x)	(1<x)
#define	CLRBIT_UI_ITEM(x)  ~(1<x)

#define	RESPONSE_STR_SIZE	32
#define	ERROR_STR_SIZE		8

char BoardIP    [20] = {0,};
char NlpServerIP[20] = {0,};
char MacStr     [20] = {0,};
char EmergencyStop = 0;

struct m1_item {
	char		item_id;
	char		response_str[RESPONSE_STR_SIZE];
	char		status;
	char		result;
	char		ui_id;
	char		thread_enable;
	const char	error_str[ERROR_STR_SIZE];
};

struct m1_server {
	fb_info_t		*pfb;
	ui_grp_t		*pui;
	struct m1_item	*items;
};

struct m1_item	M1_Items[eUI_ITEM_END] = {
	{ eUI_IPERF_SPEED, "\0", eSTATUS_WAIT, 0, 147, 0, "IPERF" },
	{ eUI_EFUSE_UUIDD, "\0", eSTATUS_WAIT, 0, 167, 0, "EFUSE" },
	{ eUI_BOARD_MEM  , "\0", eSTATUS_WAIT, 0,   8, 0, "MEM" },
	{ eUI_FB_SIZE    , "\0", eSTATUS_WAIT, 0,  42, 0, "HDMI" },
	{ eUI_EMMC_SPEED , "\0", eSTATUS_WAIT, 0,  62, 0, "EMMC" },
	{ eUI_SATA_SPEED , "\0", eSTATUS_WAIT, 0,  82, 0, "SATA" },
	{ eUI_NVME_SPEED , "\0", eSTATUS_WAIT, 0,  87, 0, "NVME" },
	{ eUI_USB30_UP   , "\0", eSTATUS_WAIT, 0, 102, 0, "USB3U" },
	{ eUI_USB30_DN   , "\0", eSTATUS_WAIT, 0, 122, 0, "USB3D" },
	{ eUI_USB20_UP   , "\0", eSTATUS_WAIT, 0, 107, 0, "USB2U" },
	{ eUI_USB20_DN   , "\0", eSTATUS_WAIT, 0, 127, 0, "USB2D" },
	{ eUI_ETH_GREEN  , "\0", eSTATUS_WAIT, 0, 162, 0, "ETH_G" },
	{ eUI_ETH_ORANGE , "\0", eSTATUS_WAIT, 0, 163, 0, "ETH_O" },
	{ eUI_HP_IN      , "\0", eSTATUS_WAIT, 0, 182, 0, "HP_I" },
	{ eUI_HP_OUT     , "\0", eSTATUS_WAIT, 0, 183, 0, "HP_O" },
	{ eUI_SPIBT_DN   , "\0", eSTATUS_WAIT, 0, 187, 0, "BT_DN" },
	{ eUI_SPIBT_UP   , "\0", eSTATUS_WAIT, 0, 188, 0, "BT_UP" },
	{ eUI_IR_INPUT   , "\0", eSTATUS_WAIT, 0, 142, 0, "IR_IN" },
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
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
// function prototype define
//------------------------------------------------------------------------------
int		run_interval_check	(struct timeval *t, double interval_ms);
int		system_memory		(void);
int		change_eth_speed	(int speed);
void	macaddr_print		(void);
void 	errcode_print		(void);
int		get_efuse_mac		(char *mac_str);
int		write_efuse			(char *uuid);

void	*test_board_mem		(void *arg);
void	*test_emmc_speed	(void *arg);
void	*test_sata_speed	(void *arg);
void	*test_nvme_speed	(void *arg);
void	*test_iperf_speed	(void *arg);
void	*test_efuse_uuid	(void *arg);
void	*thread_ui_update 	(void *arg);

void	bootup_test			(fb_info_t *pfb, ui_grp_t *pui);
int		main				(int argc, char **argv);

//------------------------------------------------------------------------------
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
// change eth speed -> ethtool used
// eth speed change cmd (apt install ethtool), speed {100|1000}
// ethtool -s eth0 speed {speed} duplex full
// check eth link speed "/sys/class/net/eth0/speed"
//------------------------------------------------------------------------------
int change_eth_speed (int speed)
{
	FILE *fp;
	char cmd_line[128], retry = TEST_RETRY_COUNT;

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

	while (retry) {
		if ((fp = fopen ("/sys/class/net/eth0/speed", "r")) != NULL) {
			memset (cmd_line, 0x00, sizeof(cmd_line));
			if (NULL != fgets (cmd_line, sizeof(cmd_line), fp)) {
				if (speed == atoi(cmd_line)) {
					fclose (fp);
					return 1;
				}
				retry--;
				fprintf (stderr, "%s : change speed = %d, read speed = %d, retry remain = %d\n",
						__func__, speed, atoi(cmd_line), retry);
			}
			fclose (fp);
		}
		sleep (1);
	}
	return 0;
}

//------------------------------------------------------------------------------
void macaddr_print (void)
{
	/* mac address print */
	nlp_server_write (NlpServerIP, NLP_SERVER_MSG_TYPE_MAC, MacStr, 0);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	PRINT_MAX_CHAR	50
#define	PRINT_MAX_LINE	2

void errcode_print (void)
{
	char err_msg[PRINT_MAX_LINE][PRINT_MAX_CHAR+1];
	int pos = 0, i, line;

	memset (err_msg, 0, sizeof(err_msg));

	for (i = 0, line = 0; i < eUI_ITEM_END; i++) {
		if (!M1_Items[i].result) {
			if ((pos + strlen(M1_Items[i].error_str) + 1) > PRINT_MAX_CHAR) {
				pos = 0, line++;
			}
			pos += sprintf (&err_msg[line][pos], "%s,", M1_Items[i].error_str);
		}
	}
	if (pos || line) {
		for (i = 0; i < line+1; i++) {
			nlp_server_write (NlpServerIP, NLP_SERVER_MSG_TYPE_ERR, &err_msg[i][0], 0);
			printf ("%s : msg = %s\n", __func__, &err_msg[i][0]);
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const char	*EFUSE_UUID_FILE = "/sys/class/efuse/uuid";

int get_efuse_mac (char *mac_str)
{
	FILE *fp;
	char cmd_line[128];

	if (access (EFUSE_UUID_FILE, F_OK) != 0)
		return 0;

	if ((fp = fopen(EFUSE_UUID_FILE, "r")) != NULL) {
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
	return 0;
}

//------------------------------------------------------------------------------
void *test_board_mem (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;
	int mem = system_memory ();

	m1->status = eSTATUS_RUNNING;

	memset  (m1->response_str, 0x00, RESPONSE_STR_SIZE);
	sprintf (m1->response_str, "%d GB", mem);

	m1->result = mem ? 1 : 0;

	m1->status = eSTATUS_FINISH;
	return arg;
}

//------------------------------------------------------------------------------
void *test_emmc_speed (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;
	int speed = 0, retry = TEST_RETRY_COUNT;

	m1->status = eSTATUS_RUNNING;

	while ((retry--) && (speed < DEV_SPEED_EMMC)) {
		memset (m1->response_str, 0x00, RESPONSE_STR_SIZE);
		speed = storage_test ("emmc", m1->response_str);
	}
	m1->result = speed < DEV_SPEED_EMMC ? 0 : 1;

	m1->status = eSTATUS_FINISH;
	return arg;
}

//------------------------------------------------------------------------------
void *test_sata_speed (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;
	int speed = 0, retry = TEST_RETRY_COUNT;

	m1->status = eSTATUS_RUNNING;

	while ((retry--) && (speed < DEV_SPEED_SATA)) {
		memset (m1->response_str, 0x00, RESPONSE_STR_SIZE);
		speed = storage_test ("sata", m1->response_str);
	}
	m1->result = speed < DEV_SPEED_SATA ? 0 : 1;

	m1->status = eSTATUS_FINISH;
	return arg;
}

//------------------------------------------------------------------------------
void *test_nvme_speed (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;
	int speed = 0, retry = TEST_RETRY_COUNT;

	m1->status = eSTATUS_RUNNING;

	while ((retry--) && (speed < DEV_SPEED_NVME)) {
		memset (m1->response_str, 0x00, RESPONSE_STR_SIZE);
		speed = storage_test ("nvme", m1->response_str);
	}
	m1->result = speed < DEV_SPEED_NVME ? 0 : 1;

	m1->status = eSTATUS_FINISH;
	return arg;
}

//------------------------------------------------------------------------------
volatile char IperfTestFlag = 0;

void *test_iperf_speed (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;
	int speed = 0, retry = TEST_RETRY_COUNT * 2;

	m1->status = eSTATUS_RUNNING;
	IperfTestFlag = 1;
	// UDP = 3, TCP = 4
	nlp_server_write   (NlpServerIP, NLP_SERVER_MSG_TYPE_UDP, "start", 0);
	sleep(1);
	while ((retry--) && (speed < IPERF_SPEED)) {
		speed = iperf3_speed_check (NlpServerIP, NLP_SERVER_MSG_TYPE_UDP);
		printf ("iperf result : retry = %d, speed = %d Mbits/s\n", retry, speed);
	}
	sleep(1);
	nlp_server_write   (NlpServerIP, NLP_SERVER_MSG_TYPE_UDP, "stop", 0);

	memset  (m1->response_str, 0x00, RESPONSE_STR_SIZE);
	sprintf (m1->response_str, "%d MBits/sec", speed);
	m1->result = speed > IPERF_SPEED ? 1 : 0;

	IperfTestFlag = 0;
	m1->status = eSTATUS_FINISH;
	return arg;
}

//------------------------------------------------------------------------------
void *test_efuse_uuid (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;

	m1->status = eSTATUS_RUNNING;

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
	memset (m1->response_str, 0x00, RESPONSE_STR_SIZE);
	if (!strncmp (MacStr, "001e06", strlen("001e06"))) {
		sprintf (m1->response_str,"00:1e:06:%c%c:%c%c:%c%c",
			MacStr[6],	MacStr[7],	MacStr[8],	MacStr[9],	MacStr[10], MacStr[11]);
		m1->result = 1;
	} else {
		sprintf (m1->response_str,"%s", "unknown mac");
		m1->result = 0;
	}

	m1->status = eSTATUS_FINISH;

	return arg;
}

//------------------------------------------------------------------------------
// apt install evetest
// /dev/input/event2, /sys/class/input/input2/name = ODROID-M1-FRONT Headphones
//------------------------------------------------------------------------------
/* 0 : event none, 1 : insert, 2 : remove */
volatile char HP_Event = 0;

void *test_hp_detect (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;

	memset (m1->response_str, 0x00, sizeof(RESPONSE_STR_SIZE));

	while (m1->status != eSTATUS_FINISH) {
		switch(m1->item_id) {
			case eUI_HP_IN:
				if (HP_Event == 1) {
					m1->result = 1;
					m1->status = eSTATUS_FINISH;
				}
			break;
			case eUI_HP_OUT:
				if (HP_Event == 2) {
					m1->result = 1;
					m1->status = eSTATUS_FINISH;
				}
			break;
		}
		usleep(1000);
	}
	return arg;
}

//------------------------------------------------------------------------------
// apt install evetest
// /dev/input/event0, /sys/class/input/input0/name = fdd70030.pwm
// int change_eth_speed (int speed)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/* 0 : event none, 1 : event pass */
volatile char IR_Event = 0;

void *test_ir_input (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;
	while (!IR_Event)	sleep(1);

	memset (m1->response_str, 0x00, sizeof(RESPONSE_STR_SIZE));
	sprintf (m1->response_str, "%s", "PASS");
	m1->status = eSTATUS_FINISH;
	m1->result = 1;
	return arg;
}

//------------------------------------------------------------------------------
/* 0 : event none, 1 : 100Mbps(GREEN) - fail, 2 : 100Mbps(GREEN) - pass */
/*                 3 : 1Gbps(ORANGE)  - fail, 4 : 1Gbps(ORANGE)  - pass */
/*                 5 : 100Mbps(GREEN) - run , 6 : 1Gbps(ORANGE)  - run */
volatile char IR_ETH_Event = 0;

void *test_eth_change (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;

	memset (m1->response_str, 0x00, sizeof(RESPONSE_STR_SIZE));

	while (m1->status != eSTATUS_FINISH) {
		switch(m1->item_id) {
			case eUI_ETH_GREEN:
				if (IR_ETH_Event == 5)
					m1->status = eSTATUS_RUNNING;
				if ((IR_ETH_Event == 1) || (IR_ETH_Event == 2)) {
					m1->result = (IR_ETH_Event == 2) ? 1 : 0;
					m1->status = eSTATUS_FINISH;
				}
			break;
			case eUI_ETH_ORANGE:
				if (IR_ETH_Event == 6)
					m1->status = eSTATUS_RUNNING;
				if ((IR_ETH_Event == 3) || (IR_ETH_Event == 4)) {
					m1->result = (IR_ETH_Event == 4) ? 1 : 0;
					m1->status = eSTATUS_FINISH;
				}
			break;
		}
		usleep(100000);
	}
	return arg;
}

//------------------------------------------------------------------------------
/* 0 : event none, 1 : BT Press, 2 : BT Release */
volatile char BT_Event = 0;

void *test_spibt_input (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;

	memset (m1->response_str, 0x00, sizeof(RESPONSE_STR_SIZE));

	while (m1->status != eSTATUS_FINISH) {
		switch(m1->item_id) {
			case eUI_SPIBT_DN:
				if (BT_Event == 1) {
					m1->result = 1;
					m1->status = eSTATUS_FINISH;
				}
			break;
			case eUI_SPIBT_UP:
				if (BT_Event == 2) {
					m1->result = 1;
					m1->status = eSTATUS_FINISH;
				}
			break;
		}
		usleep(1000);
	}
	return arg;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/*
	find /sys/devices/platform -name *sd* | grep 8-1 2<&1
	strstr "/block/sd"
	usb detect speed = /sys/bus/usb/devices/{usb_device_name}/speed
	get node (ptr + 9) = 'a,b,c,d,....'
	apt install usbutils (lsusb -t...)
*/
//------------------------------------------------------------------------------
/* 0 : event none, 1 : BUSY */
volatile char USB_Event = 0;

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
void *test_usb_speed (void *arg)
{
	struct m1_item *m1 = (struct m1_item *)arg;
	int item_cnt, i, usb_prev_check = -1, usb_detect_cnt;;
	char fname[256];

	item_cnt = sizeof(USB_DEVICE_NAME) / sizeof(USB_DEVICE_NAME[0]);

	while (!EmergencyStop && (m1->status != eSTATUS_FINISH)) {
		for (i = 0, usb_detect_cnt = 0; i < item_cnt; i++) {
			memset (fname, 0, sizeof(fname));
			sprintf (fname, "%s/%s", USB_DEVICE_DIR, USB_DEVICE_NAME[i]);
			if (access (fname, F_OK) == 0) {
				FILE *fp;
				int usb_det_speed = 0;

				usb_detect_cnt++;
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
					int speed = -1;

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
							if (m1->item_id == eUI_USB30_UP) {
								m1->status = eSTATUS_RUNNING;
								if (usb_det_speed > 12)
									speed = storage_read_test (fname, usb_det_speed > 480 ? 5 : 1);
								m1->result = (speed > USB30_MASS_SPEED) ? 1 : 0;
								m1->status = m1->result ? eSTATUS_FINISH : eSTATUS_STOP;
								if (speed != -1) {
									memset  (m1->response_str, 0x00, RESPONSE_STR_SIZE);
									sprintf (m1->response_str, "%dM - %d MB/s", usb_det_speed, speed);
								}
							}
						break;
						case 2:	case 3:
							if (m1->item_id == eUI_USB30_DN) {
								m1->status = eSTATUS_RUNNING;
								if (usb_det_speed > 12)
									speed = storage_read_test (fname, usb_det_speed > 480 ? 5 : 1);
								m1->result = (speed > USB30_MASS_SPEED) ? 1 : 0;
								m1->status = m1->result ? eSTATUS_FINISH : eSTATUS_STOP;
								if (speed != -1) {
									memset  (m1->response_str, 0x00, RESPONSE_STR_SIZE);
									sprintf (m1->response_str, "%dM - %d MB/s", usb_det_speed, speed);
								}
							}
						break;
						case 4:	case 5:
							if (m1->item_id == eUI_USB20_UP) {
								m1->status = eSTATUS_RUNNING;
								if (usb_det_speed > 12)
									speed = storage_read_test (fname, usb_det_speed > 12 ? 5 : 1);
								m1->result = (speed > USB20_MASS_SPEED) ? 1 : 0;
								m1->status = m1->result ? eSTATUS_FINISH : eSTATUS_STOP;
								if (speed != -1) {
									memset  (m1->response_str, 0x00, RESPONSE_STR_SIZE);
									sprintf (m1->response_str, "%dM - %d MB/s", usb_det_speed, speed);
								}
							}
						break;
						case 6:	case 7:
							if (m1->item_id == eUI_USB20_DN) {
								m1->status = eSTATUS_RUNNING;
								if (usb_det_speed > 12)
									speed = storage_read_test (fname, usb_det_speed > 12 ? 5 : 1);
								m1->result = (speed > USB20_MASS_SPEED) ? 1 : 0;
								m1->status = m1->result ? eSTATUS_FINISH : eSTATUS_STOP;
								if (speed != -1) {
									memset  (m1->response_str, 0x00, RESPONSE_STR_SIZE);
									sprintf (m1->response_str, "%dM - %d MB/s", usb_det_speed, speed);
								}
							}
						break;
						default :
						break;
					}
				}
			}
		}
		// remove all usb port
		if (!usb_detect_cnt)
			usb_prev_check = -1;
		usleep(1000);
	}
	return	arg;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TIMEOVER_COUNT	90

void *thread_ui_update (void *arg)
{
	struct m1_server *m1_server = (struct m1_server *)arg;
	int i, timeover = TIMEOVER_COUNT, fin_cnt, loop_cnt = 0;;

	/* default status */
	ui_set_sitem (m1_server->pfb, m1_server->pui, 47, -1, -1, "WAIT");
	ui_set_ritem (m1_server->pfb, m1_server->pui, 47, COLOR_GRAY, -1);

	while (!EmergencyStop && timeover) {
		for (i = 0, fin_cnt = 0; i < eUI_ITEM_END; i++) {
			if (m1_server->items[i].status != eSTATUS_WAIT) {
				switch (m1_server->items[i].status) {
					case eSTATUS_RUNNING:
						ui_set_ritem (m1_server->pfb, m1_server->pui,
							m1_server->items[i].ui_id, COLOR_YELLOW, -1);
					break;
					case eSTATUS_STOP:
					case eSTATUS_FINISH:
						ui_set_ritem (m1_server->pfb, m1_server->pui,
							m1_server->items[i].ui_id,
							m1_server->items[i].result ? COLOR_GREEN : COLOR_RED, -1);
						if (m1_server->items[i].response_str[0] != 0) {
							ui_set_sitem (m1_server->pfb, m1_server->pui,
								m1_server->items[i].ui_id, -1, -1,
								m1_server->items[i].response_str);
						}
					break;
				}
			}
			if (m1_server->items[i].status == eSTATUS_FINISH)
				fin_cnt++;
		}

		if (fin_cnt) {
			if (fin_cnt == eUI_ITEM_END) {
				int error_cnt;
				for (i = 0, error_cnt = 0; i < eUI_ITEM_END; i++) {
					if (!m1_server->items[i].result) {
						error_cnt++;
						printf ("%s : item err = %s\n", __func__, m1_server->items[i].error_str);
					}
				}
				ui_set_sitem (m1_server->pfb, m1_server->pui, 47, -1, -1, "FINISH");
				ui_set_ritem (m1_server->pfb, m1_server->pui, 47,
										error_cnt ? COLOR_RED : COLOR_GREEN, -1);
				break;
			} else {
				char status_msg[32];
				memset  (status_msg, 0x00, sizeof(status_msg));
				sprintf (status_msg, "RUNNING - %d", timeover);
				ui_set_sitem (m1_server->pfb, m1_server->pui, 47, -1, -1, status_msg);
				ui_set_ritem (m1_server->pfb, m1_server->pui, 47,
					((loop_cnt % 2) == 0) ? RUN_BOX_ON : RUN_BOX_OFF, -1);
				ui_update (m1_server->pfb, m1_server->pui, -1);
			}
		}
		if ((loop_cnt++ % 2) == 0)
			timeover--;

		usleep(500000);
	}

	macaddr_print ();	errcode_print ();

	if (EmergencyStop || !timeover) {
		ui_set_sitem (m1_server->pfb, m1_server->pui, 47, -1, -1, "STOP");
		ui_set_ritem (m1_server->pfb, m1_server->pui, 47, COLOR_RED, -1);
	}

	while (1) {
		EmergencyStop = 1;
		fprintf(stdout, "%s finish...\n", __func__);
		fflush (stdout);
		sleep(1);
		ui_update (m1_server->pfb, m1_server->pui, -1);
	}
	return arg;
}

//------------------------------------------------------------------------------
void bootup_test (fb_info_t *pfb, ui_grp_t *pui)
{
	int retry;

	memset (BoardIP, 0, sizeof(BoardIP));
	memset (NlpServerIP, 0, sizeof(NlpServerIP));
	memset (MacStr, 0, sizeof(MacStr));

	/* default network speed GBits/sec */
	change_eth_speed (1000);

	while (!get_my_ip (BoardIP)) {
		memset (BoardIP, 0, sizeof(BoardIP));
		sprintf(BoardIP, "%s", "Network Error!");
		ui_set_sitem (pfb, pui, 4, -1, -1, BoardIP);
		ui_set_ritem (pfb, pui, 4, (retry++ % 2)
			? COLOR_RED : COLOR_DIM_GRAY, -1);
		sleep(1);
	}
	ui_set_sitem (pfb, pui, 4, -1, -1, BoardIP);
	ui_set_ritem (pfb, pui, 4, COLOR_GREEN, -1);

	while (!nlp_server_find (NlpServerIP)) {
		memset (NlpServerIP, 0, sizeof(NlpServerIP));
		sprintf(NlpServerIP, "%s", "Network Error!");
		ui_set_sitem (pfb, pui, 24, -1, -1, NlpServerIP);
		ui_set_ritem (pfb, pui, 24, (retry++ % 2)
			? COLOR_RED : COLOR_DIM_GRAY, -1);
		sleep(1);
	}
	ui_set_sitem (pfb, pui, 24, -1, -1, NlpServerIP);
	ui_set_ritem (pfb, pui, 24, COLOR_GREEN, -1);

	// thread run
	M1_Items[eUI_FB_SIZE].status = eSTATUS_RUNNING;
	memset  (M1_Items[eUI_FB_SIZE].response_str, 0x00, RESPONSE_STR_SIZE);
	sprintf (M1_Items[eUI_FB_SIZE].response_str, "%d x %d", pfb->w, pfb->h);
	if ((pfb->w != 1920) || (pfb->h != 1080))
		M1_Items[eUI_FB_SIZE].result = 0;
	else
		M1_Items[eUI_FB_SIZE].result = 1;
	M1_Items[eUI_FB_SIZE].status = eSTATUS_FINISH;

}

//------------------------------------------------------------------------------
void test_thread_run (void)
{
	pthread_t thread_iperf, thread_efuse;
	pthread_t thread_board_mem, thread_emmc, thread_sata, thread_nvme;
	pthread_t thread_hp_in, thread_hp_out, thread_ir_input;
	pthread_t thread_eth_green, thread_eth_orange;
	pthread_t thread_spibt_up, thread_spibt_dn;
	pthread_t thread_usb30_up, thread_usb30_dn, thread_usb20_up, thread_usb20_dn;

	pthread_create (&thread_efuse, NULL, test_efuse_uuid,  &M1_Items[eUI_EFUSE_UUIDD]);
	pthread_create (&thread_iperf, NULL, test_iperf_speed, &M1_Items[eUI_IPERF_SPEED]);

	pthread_create (&thread_board_mem, NULL, test_board_mem,  &M1_Items[eUI_BOARD_MEM]);
	pthread_create (&thread_emmc, NULL, test_emmc_speed, &M1_Items[eUI_EMMC_SPEED]);
	pthread_create (&thread_sata, NULL, test_sata_speed, &M1_Items[eUI_SATA_SPEED]);
	pthread_create (&thread_nvme, NULL, test_nvme_speed, &M1_Items[eUI_NVME_SPEED]);

	pthread_create (&thread_hp_in,  NULL, test_hp_detect, &M1_Items[eUI_HP_IN]);
	pthread_create (&thread_hp_out, NULL, test_hp_detect, &M1_Items[eUI_HP_OUT]);

	pthread_create (&thread_ir_input, NULL, test_ir_input, &M1_Items[eUI_IR_INPUT]);

	pthread_create (&thread_eth_green,  NULL, test_eth_change, &M1_Items[eUI_ETH_GREEN]);
	pthread_create (&thread_eth_orange, NULL, test_eth_change, &M1_Items[eUI_ETH_ORANGE]);

	pthread_create (&thread_spibt_up, NULL, test_spibt_input, &M1_Items[eUI_SPIBT_UP]);
	pthread_create (&thread_spibt_dn, NULL, test_spibt_input, &M1_Items[eUI_SPIBT_DN]);

	pthread_create (&thread_usb30_up, NULL, test_usb_speed, &M1_Items[eUI_USB30_UP]);
	pthread_create (&thread_usb30_dn, NULL, test_usb_speed, &M1_Items[eUI_USB30_DN]);
	pthread_create (&thread_usb20_up, NULL, test_usb_speed, &M1_Items[eUI_USB20_UP]);
	pthread_create (&thread_usb20_dn, NULL, test_usb_speed, &M1_Items[eUI_USB20_DN]);

	#if 0
	test_iperf_speed (&M1_Items[eUI_IPERF_SPEED]);
	#endif
}

//------------------------------------------------------------------------------
void *thread_ir_event (void *arg)
{
	struct input_event event;

	struct timeval  timeout;
	fd_set readFds;
	static int fd = -1, green_test = 0, orange_test = 0;

	if ((fd = open("/dev/input/event0", O_RDONLY)) < 0) {
		printf ("%s : /dev/input/event0 open error!\n", __func__);
		return arg;
	}
	printf("%s fd = %d\n", __func__, fd);

	while (!EmergencyStop) {
		// recive time out config
		// Set 1ms timeout counter
		timeout.tv_sec  = 0;
		// timeout.tv_usec = timeout_ms*1000;
		timeout.tv_usec = 100000;

		FD_ZERO(&readFds);
		FD_SET(fd, &readFds);
		select(fd+1, &readFds, NULL, NULL, &timeout);

		if(FD_ISSET(fd, &readFds))
		{
			if(fd && read(fd, &event, sizeof(struct input_event))) {
				int changed = -1;
				switch (event.type) {
					case	EV_SYN:
						break;
					case	EV_KEY:
						IR_Event = 1;
						switch (event.code) {
							/* emergency stop */
							case	KEY_HOME:
								EmergencyStop = 1;
								printf ("%s : EmergencyStop!!\n", __func__);
							break;
							case	KEY_VOLUMEDOWN:
								if (green_test || IperfTestFlag)
									break;
								IR_ETH_Event = 5;
								if ((changed = change_eth_speed(100)) != -1) {
									green_test = 1;
									IR_ETH_Event = changed ? 2 : 1;
								}
							break;
							case	KEY_VOLUMEUP:
								if (!green_test || orange_test || IperfTestFlag)
									break;
								IR_ETH_Event = 6;
								if ((changed = change_eth_speed(1000)) != -1) {
									orange_test = 1;
									IR_ETH_Event = changed ? 4 : 3;
								}
							break;
							default :
							break;
						}
						break;
					default	:
						IR_Event = 0;
						printf("unknown event\n");
						break;
				}
			}
		}
		usleep(100000);
	}
	return arg;
}

//------------------------------------------------------------------------------
void *thread_hp_event (void *arg)
{
	struct input_event event;
	struct timeval  timeout;
	fd_set readFds;
	int fd;

	if ((fd = open("/dev/input/event2", O_RDONLY)) < 0) {
		printf ("%s : /dev/input/event2 open error!\n", __func__);
		return arg;
	}
	printf("%s fd = %d\n", __func__, fd);

	while (!EmergencyStop) {
		// recive time out config
		// Set 1ms timeout counter
		timeout.tv_sec  = 0;
		// timeout.tv_usec = timeout_ms*1000;
		timeout.tv_usec = 100000;

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
								HP_Event = event.value ? 1 : 2;
							break;
							default :
								HP_Event = 0;
							break;
						}
						break;
					default	:
						break;
				}
			}
		}
		usleep(1000);
	}
	return arg;
}

//------------------------------------------------------------------------------
void *thread_bt_event (void *arg)
{
	char mac_str[20];

	/* state 0 : press wait, state 1 : release wait*/
	int bt_state = 0;

	while (!EmergencyStop && (bt_state != 2)) {
		switch (bt_state) {
			case	0:
				// key_press wait
				if(get_efuse_mac(mac_str) == 0) {
					bt_state = 1;	BT_Event = 1;
				}
			break;
			case	1:
				// key_release wait
				if(get_efuse_mac(mac_str) == 1) {
					bt_state = 2;	BT_Event = 2;
				}
			break;
			default	:
			break;
		}
		usleep(200000);
	}
	return arg;
}

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	pthread_t ui_thread, ir_thread, hp_thread, bt_thread;

	struct m1_server m1_server;
	fb_info_t	*pfb;
	ui_grp_t 	*pui;

	if ((pfb = fb_init (OPT_DEVICE_NAME)) == NULL) {
		fprintf(stdout, "ERROR: frame buffer init fail!\n");
		exit(1);
	}
    fb_cursor (0);

	if ((pui = ui_init (pfb, OPT_FBUI_CFG)) == NULL) {
		fprintf(stdout, "ERROR: User interface create fail!\n");
		exit(1);
	}
	ui_update(pfb, pui, -1);

	m1_server.items = &M1_Items[0];
	m1_server.pfb   = pfb;
	m1_server.pui   = pui;

	/* UI Thread running */
	pthread_create(&ui_thread, NULL, thread_ui_update, &m1_server);
	bootup_test(pfb, pui);

	/* IR Thread running */
	pthread_create(&ir_thread, NULL, thread_ir_event, &m1_server);
	/* HP Thread running */
	pthread_create(&hp_thread, NULL, thread_hp_event, &m1_server);
	/* SPI Button Thread rinning */
	pthread_create(&bt_thread, NULL, thread_bt_event, &m1_server);

	test_thread_run ();

	while (1)
		sleep(1);

	ui_close(pui);
	fb_close (pfb);

	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
