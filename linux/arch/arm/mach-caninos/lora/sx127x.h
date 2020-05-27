#ifndef SX127X_H_
#define SX127X_H_

#include <linux/types.h>

enum sx127x_modulation
{
	SX127X_MODULATION_FSK     = 0,
	SX127X_MODULATION_OOK     = 1,
	SX127X_MODULATION_LORA    = 2,
	SX127X_MODULATION_INVALID = 3,
};

enum sx127x_opmode
{
	SX127X_OPMODE_SLEEP        = 0,
	SX127X_OPMODE_STANDBY      = 1,
	SX127X_OPMODE_FSTX         = 2,
	SX127X_OPMODE_TX           = 3,
	SX127X_OPMODE_FSRX         = 4,
	SX127X_OPMODE_RX           = 5, /* only valid in FSK/OOK mode */
	SX127X_OPMODE_RXCONTINUOUS = 6, /* only valid in LoRa mode */
	SX127X_OPMODE_RXSINGLE     = 7, /* only valid in LoRa mode */
	SX127X_OPMODE_CAD          = 8, /* only valid in LoRa mode */
	SX127X_OPMODE_INVALID      = 9,
};

enum sx127x_pa
{
	SX127X_PA_RFO     = 0,
	SX127X_PA_PABOOST = 1,
};

enum sx127x_ioctl_cmd {
	SX127X_IOCTL_CMD_GETMODULATION       = 0,
	SX127X_IOCTL_CMD_SETMODULATION       = 1,
	SX127X_IOCTL_CMD_GETCARRIERFREQUENCY = 2,
	SX127X_IOCTL_CMD_SETCARRIERFREQUENCY = 3,
	SX127X_IOCTL_CMD_GETSF               = 4,
	SX127X_IOCTL_CMD_SETSF               = 5,
	SX127X_IOCTL_CMD_GETOPMODE           = 6,
	SX127X_IOCTL_CMD_SETOPMODE           = 7,
	SX127X_IOCTL_CMD_GETPAOUTPUT         = 8,
	SX127X_IOCTL_CMD_SETPAOUTPUT         = 9,
	SX127X_IOCTL_CMD_GETOUTPUTPOWER      = 10,
	SX127X_IOCTL_CMD_SETOUTPUTPOWER      = 11,
	SX127X_IOCTL_CMD_GETBANDWIDTH        = 12,
	SX127X_IOCTL_CMD_SETBANDWIDTH        = 13,
	SX127X_IOCTL_CMD_GETSYNCWORD         = 14,
	SX127X_IOCTL_CMD_SETSYNCWORD         = 15,
	SX127X_IOCTL_CMD_GETCRC              = 16,
	SX127X_IOCTL_CMD_SETCRC              = 17,
	SX127X_IOCTL_CMD_GETINVERTIQ         = 18,
	SX127X_IOCTL_CMD_SETINVERTIQ         = 19,
};

struct sx127x_pkt {
	size_t len;
	size_t hdrlen;
	size_t payloadlen;

	__s16 snr;
	__s16 rssi;
	__u32 fei;
	__u8 crcfail;
} __attribute__ ((packed));

#endif

