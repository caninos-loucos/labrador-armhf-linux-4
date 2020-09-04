#ifndef _LORA_DEVICE_H_
#define _LORA_DEVICE_H_

/*number of retries in case read or write functions fail*/
#define RW_RETRY (5)

#define RX_WORK_POOLING_PERIOD (10) // Pool rx registers every 10ms

#define REG_FIFO     (0x00)
#define REG_OPMODE   (0x01)
#define REG_FRFMSB   (0x06)
#define REG_FRFMLD   (0x07)
#define REG_FRFLSB   (0x08)
#define REG_PACONFIG (0x09)
#define REG_VERSION  (0x42)

#define REG_IRQ_FLAGSMASK (0x11)
#define REG_IRQ_FLAGS     (0x12)
#define REG_LORA_IRQ_FLAGS_PAYLOAD_CRCERROR  (0x01 << 5)
#define REG_IRQ_FLAGS1_MODEREADY (0x3e << 7)
#define REG_LNA (0x0C)

#define REG_LORA_LNA_GAIN_SHIFT  (0x5)
#define REG_LORA_LNA_GAIN_MASK   (0x7 << REG_LORA_LNA_GAIN_SHIFT)
#define REG_LORA_LNA_GAIN_SET(x) (x << REG_LORA_LNA_GAIN_SHIFT)


#define LNA_GAIN_G1 (0x1)
#define LNA_GAIN_G2 (0x2)
#define LNA_GAIN_G3 (0x3)
#define LNA_GAIN_G4 (0x4)
#define LNA_GAIN_G5 (0x5)
#define LNA_GAIN_G6 (0x6)

/*
Signal
#define SIG_DIO0	 (0x01)
*/
#define REGADDR(x)   (x & 0x7f)
#define WRITEADDR(x) (x | (0x1 << 7))

#define LORAREG_MASK   (0x1 << 8)
#define FSKOOKREG_MASK (0x1 << 9)

#define LORAREG(x)   (x | LORAREG_MASK)
#define FSKOOKREG(x) (x | FSKOOK_MASK)

#define REG_LORA_SYNCWORD           LORAREG(0x39)
#define REG_LORA_INVERTIQ           LORAREG(0x33)
#define REG_LORA_MODEMCONFIG1       LORAREG(0x1d)
#define REG_LORA_MODEMCONFIG2       LORAREG(0x1e)
#define REG_LORA_DETECTOPTIMIZATION LORAREG(0x31)
#define REG_LORA_RXCURRENTADDR      LORAREG(0x10)
#define REG_LORA_IRQFLAGSMSK        LORAREG(0x11)
#define REG_LORA_IRQFLAGS           LORAREG(0x12)
#define REG_LORA_RXNBBYTES          LORAREG(0x13)
#define REG_LORA_FIFOADDRPTR        LORAREG(0x0d)
#define REG_LORA_FIFOTXBASEADDR     LORAREG(0x0e)
#define REG_LORA_PKTSNRVALUE        LORAREG(0x19)
#define REG_LORA_PKTRSSIVALUE       LORAREG(0x1A)
#define REG_LORA_PAYLOADLENGTH      LORAREG(0x22)

#define REG_LORA_IRQFLAGS_CADDETECTED     (0x1 << 0)
#define REG_LORA_IRQFLAGS_CADDONE         (0x1 << 2)
#define REG_LORA_IRQFLAGS_TXDONE          (0x1 << 3)
#define REG_LORA_IRQFLAGS_PAYLOADCRCERROR (0x1 << 5)
#define REG_LORA_IRQFLAGS_RXDONE          (0x1 << 6)

#define REG_LORA_DETECTOPTIMIZATION_DETECTIONOPTIMIZE_MASK (0x7 << 0)

#define REG_LORA_INVERTIQ_INVERTIQ (0x1 << 6)

#define REG_LORA_MODEMCONFIG1_RXPAYLOADCRCON (0x1 << 1)

#define REG_LORA_MODEMCONFIG2_SPREADINGFACTOR_MASK  (0xF << 4)
#define REG_LORA_MODEMCONFIG2_SPREADINGFACTOR_SHIFT (4)

#define REG_OPMODE_LONGRANGEMODE_MASK    (0x1 << 7)
#define REG_OPMODE_LONGRANGEMODE_FSK_OOK (0x0 << 7)
#define REG_OPMODE_LONGRANGEMODE_LORA    (0x1 << 7)

#define REG_OPMODE_MODULATIONTYPE_MASK   (0x3 << 5)
#define REG_OPMODE_MODULATIONTYPE_FSK    (0x0 << 5)
#define REG_OPMODE_MODULATIONTYPE_OOK    (0x1 << 5)

