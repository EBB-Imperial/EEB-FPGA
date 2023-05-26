
#include <stdio.h>
#include "I2C_core.h"
#include "terasic_includes.h"
#include "mipi_camera_config.h"
#include "mipi_bridge_config.h"

#include "auto_focus.h"

#include <fcntl.h>
#include <unistd.h>

#include <altera_avalon_uart.h>

//EEE_IMGPROC defines
#define EEE_IMGPROC_MSG_START ('R'<<16 | 'B'<<8 | 'B')

//offsets
#define EEE_IMGPROC_STATUS 0
#define EEE_IMGPROC_MSG 1
#define EEE_IMGPROC_ID 2
#define EEE_IMGPROC_BBCOL 3

#define EXPOSURE_INIT 0x002000
#define EXPOSURE_STEP 0x100
#define GAIN_INIT 0x080
#define GAIN_STEP 0x040
#define DEFAULT_LEVEL 3

#define MIPI_REG_PHYClkCtl		0x0056
#define MIPI_REG_PHYData0Ctl	0x0058
#define MIPI_REG_PHYData1Ctl	0x005A
#define MIPI_REG_PHYData2Ctl	0x005C
#define MIPI_REG_PHYData3Ctl	0x005E
#define MIPI_REG_PHYTimDly		0x0060
#define MIPI_REG_PHYSta			0x0062
#define MIPI_REG_CSIStatus		0x0064
#define MIPI_REG_CSIErrEn		0x0066
#define MIPI_REG_MDLSynErr		0x0068
#define MIPI_REG_FrmErrCnt		0x0080
#define MIPI_REG_MDLErrCnt		0x0090

void mipi_clear_error(void) {
	MipiBridgeRegWrite(MIPI_REG_CSIStatus, 0x01FF); // clear error
	MipiBridgeRegWrite(MIPI_REG_MDLSynErr, 0x0000); // clear error
	MipiBridgeRegWrite(MIPI_REG_FrmErrCnt, 0x0000); // clear error
	MipiBridgeRegWrite(MIPI_REG_MDLErrCnt, 0x0000); // clear error

	MipiBridgeRegWrite(0x0082, 0x00);
	MipiBridgeRegWrite(0x0084, 0x00);
	MipiBridgeRegWrite(0x0086, 0x00);
	MipiBridgeRegWrite(0x0088, 0x00);
	MipiBridgeRegWrite(0x008A, 0x00);
	MipiBridgeRegWrite(0x008C, 0x00);
	MipiBridgeRegWrite(0x008E, 0x00);
	MipiBridgeRegWrite(0x0090, 0x00);
}

void mipi_show_error_info(void) {

	alt_u16 PHY_status, SCI_status, MDLSynErr, FrmErrCnt, MDLErrCnt;

	PHY_status = MipiBridgeRegRead(MIPI_REG_PHYSta);
	SCI_status = MipiBridgeRegRead(MIPI_REG_CSIStatus);
	MDLSynErr = MipiBridgeRegRead(MIPI_REG_MDLSynErr);
	FrmErrCnt = MipiBridgeRegRead(MIPI_REG_FrmErrCnt);
	MDLErrCnt = MipiBridgeRegRead(MIPI_REG_MDLErrCnt);
	printf(
			"PHY_status=%xh, CSI_status=%xh, MDLSynErr=%xh, FrmErrCnt=%xh, MDLErrCnt=%xh\r\n",
			PHY_status, SCI_status, MDLSynErr, FrmErrCnt, MDLErrCnt);
}

void mipi_show_error_info_more(void) {
	printf("FrmErrCnt = %d\n", MipiBridgeRegRead(0x0080));
	printf("CRCErrCnt = %d\n", MipiBridgeRegRead(0x0082));
	printf("CorErrCnt = %d\n", MipiBridgeRegRead(0x0084));
	printf("HdrErrCnt = %d\n", MipiBridgeRegRead(0x0086));
	printf("EIDErrCnt = %d\n", MipiBridgeRegRead(0x0088));
	printf("CtlErrCnt = %d\n", MipiBridgeRegRead(0x008A));
	printf("SoTErrCnt = %d\n", MipiBridgeRegRead(0x008C));
	printf("SynErrCnt = %d\n", MipiBridgeRegRead(0x008E));
	printf("MDLErrCnt = %d\n", MipiBridgeRegRead(0x0090));
	printf("FIFOSTATUS = %d\n", MipiBridgeRegRead(0x00F8));
	printf("DataType = 0x%04x\n", MipiBridgeRegRead(0x006A));
	printf("CSIPktLen = %d\n", MipiBridgeRegRead(0x006E));
}

bool MIPI_Init(void) {
	bool bSuccess;

	bSuccess = oc_i2c_init_ex(I2C_OPENCORES_MIPI_BASE, 50 * 1000 * 1000,
			400 * 1000); //I2C: 400K
	if (!bSuccess)
		printf("failed to init MIPI- Bridge i2c\r\n");

	usleep(50 * 1000);
	MipiBridgeInit();

	usleep(500 * 1000);

	MipiCameraInit();
	MIPI_BIN_LEVEL(DEFAULT_LEVEL);

	usleep(1000);

	return bSuccess;
}