/*FSK/OOK mode*/
#define REG_OPMODE_FSKOOK_MODE_MSK        (0x07 << 0)
#define REG_OPMODE_FSKOOK_MODE_SLEEP      (0x00 << 0)
#define REG_OPMODE_FSKOOK_MODE_STDBY      (0x01 << 0)
#define REG_OPMODE_FSKOOK_LFMODE_MSK      (0x01 << 3)
#define REG_OPMODE_FSKOOK_LFMODE_ON       (0x01 << 3)
#define REG_OPMODE_FSKOOK_LFMODE_OFF      (0x00 << 3)
#define REG_OPMODE_FSKOOK_RESVD_MSK       (0x01 << 4)
#define REG_OPMODE_FSKOOK_MODULATION_MSK  (0x03 << 5)
#define REG_OPMODE_FSKOOK_MODULATION_FSK  (0x00 << 5)
#define REG_OPMODE_FSKOOK_MODULATION_OOK  (0x01 << 5)
#define REG_OPMODE_FSKOOK_LORAMODE_MSK    (0x01 << 7)
#define REG_OPMODE_FSKOOK_LORAMODE_FSKOOK (0x00 << 7)
#define REG_OPMODE_FSKOOK_LORAMODE_LORA   (0x01 << 7)

#define REG_PACONFIG_PASELECT (0x1 << 7)
#define REG_PA_CONFIG_SELECT_PA_BOOST (0x01)

#define FIFO_SIZE 						  (512)
#define REG_LORA_PREAMBLE_MSB 			  (0x20)
#define REG_LORA_PREAMBLE_LSB             (0x21)
#define REG_LORA_FIFO_RX_BYTE_ADDRES      (0x25)
#define REG_LORA_FIFO_RX_BASE_ADDRESS     (0x0F)
#define REG_LORA_RX_NB_BYTES              (0x13)
#define REG_LORA_FIFO_ADDR_PTR            (0x0D)
#define REG_LORA_FIFO_RX_CURR_ADDRESS     (0x10)

#define REG_OPMODE_LORA_SHARED_ACCESS_MSK        (0x01 << 7)

#define REG_VERSION_FULLREV_MSK 		  (0x0F << 4)
#define REG_VERSION_METALREV_MSK 		  (0x0F << 0)

#define REG_OPMODE_LORA_MSK 			  (0x07 << 0)
#define REG_OPMODE_LORA_RX_CONTINUOUS	  (0x05 << 0)
#define REG_OPMODE_LORA_TX				  (0x03 << 0)

#define DEF_LORA_RF_CARRIER_FREQ (915000000)
#define DEF_CRYSTAL_OSC_FREQ (32000000)


static ssize_t sx127x_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

static ssize_t sx127x_dev_write(struct sx127x_priv *sx127x,struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

int sx127x_reg_read(struct sx127x_priv *sx127x, int reg);

int sx127x_reg_read16(struct spi_device *spi, u16 reg, u16* result);

int sx127x_reg_read24(struct sx127x_priv *sx127x, int reg);

int sx127x_reg_write(struct sx127x_priv *sx127x, int reg, u8 value);

int sx127x_reg_write24(struct sx127x_priv *sx127x, int reg, u32 value);

enum sx127x_modulation sx127x_getmodulation(struct sx127x *data);

int mode sx127x_getopmode(struct sx127x *sx127x);

int sx127x_getchipversion(struct sx127x *data, u8 *version);

int sx127x_setopmode(struct sx127x *sx127x, enum sx127x_opmode opmode);

int sx127x_setmodulation(struct sx127x *data, enum sx127x_modulation mod);

int sx127x_setsyncword(struct sx127x *data, u8 syncword);

int sx127x_getsyncword(struct sx127x *data);

int sx127x_setinvertiq(struct sx127x *data, bool invert);

int sx127x_getinvertiq(struct sx127x *data);

int sx127x_setcrc(struct sx127x *data, bool crc);

int sx127x_getcrc(struct sx127x *data);

int sx127x_setcarrierfrequency(struct sx127x *data, u64 freq);

int sx127x_getcarrierfrequency(struct sx127x *data);

int sx127x_setpaoutput(struct sx127x *data, enum sx127x_pa pa);

int sx127x_getpaoutput(struct sx127x *data);

int sx127x_setsf(struct sx127x *data, u8 sf);

int sx127x_getsf(struct sx127x *data);

int sx127x_fifo_readpkt(struct spi_device *spi, void *buffer, u8 *len);

int sx127x_fifo_writepkt(struct spi_device *spi, void *buffer, u8 len);

#endif