int main() {

	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

	printf("DE10-LITE D8M VGA Demo\n");
	printf("Imperial College EEE2 Project version\n");
	IOWR(MIPI_PWDN_N_BASE, 0x00, 0x00);
	IOWR(MIPI_RESET_N_BASE, 0x00, 0x00);

	usleep(2000);
	IOWR(MIPI_PWDN_N_BASE, 0x00, 0xFF);
	usleep(2000);
	IOWR(MIPI_RESET_N_BASE, 0x00, 0xFF);

	//printf("Image Processor ID: %x\n",IORD(0x42000,EEE_IMGPROC_ID));
	printf("Image Processor ID: %x\n",
			IORD(EEE_IMGPROC_0_BASE, EEE_IMGPROC_ID)); //Don't know why this doesn't work - definition is in system.h in BSP

	usleep(2000);

	// MIPI Init
	if (!MIPI_Init()) {
		printf("MIPI_Init Init failed!\r\n");
	} else {
		printf("MIPI_Init Init successfully!\r\n");
	}

	mipi_clear_error();
	usleep(50 * 1000);
	mipi_clear_error();
	usleep(500 * 1000);
	mipi_show_error_info();

	printf("\n");

	alt_u16 bin_level = DEFAULT_LEVEL;
	alt_u8 manual_focus_step = 10;
	alt_u16 current_focus = 300;
	alt_u32 exposureTime = EXPOSURE_INIT;
	alt_u16 gain = GAIN_INIT;

	OV8865SetExposure(exposureTime);
	OV8865SetGain(gain);
	Focus_Init();

	FILE* esp32_uart = fopen(UART_0_NAME, "w");
	if (!esp32_uart) {
		printf("Failed to open ESP UART\n");
		while(1) ;
	} else {
		printf("Opened ESP UART\n");
	}

	while (1) {

		// touch KEY0 to trigger Auto focus
		if (!(IORD(KEY_BASE,0) & 0x02)) {
			current_focus = Focus_Window(320, 240);
		}

		// touch KEY1 to ZOOM
		if (!(IORD(KEY_BASE,0) & 0x01)) {
			if (bin_level == 3) {
				bin_level = 1;
			} else {
				bin_level++;
			}
			printf("set bin level to %d\n", bin_level);
			MIPI_BIN_LEVEL(bin_level);
			usleep(500000);
		}

		//Read messages from the image processor and print them on the terminal
		//IOWR(EEE_IMGPROC_0_BASE, EEE_IMGPROC_STATUS, 0x0010); // Flush previous messages
		while (IORD(EEE_IMGPROC_0_BASE, EEE_IMGPROC_MSG) != EEE_IMGPROC_MSG_START) {
			// putchar('.');
		}
		//putchar('\n');
		alt_u32 msg_buf[2];
		msg_buf[0] = IORD(EEE_IMGPROC_0_BASE, EEE_IMGPROC_MSG);
		msg_buf[1] = IORD(EEE_IMGPROC_0_BASE, EEE_IMGPROC_MSG);

		alt_u16 left = (msg_buf[0] >> 16) & 0x07ff;
		alt_u16 top = msg_buf[0] & 0x07ff;
		alt_u16 right = (msg_buf[1] >> 16) & 0x07ff;
		alt_u16 bottom = msg_buf[1] & 0x07ff;
		alt_u16 mid_x = (left + right) / 2;
		alt_u16 mid_y = (top + bottom) / 2;
		printf("bounding box: (%i, %i) (%i, %i);  center: (%i, %i)\n", left, top, right, bottom, mid_x, mid_y);
		fprintf(esp32_uart, "*%03i,%03i\n", mid_x, mid_y);

		/*
		//Process input commands
		int in = getchar();
		switch (in) {
		case 'e':
			exposureTime += EXPOSURE_STEP;
			OV8865SetExposure(exposureTime);
			printf("\nExposure = %x ", exposureTime);
			break;
		case 'd':
			exposureTime -= EXPOSURE_STEP;
			OV8865SetExposure(exposureTime);
			printf("\nExposure = %x ", exposureTime);
			break;
		case 't':
			gain += GAIN_STEP;
			OV8865SetGain(gain);
			printf("\nGain = %x ", gain);
			break;
		case 'g':
			gain -= GAIN_STEP;
			OV8865SetGain(gain);
			printf("\nGain = %x ", gain);
			break;
		case 'r':
			current_focus += manual_focus_step;
			if (current_focus > 1023)
				current_focus = 1023;
			OV8865_FOCUS_Move_to(current_focus);
			printf("\nFocus = %x ", current_focus);
			break;
		case 'f':
			if (current_focus > manual_focus_step)
				current_focus -= manual_focus_step;
			OV8865_FOCUS_Move_to(current_focus);
			printf("\nFocus = %x ", current_focus);
			break;
		}
		*/
	};

	return 0;
}
