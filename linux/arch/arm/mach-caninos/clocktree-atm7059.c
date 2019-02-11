#include "clocktree-owl.h"

static unsigned long rvregs[R_CMUMAX] = {
    [R_CMU_COREPLL]                 =	0x00000064,
    [R_CMU_DEVPLL]                  =	0x00001064,
    [R_CMU_DDRPLL]                  =	0x00000019,
    [R_CMU_NANDPLL]                 =	0x00000032,
    [R_CMU_DISPLAYPLL]              =	0x00000064,
    [R_CMU_AUDIOPLL]                =	0x10000001,
    [R_CMU_TVOUTPLL]                =	0x00250000,
    [R_CMU_BUSCLK]                  =	0x3df60012,
    [R_CMU_SENSORCLK]               =	0x00000000,
    [R_CMU_LCDCLK]                  =	0x00000100,
    [R_CMU_DSICLK]                  =	0x00000000,
    [R_CMU_CSICLK]                  =	0x00000000,
    [R_CMU_DECLK]                   =	0x00000000,
    [R_CMU_BISPCLK]                 =	0x00000000,
    [R_CMU_VDECLK]                  =	0x00000000,
    [R_CMU_VCECLK]                  =	0x00000000,
    [R_CMU_NANDCCLK]                =	0x00000001,
    [R_CMU_SD0CLK]                  =	0x00000000,
    [R_CMU_SD1CLK]                  =	0x00000000,
    [R_CMU_SD2CLK]                  =	0x00000000,
    [R_CMU_UART0CLK]                =	0x00000000,
    [R_CMU_UART1CLK]                =	0x00000000,
    [R_CMU_UART2CLK]                =	0x00000000,
    [R_CMU_PWM0CLK]                 =	0x00000000,
    [R_CMU_PWM1CLK]                 =	0x00000000,
    [R_CMU_PWM2CLK]                 =	0x00000000,
    [R_CMU_PWM3CLK]                 =	0x00000000,
    [R_CMU_ETHERNETPLL]             =	0x00000000,
    [R_CMU_CVBSPLL]             =	0x00000000,
    [R_CMU_LENSCLK]                 =	0x00000000,
    [R_CMU_GPU3DCLK]                =	0x00000000,
    [R_CMU_SSCLK]                   =	0x00000000,
    [R_CMU_UART3CLK]                =	0x00000000,
    [R_CMU_UART4CLK]                =	0x00000000,
    [R_CMU_UART5CLK]                =	0x00000000,
    [R_CMU_UART6CLK]                =	0x00000000,
    [R_CMU_COREPLLDEBUG]            =	0x00000000,
    [R_CMU_DEVPLLDEBUG]             =	0x00000000,
    [R_CMU_DDRPLLDEBUG]             =	0x00000000,
    [R_CMU_NANDPLLDEBUG]            =	0x00000000,
    [R_CMU_DISPLAYPLLDEBUG]         =	0x00000000,
    [R_CMU_TVOUTPLLDEBUG]           =	0x00000000,
    [R_CMU_DEEPCOLORPLLDEBUG]       =	0x00000000,
    [R_CMU_AUDIOPLL_ETHPLLDEBUG]    =	0x00000000,
};

static struct owl_clkreq busbit_DIVEN               = BITMAP(CMU_BUSCLK,                0x80000000, 31);

static struct owl_clkreq divbit_SPDIF_CLK           = BITMAP(CMU_AUDIOPLL,              0xf0000000, 28);
static struct owl_clkreq divbit_HDMIA_CLK           = BITMAP(CMU_AUDIOPLL,              0x0f000000, 24);
static struct owl_clkreq divbit_I2SRX_CLK           = BITMAP(CMU_AUDIOPLL,              0x00f00000, 20);
static struct owl_clkreq divbit_I2STX_CLK           = BITMAP(CMU_AUDIOPLL,              0x000f0000, 16);
static struct owl_clkreq divbit_APBDBG_CLK          = BITMAP(CMU_BUSCLK,                0x1c000000, 26);
static struct owl_clkreq divbit_ACP_CLK             = BITMAP(CMU_BUSCLK,                0x01800000, 23);
static struct owl_clkreq divbit_PERIPH_CLK          = BITMAP(CMU_BUSCLK,                0x00700000, 20);
static struct owl_clkreq divbit_NIC_DIV_CLK         = BITMAP(CMU_BUSCLK,                0x000c0000, 18);
static struct owl_clkreq divbit_NIC_CLK             = BITMAP(CMU_BUSCLK,                0x00030000, 16);
static struct owl_clkreq divbit_APB30_CLK           = BITMAP(CMU_BUSCLK,                0x0000c000, 14);
static struct owl_clkreq divbit_APB20_CLK           = BITMAP(CMU_BUSCLK,                0x00000700, 8);
static struct owl_clkreq divbit_AHBPREDIV_CLK       = BITMAP(CMU_BUSCLK1,               0x00003000, 12);
static struct owl_clkreq divbit_H_CLK               = BITMAP(CMU_BUSCLK1,               0x0000000c, 2);
static struct owl_clkreq divbit_SENSOR_CLKOUT1      = BITMAP(CMU_SENSORCLK,             0x00000f00, 8);
static struct owl_clkreq divbit_SENSOR_CLKOUT0      = BITMAP(CMU_SENSORCLK,             0x0000000f, 0);
static struct owl_clkreq divbit_LCD_CLK             = BITMAP(CMU_LCDCLK,                0x00000100, 8);
static struct owl_clkreq divbit_LCD1_CLK            = BITMAP(CMU_LCDCLK,                0x000000f0, 4);
static struct owl_clkreq divbit_LCD0_CLK            = BITMAP(CMU_LCDCLK,                0x0000000f, 0);
static struct owl_clkreq divbit_PRO_CLK             = BITMAP(CMU_DSICLK,                0x00000030, 4);
static struct owl_clkreq divbit_DSI_HCLK            = BITMAP(CMU_DSICLK,                0x00000003, 0);
static struct owl_clkreq divbit_CSI_CLK             = BITMAP(CMU_CSICLK,                0x0000000f, 0);
static struct owl_clkreq divbit_DE2_CLK             = BITMAP(CMU_DECLK,                 0x000000f0, 4);
static struct owl_clkreq divbit_DE1_CLK             = BITMAP(CMU_DECLK,                 0x0000000f, 0);
static struct owl_clkreq divbit_BISP_CLK            = BITMAP(CMU_BISPCLK,               0x0000000f, 0);
static struct owl_clkreq divbit_VDE_CLK             = BITMAP(CMU_VDECLK,                0x00000007, 0);
static struct owl_clkreq divbit_VCE_CLK             = BITMAP(CMU_VCECLK,                0x00000007, 0);
static struct owl_clkreq divbit_ECC_CLK             = BITMAP(CMU_NANDCCLK,              0x00000070, 4);
static struct owl_clkreq divbit_NANDC_CLK           = BITMAP(CMU_NANDCCLK,              0x00000007, 0);
static struct owl_clkreq divbit_PRESD0_CLK          = BITMAP(CMU_SD0CLK,                0x0000001f, 0);
static struct owl_clkreq divbit_SD0_CLK_2X          = BITMAP(CMU_SD0CLK,                0x00000100, 8);
static struct owl_clkreq divbit_PRESD1_CLK          = BITMAP(CMU_SD1CLK,                0x0000001f, 0);
static struct owl_clkreq divbit_SD1_CLK_2X          = BITMAP(CMU_SD1CLK,                0x00000100, 8);
static struct owl_clkreq divbit_PRESD2_CLK          = BITMAP(CMU_SD2CLK,                0x0000001f, 0);
static struct owl_clkreq divbit_SD2_CLK_2X          = BITMAP(CMU_SD2CLK,                0x00000100, 8);
static struct owl_clkreq divbit_UART0_CLK           = BITMAP(CMU_UART0CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_UART1_CLK           = BITMAP(CMU_UART1CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_UART2_CLK           = BITMAP(CMU_UART2CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_PWM0_CLK            = BITMAP(CMU_PWM0CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_PWM1_CLK            = BITMAP(CMU_PWM1CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_PWM2_CLK            = BITMAP(CMU_PWM2CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_PWM3_CLK            = BITMAP(CMU_PWM3CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_PWM4_CLK            = BITMAP(CMU_PWM4CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_PWM5_CLK            = BITMAP(CMU_PWM5CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_RMII_REF_CLK        = BITMAP(CMU_ETHERNETPLL,           0x00000002, 1);
static struct owl_clkreq divbit_LENS_CLK            = BITMAP(CMU_LENSCLK,               0x00000007, 0);
static struct owl_clkreq divbit_GPU3D_SYSCLK        = BITMAP(CMU_GPU3DCLK,              0x07000000, 24);
static struct owl_clkreq divbit_GPU3D_NIC_MEMCLK    = BITMAP(CMU_GPU3DCLK,              0x00070000, 16);
static struct owl_clkreq divbit_GPU3D_HYDCLK        = BITMAP(CMU_GPU3DCLK,              0x00000700, 8);
static struct owl_clkreq divbit_GPU3D_CORECLK       = BITMAP(CMU_GPU3DCLK,              0x00000007, 0);
static struct owl_clkreq divbit_SS_CLK              = BITMAP(CMU_SSCLK,                 0x000003ff, 0);
static struct owl_clkreq divbit_UART3_CLK           = BITMAP(CMU_UART3CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_UART4_CLK           = BITMAP(CMU_UART4CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_UART5_CLK           = BITMAP(CMU_UART5CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_UART6_CLK           = BITMAP(CMU_UART6CLK,              0x000001ff, 0);
                                                                                        
                                                                                        
static struct owl_clkreq selbit_NIC_CLK             = BITMAP(CMU_BUSCLK,                0x00000070, 4);
static struct owl_clkreq selbit_AHBPREDIV_CLK       = BITMAP(CMU_BUSCLK1,               0x00000700, 8);
static struct owl_clkreq selbit_DEV_CLK             = BITMAP(CMU_DEVPLL,                0x00001000, 12);
static struct owl_clkreq selbit_DDR_CLK_CH1         = BITMAP(CMU_DDRPLL,                0x00000600, 9);
static struct owl_clkreq selbit_CORE_CLK            = BITMAP(CMU_BUSCLK,                0x00000003, 0);
static struct owl_clkreq selbit_SENSOR_CLKOUT0      = BITMAP(CMU_SENSORCLK,             0x00000010, 4);
static struct owl_clkreq selbit_SENSOR_CLKOUT1      = BITMAP(CMU_SENSORCLK,             0x00000010, 4);
static struct owl_clkreq selbit_LCD_CLK             = BITMAP(CMU_LCDCLK,                0x00003000, 12);
static struct owl_clkreq selbit_CSI_CLK             = BITMAP(CMU_CSICLK,                0x00000010, 4);
static struct owl_clkreq selbit_IMG5_CLK            = BITMAP(CMU_DECLK,                 0x00020000, 17);
static struct owl_clkreq selbit_DE1_CLK             = BITMAP(CMU_DECLK,                 0x00001000, 12);
static struct owl_clkreq selbit_DE2_CLK             = BITMAP(CMU_DECLK,                 0x00001000, 12);
static struct owl_clkreq selbit_BISP_CLK            = BITMAP(CMU_BISPCLK,               0x00000010, 4);
static struct owl_clkreq selbit_VDE_CLK             = BITMAP(CMU_VDECLK,                0x00000030, 4);
static struct owl_clkreq selbit_VCE_CLK             = BITMAP(CMU_VCECLK,                0x00000030, 4);
static struct owl_clkreq selbit_NANDC_CLK           = BITMAP(CMU_NANDCCLK,              0x00000300, 8);
static struct owl_clkreq selbit_ECC_CLK             = BITMAP(CMU_NANDCCLK,              0x00000300, 8);
static struct owl_clkreq selbit_PRESD0_CLK          = BITMAP(CMU_SD0CLK,                0x00000200, 9);
static struct owl_clkreq selbit_PRESD1_CLK          = BITMAP(CMU_SD1CLK,                0x00000200, 9);
static struct owl_clkreq selbit_PRESD2_CLK          = BITMAP(CMU_SD2CLK,                0x00000200, 9);
static struct owl_clkreq selbit_UART0_CLK           = BITMAP(CMU_UART0CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART1_CLK           = BITMAP(CMU_UART1CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART2_CLK           = BITMAP(CMU_UART2CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART3_CLK           = BITMAP(CMU_UART3CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART4_CLK           = BITMAP(CMU_UART4CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART5_CLK           = BITMAP(CMU_UART5CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART6_CLK           = BITMAP(CMU_UART6CLK,              0x00010000, 16);
static struct owl_clkreq selbit_PWM0_CLK            = BITMAP(CMU_PWM0CLK,               0x00001000, 12);
static struct owl_clkreq selbit_PWM1_CLK            = BITMAP(CMU_PWM1CLK,               0x00001000, 12);
static struct owl_clkreq selbit_PWM2_CLK            = BITMAP(CMU_PWM2CLK,               0x00001000, 12);
static struct owl_clkreq selbit_PWM3_CLK            = BITMAP(CMU_PWM3CLK,               0x00001000, 12);
static struct owl_clkreq selbit_PWM4_CLK            = BITMAP(CMU_PWM4CLK,               0x00001000, 12);
static struct owl_clkreq selbit_PWM5_CLK            = BITMAP(CMU_PWM5CLK,               0x00001000, 12);
static struct owl_clkreq selbit_GPU3D_SYSCLK        = BITMAP(CMU_GPU3DCLK,              0x70000000, 28);
static struct owl_clkreq selbit_GPU3D_NIC_MEMCLK    = BITMAP(CMU_GPU3DCLK,              0x00700000, 20);
static struct owl_clkreq selbit_GPU3D_HYDCLK        = BITMAP(CMU_GPU3DCLK,              0x00007000, 12);
static struct owl_clkreq selbit_GPU3D_CORECLK       = BITMAP(CMU_GPU3DCLK,              0x00000070, 4);
                                                                                        
                                                                                        
static struct owl_clkreq pllbit_COREPLLEN           = BITMAP(CMU_COREPLL,               0x00000200, 9);
static struct owl_clkreq pllbit_COREPLLFREQ         = BITMAP(CMU_COREPLL,               0x000000ff, 0);
static struct owl_clkreq pllbit_DEVPLLEN            = BITMAP(CMU_DEVPLL,                0x00000100, 8);
static struct owl_clkreq pllbit_DEVPLLFREQ          = BITMAP(CMU_DEVPLL,                0x0000007f, 0);
static struct owl_clkreq pllbit_DDRPLLEN            = BITMAP(CMU_DDRPLL,                0x00000100, 8);
static struct owl_clkreq pllbit_DDRPLLFREQ          = BITMAP(CMU_DDRPLL,                0x000000ff, 0);
static struct owl_clkreq pllbit_NANDPLLEN           = BITMAP(CMU_NANDPLL,               0x00000100, 8);
static struct owl_clkreq pllbit_NANDPLLFREQ         = BITMAP(CMU_NANDPLL,               0x0000007f, 0);
static struct owl_clkreq pllbit_DISPLAYPLLEN        = BITMAP(CMU_DISPLAYPLL,            0x00000100, 8);
static struct owl_clkreq pllbit_DISPALYPLLFREQ      = BITMAP(CMU_DISPLAYPLL,            0x000000ff, 0);
static struct owl_clkreq pllbit_AUDIOPLLEN          = BITMAP(CMU_AUDIOPLL,              0x00000010, 4);
static struct owl_clkreq pllbit_AUDIOPLLFREQ_SEL    = BITMAP(CMU_AUDIOPLL,              0x00000001, 0);
static struct owl_clkreq pllbit_TVOUTPLLEN          = BITMAP(CMU_TVOUTPLL,              0x00000008, 3);
static struct owl_clkreq pllbit_TVOUTPLLFREQ_SEL    = BITMAP(CMU_TVOUTPLL,              0x00070000, 16);
static struct owl_clkreq pllbit_ENTRNETPLL_EN       = BITMAP(CMU_ETHERNETPLL,           0x00000001, 0);
static struct owl_clkreq pllbit_CVBSPLL_EN       = BITMAP(CMU_CVBSPLL,           0x00000100, 8);
static struct owl_clkreq pllbit_CVBSPLLREQ      = BITMAP(CMU_CVBSPLL,            0x000000ff, 0);
static struct owl_clkreq pllbit_COREPLLLOCK         = BITMAP(CMU_COREPLLDEBUG,          0x00000800, 11);
static struct owl_clkreq pllbit_DEVPLLLOCK          = BITMAP(CMU_DEVPLLDEBUG,           0x00000800, 11);
static struct owl_clkreq pllbit_DDRPLLLOCK          = BITMAP(CMU_DDRPLLDEBUG,           0x80000000, 31);
static struct owl_clkreq pllbit_NANDPLLLOCK         = BITMAP(CMU_NANDPLLDEBUG,          0x80000000, 31);
static struct owl_clkreq pllbit_DISPLAYPLLLOCK      = BITMAP(CMU_DISPLAYPLLDEBUG,       0x80000000, 31);
static struct owl_clkreq pllbit_TVOUTPLLLOCK        = BITMAP(CMU_TVOUTPLLDEBUG,         0x00004000, 14);
static struct owl_clkreq pllbit_ETHERNETPLLLOCK     = BITMAP(CMU_AUDIOPLL_ETHPLLDEBUG,  0x00800000, 23);
static struct owl_clkreq pllbit_AUDIOPLLLOCK        = BITMAP(CMU_AUDIOPLL_ETHPLLDEBUG,  0x00000200, 9);
static struct owl_clkreq pllbit_CVBSPLLLOCK        = BITMAP(CMU_CVBSPLLDEBUG,  0x00000200, 9);


static struct owl_clkreq enablebit_HOSC             = BITMAP(CMU_COREPLL,               0x00000100, 8);
static struct owl_clkreq enablebit_MODULE_GPU3D     = BITMAP(CMU_DEVCLKEN0,             0x40000000, 30);
static struct owl_clkreq enablebit_MODULE_SHARESRAM = BITMAP(CMU_DEVCLKEN0,             0x10000000, 28);
static struct owl_clkreq enablebit_MODULE_HDCP2X    = BITMAP(CMU_DEVCLKEN0,             0x08000000, 27);
static struct owl_clkreq enablebit_MODULE_VCE       = BITMAP(CMU_DEVCLKEN0,             0x04000000, 26);
static struct owl_clkreq enablebit_MODULE_VDE       = BITMAP(CMU_DEVCLKEN0,             0x02000000, 25);
static struct owl_clkreq enablebit_MODULE_PCM0      = BITMAP(CMU_DEVCLKEN0,             0x01000000, 24);
static struct owl_clkreq enablebit_MODULE_SPDIF     = BITMAP(CMU_DEVCLKEN0,             0x00800000, 23);
static struct owl_clkreq enablebit_MODULE_HDMIA     = BITMAP(CMU_DEVCLKEN0,             0x00400000, 22);
static struct owl_clkreq enablebit_MODULE_I2SRX     = BITMAP(CMU_DEVCLKEN0,             0x00200000, 21);
static struct owl_clkreq enablebit_MODULE_I2STX     = BITMAP(CMU_DEVCLKEN0,             0x00100000, 20);
static struct owl_clkreq enablebit_MODULE_GPIO      = BITMAP(CMU_DEVCLKEN0,             0x00040000, 18);
static struct owl_clkreq enablebit_MODULE_KEY       = BITMAP(CMU_DEVCLKEN0,             0x00020000, 17);
static struct owl_clkreq enablebit_MODULE_LENS      = BITMAP(CMU_DEVCLKEN0,             0x00010000, 16);
static struct owl_clkreq enablebit_MODULE_BISP      = BITMAP(CMU_DEVCLKEN0,             0x00004000, 14);
static struct owl_clkreq enablebit_MODULE_CSI       = BITMAP(CMU_DEVCLKEN0,             0x00002000, 13);
static struct owl_clkreq enablebit_MODULE_DSI       = BITMAP(CMU_DEVCLKEN0,             0x00001000, 12);
static struct owl_clkreq enablebit_MODULE_LVDS      = BITMAP(CMU_DEVCLKEN0,             0x00000800, 11);
static struct owl_clkreq enablebit_MODULE_LCD1      = BITMAP(CMU_DEVCLKEN0,             0x00000400, 10);
static struct owl_clkreq enablebit_MODULE_LCD0      = BITMAP(CMU_DEVCLKEN0,             0x00000200, 9);
static struct owl_clkreq enablebit_MODULE_DE        = BITMAP(CMU_DEVCLKEN0,             0x00000100, 8);
static struct owl_clkreq enablebit_MODULE_SD2       = BITMAP(CMU_DEVCLKEN0,             0x00000080, 7);
static struct owl_clkreq enablebit_MODULE_SD1       = BITMAP(CMU_DEVCLKEN0,             0x00000040, 6);
static struct owl_clkreq enablebit_MODULE_SD0       = BITMAP(CMU_DEVCLKEN0,             0x00000020, 5);
static struct owl_clkreq enablebit_MODULE_NANDC     = BITMAP(CMU_DEVCLKEN0,             0x00000010, 4);
static struct owl_clkreq enablebit_MODULE_DDRCH0    = BITMAP(CMU_DEVCLKEN0,             0x00000008, 3);
static struct owl_clkreq enablebit_MODULE_NOR       = BITMAP(CMU_DEVCLKEN0,             0x00000004, 2);
static struct owl_clkreq enablebit_MODULE_DMAC      = BITMAP(CMU_DEVCLKEN0,             0x00000002, 1);
static struct owl_clkreq enablebit_MODULE_DDRCH1    = BITMAP(CMU_DEVCLKEN0,             0x00000001, 0);
static struct owl_clkreq enablebit_MODULE_I2C3      = BITMAP(CMU_DEVCLKEN1,             0x80000000, 31);
static struct owl_clkreq enablebit_MODULE_I2C2      = BITMAP(CMU_DEVCLKEN1,             0x40000000, 30);
static struct owl_clkreq enablebit_MODULE_TIMER     = BITMAP(CMU_DEVCLKEN1,             0x08000000, 27);
static struct owl_clkreq enablebit_MODULE_PWM5      = BITMAP(CMU_DEVCLKEN0,             0x00000001, 0);
static struct owl_clkreq enablebit_MODULE_PWM4      = BITMAP(CMU_DEVCLKEN0,             0x00000800, 11);
static struct owl_clkreq enablebit_MODULE_PWM3      = BITMAP(CMU_DEVCLKEN1,             0x04000000, 26);
static struct owl_clkreq enablebit_MODULE_PWM2      = BITMAP(CMU_DEVCLKEN1,             0x02000000, 25);
static struct owl_clkreq enablebit_MODULE_PWM1      = BITMAP(CMU_DEVCLKEN1,             0x01000000, 24);
static struct owl_clkreq enablebit_MODULE_PWM0      = BITMAP(CMU_DEVCLKEN1,             0x00800000, 23);
static struct owl_clkreq enablebit_MODULE_ETHERNET  = BITMAP(CMU_DEVCLKEN1,             0x00400000, 22);
static struct owl_clkreq enablebit_MODULE_UART5     = BITMAP(CMU_DEVCLKEN1,             0x00200000, 21);
static struct owl_clkreq enablebit_MODULE_UART4     = BITMAP(CMU_DEVCLKEN1,             0x00100000, 20);
static struct owl_clkreq enablebit_MODULE_UART3     = BITMAP(CMU_DEVCLKEN1,             0x00080000, 19);
static struct owl_clkreq enablebit_MODULE_UART6     = BITMAP(CMU_DEVCLKEN1,             0x00040000, 18);
static struct owl_clkreq enablebit_MODULE_PCM1      = BITMAP(CMU_DEVCLKEN1,             0x00010000, 16);
static struct owl_clkreq enablebit_MODULE_I2C1      = BITMAP(CMU_DEVCLKEN1,             0x00008000, 15);
static struct owl_clkreq enablebit_MODULE_I2C0      = BITMAP(CMU_DEVCLKEN1,             0x00004000, 14);
static struct owl_clkreq enablebit_MODULE_SPI3      = BITMAP(CMU_DEVCLKEN1,             0x00002000, 13);
static struct owl_clkreq enablebit_MODULE_SPI2      = BITMAP(CMU_DEVCLKEN1,             0x00001000, 12);
static struct owl_clkreq enablebit_MODULE_SPI1      = BITMAP(CMU_DEVCLKEN1,             0x00000800, 11);
static struct owl_clkreq enablebit_MODULE_SPI0      = BITMAP(CMU_DEVCLKEN1,             0x00000400, 10);
static struct owl_clkreq enablebit_MODULE_IRC       = BITMAP(CMU_DEVCLKEN1,             0x00000200, 9);
static struct owl_clkreq enablebit_MODULE_UART2     = BITMAP(CMU_DEVCLKEN1,             0x00000100, 8);
static struct owl_clkreq enablebit_MODULE_UART1     = BITMAP(CMU_DEVCLKEN1,             0x00000080, 7);
static struct owl_clkreq enablebit_MODULE_UART0     = BITMAP(CMU_DEVCLKEN1,             0x00000040, 6);
static struct owl_clkreq enablebit_MODULE_HDMI      = BITMAP(CMU_DEVCLKEN1,             0x00000008, 3);
static struct owl_clkreq enablebit_MODULE_SS        = BITMAP(CMU_DEVCLKEN1,             0x00000004, 2);
static struct owl_clkreq enablebit_MODULE_TV24M     = BITMAP(CMU_TVOUTPLL,              0x00800000, 23);
static struct owl_clkreq enablebit_MODULE_CVBS_CLK108M     = BITMAP(CMU_CVBSPLL,              0x00000200, 9);
static struct owl_clkreq enablebit_MODULE_TVOUT     = BITMAP(CMU_DEVCLKEN1,             0x00000001, 0);

static struct owl_clkreq resetbit_MODULE_PERIPHRESET       = BITMAP(CMU_DEVRST0,               0x08000000, 27);
static struct owl_clkreq resetbit_MODULE_LENS              = BITMAP(CMU_DEVRST0,               0x04000000, 26);
static struct owl_clkreq resetbit_MODULE_NIC301            = BITMAP(CMU_DEVRST0,               0x00800000, 23);
static struct owl_clkreq resetbit_MODULE_GPU3D             = BITMAP(CMU_DEVRST0,               0x00400000, 22);
static struct owl_clkreq resetbit_MODULE_VCE               = BITMAP(CMU_DEVRST0,               0x00100000, 20);
static struct owl_clkreq resetbit_MODULE_VDE               = BITMAP(CMU_DEVRST0,               0x00080000, 19);
static struct owl_clkreq resetbit_MODULE_PCM0              = BITMAP(CMU_DEVRST0,               0x00040000, 18);
static struct owl_clkreq resetbit_MODULE_AUDIO             = BITMAP(CMU_DEVRST0,               0x00020000, 17);
static struct owl_clkreq resetbit_MODULE_GPIO              = BITMAP(CMU_DEVRST0,               0x00008000, 15);
static struct owl_clkreq resetbit_MODULE_KEY               = BITMAP(CMU_DEVRST0,               0x00004000, 14);
static struct owl_clkreq resetbit_MODULE_BISP              = BITMAP(CMU_DEVRST0,               0x00001000, 12);
static struct owl_clkreq resetbit_MODULE_CSI               = BITMAP(CMU_DEVRST0,               0x00000800, 11);
static struct owl_clkreq resetbit_MODULE_DSI               = BITMAP(CMU_DEVRST0,               0x00000400, 10);
static struct owl_clkreq resetbit_MODULE_SD2               = BITMAP(CMU_DEVRST0,               0x00000200, 9);
static struct owl_clkreq resetbit_MODULE_LCD               = BITMAP(CMU_DEVRST0,               0x00000100, 8);
static struct owl_clkreq resetbit_MODULE_DE                = BITMAP(CMU_DEVRST0,               0x00000080, 7);
static struct owl_clkreq resetbit_MODULE_PCM1              = BITMAP(CMU_DEVRST0,               0x00000040, 6);
static struct owl_clkreq resetbit_MODULE_SD1               = BITMAP(CMU_DEVRST0,               0x00000020, 5);
static struct owl_clkreq resetbit_MODULE_SD0               = BITMAP(CMU_DEVRST0,               0x00000010, 4);
static struct owl_clkreq resetbit_MODULE_NANDC             = BITMAP(CMU_DEVRST0,               0x00000008, 3);
static struct owl_clkreq resetbit_MODULE_DDR               = BITMAP(CMU_DEVRST0,               0x00000004, 2);
static struct owl_clkreq resetbit_MODULE_NORIF             = BITMAP(CMU_DEVRST0,               0x00000002, 1);
static struct owl_clkreq resetbit_MODULE_DMAC              = BITMAP(CMU_DEVRST0,               0x00000001, 0);
static struct owl_clkreq resetbit_MODULE_DBG3RESET         = BITMAP(CMU_DEVRST1,               0x80000000, 31);
static struct owl_clkreq resetbit_MODULE_DBG2RESET         = BITMAP(CMU_DEVRST1,               0x40000000, 30);
static struct owl_clkreq resetbit_MODULE_DBG1RESET         = BITMAP(CMU_DEVRST1,               0x20000000, 29);
static struct owl_clkreq resetbit_MODULE_DBG0RESET         = BITMAP(CMU_DEVRST1,               0x10000000, 28);
static struct owl_clkreq resetbit_MODULE_WD3RESET          = BITMAP(CMU_DEVRST1,               0x08000000, 27);
static struct owl_clkreq resetbit_MODULE_WD2RESET          = BITMAP(CMU_DEVRST1,               0x04000000, 26);
static struct owl_clkreq resetbit_MODULE_WD1RESET          = BITMAP(CMU_DEVRST1,               0x02000000, 25);
static struct owl_clkreq resetbit_MODULE_WD0RESET          = BITMAP(CMU_DEVRST1,               0x01000000, 24);
static struct owl_clkreq resetbit_MODULE_USB2_1            = BITMAP(CMU_DEVRST1,               0x00400000, 22);
static struct owl_clkreq resetbit_MODULE_CHIPID            = BITMAP(CMU_DEVRST1,               0x00200000, 21);
static struct owl_clkreq resetbit_MODULE_ETHERNET          = BITMAP(CMU_DEVRST1,               0x00100000, 20);
static struct owl_clkreq resetbit_MODULE_I2C3              = BITMAP(CMU_DEVRST1,               0x00080000, 19);
static struct owl_clkreq resetbit_MODULE_I2C2              = BITMAP(CMU_DEVRST1,               0x00040000, 18);
static struct owl_clkreq resetbit_MODULE_UART5             = BITMAP(CMU_DEVRST1,               0x00020000, 17);
static struct owl_clkreq resetbit_MODULE_UART4             = BITMAP(CMU_DEVRST1,               0x00010000, 16);
static struct owl_clkreq resetbit_MODULE_UART3             = BITMAP(CMU_DEVRST1,               0x00008000, 15);
static struct owl_clkreq resetbit_MODULE_USB3              = BITMAP(CMU_DEVRST1,               0x00004000, 14);
static struct owl_clkreq resetbit_MODULE_I2C1              = BITMAP(CMU_DEVRST1,               0x00002000, 13);
static struct owl_clkreq resetbit_MODULE_I2C0              = BITMAP(CMU_DEVRST1,               0x00001000, 12);
static struct owl_clkreq resetbit_MODULE_SPI3              = BITMAP(CMU_DEVRST1,               0x00000800, 11);
static struct owl_clkreq resetbit_MODULE_SPI2              = BITMAP(CMU_DEVRST1,               0x00000400, 10);
static struct owl_clkreq resetbit_MODULE_SPI1              = BITMAP(CMU_DEVRST1,               0x00000200, 9);
static struct owl_clkreq resetbit_MODULE_SPI0              = BITMAP(CMU_DEVRST1,               0x00000100, 8);
static struct owl_clkreq resetbit_MODULE_UART2             = BITMAP(CMU_DEVRST1,               0x00000080, 7);
static struct owl_clkreq resetbit_MODULE_UART1             = BITMAP(CMU_DEVRST1,               0x00000040, 6);
static struct owl_clkreq resetbit_MODULE_UART0             = BITMAP(CMU_DEVRST1,               0x00000020, 5);
static struct owl_clkreq resetbit_MODULE_UART6             = BITMAP(CMU_DEVRST1,               0x00000010, 4);
static struct owl_clkreq resetbit_MODULE_HDCP2X              = BITMAP(CMU_DEVRST1,               0x00000008, 3);
static struct owl_clkreq resetbit_MODULE_HDMI              = BITMAP(CMU_DEVRST1,               0x00000004, 2);
static struct owl_clkreq resetbit_MODULE_TVOUT             = BITMAP(CMU_DEVRST1,               0x00000002, 1);
static struct owl_clkreq resetbit_MODULE_USB2_0            = BITMAP(CMU_DEVRST1,               0x00000001, 0);


static struct owl_clocknode clocks[CLOCK__MAX] = {
	CLOCKNODE_ROOT(HOSC, FREQUENCY_24M),
	CLOCKNODE_ROOT(IC_32K, FREQUENCY_32K),
	PLLNODE(COREPLL, HOSC),
	PLLNODE(DEVPLL, HOSC),
	PLLNODE(DDRPLL, HOSC),
	PLLNODE(NANDPLL, HOSC),
	PLLNODE(DISPLAYPLL, HOSC),
	PLLNODE(AUDIOPLL, HOSC),
	PLLNODE(TVOUTPLL, HOSC),
	CLOCKNODE_UNUSED(DEEPCOLORPLL),
	PLLNODE(ETHERNETPLL, HOSC),
	PLLNODE(CVBSPLL, HOSC),
	CLOCKNODE_S2(DEV_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S1(DDR_CLK_0, DDRPLL),
	CLOCKNODE_S1(DDR_CLK_90, DDRPLL),
	CLOCKNODE_S1(DDR_CLK_180, DDRPLL),
	CLOCKNODE_S1(DDR_CLK_270, DDRPLL),
	CLOCKNODE_S1(DDR_CLK_CH0, DDR_CLK_0),
	CLOCKNODE_S4(DDR_CLK_CH1, DDR_CLK_0, DDR_CLK_90, DDR_CLK_180, DDR_CLK_270, 0),
	CLOCKNODE_S1(DDR_CLK, DDR_CLK_0),
	CLOCKNODE_S1(SPDIF_CLK, AUDIOPLL),
	CLOCKNODE_S1(HDMIA_CLK, AUDIOPLL),
	CLOCKNODE_S1(I2SRX_CLK, AUDIOPLL),
	CLOCKNODE_S1(I2STX_CLK, AUDIOPLL),
	CLOCKNODE_S1(PCM0_CLK, AUDIOPLL),
	CLOCKNODE_S1(PCM1_CLK, AUDIOPLL),
	CLOCKNODE_UNUSED(CLK_CVBSX2),
	CLOCKNODE_UNUSED(CLK_CVBS),
	CLOCKNODE_S1(CVBS_CLK108M, HOSC),
	CLOCKNODE_S1(CLK_PIXEL, TVOUTPLL),
	CLOCKNODE_S1(CLK_TMDS, TVOUTPLL),
	CLOCKNODE_S1(CLK_TMDS_PHY_P, CLK_TMDS),
	CLOCKNODE_UNUSED(CLK_TMDS_PHY_N),
	CLOCKNODE_S1(L2_NIC_CLK, CORE_CLK),
	CLOCKNODE_S1(APBDBG_CLK, CPU_CLK),
	CLOCKNODE_S1(L2_CLK, CPU_CLK),
	CLOCKNODE_S1(ACP_CLK, CPU_CLK),
	CLOCKNODE_S1(PERIPH_CLK, CPU_CLK),
	CLOCKNODE_S1(NIC_DIV_CLK, NIC_CLK),
	CLOCKNODE_S4(NIC_CLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, 0),
	CLOCKNODE_S4(AHBPREDIV_CLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, 0),
	CLOCKNODE_S1(H_CLK, AHBPREDIV_CLK),
	CLOCKNODE_S1(APB30_CLK, NIC_CLK),
	CLOCKNODE_S1(APB20_CLK, NIC_CLK),
	CLOCKNODE_S1(AHB_CLK, H_CLK),
	CLOCKNODE_S4(CORE_CLK, IC_32K, HOSC, COREPLL, VCE_CLK, 1),
	CLOCKNODE_S1(CPU_CLK, CORE_CLK),
	CLOCKNODE_S2(SENSOR_CLKOUT0, HOSC, ISPBP_CLK, 0),
	CLOCKNODE_S2(SENSOR_CLKOUT1, HOSC, ISPBP_CLK, 0),
	CLOCKNODE_S2(LCD_CLK, DISPLAYPLL, DEV_CLK, 0),
	CLOCKNODE_S1(LVDS_CLK, DISPLAYPLL),
	CLOCKNODE_S1(CKA_LCD_H, LVDS_CLK),
	CLOCKNODE_S1(LCD1_CLK, LCD_CLK),
	CLOCKNODE_S1(LCD0_CLK, LCD_CLK),
	CLOCKNODE_S1(DSI_HCLK, DISPLAYPLL),
	CLOCKNODE_S1(DSI_HCLK90, DSI_HCLK),
	CLOCKNODE_S1(PRO_CLK, DSI_HCLK90),
	CLOCKNODE_S1(PHY_CLK, DSI_HCLK90),
	CLOCKNODE_S2(CSI_CLK, DISPLAYPLL, DEV_CLK, 0),
	CLOCKNODE_S2(DE1_CLK, DISPLAYPLL, DEV_CLK, 0),
	CLOCKNODE_S2(DE2_CLK, DISPLAYPLL, DEV_CLK, 0),
	CLOCKNODE_S2(BISP_CLK, DISPLAYPLL, DEV_CLK, 0),
	CLOCKNODE_S1(ISPBP_CLK, BISP_CLK),
	CLOCKNODE_S2(IMG5_CLK, LCD1_CLK, LCD0_CLK, 0),
	CLOCKNODE_S4(VDE_CLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, 0),
	CLOCKNODE_S4(VCE_CLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, 0),
	CLOCKNODE_S4(NANDC_CLK, NANDPLL, DISPLAYPLL, DEV_CLK, DDRPLL, 2),
	CLOCKNODE_S4(ECC_CLK, NANDPLL, DISPLAYPLL, DEV_CLK, DDRPLL, 2),
	CLOCKNODE_S2(PRESD0_CLK, DEV_CLK, NANDPLL, 0),
	CLOCKNODE_S2(PRESD1_CLK, DEV_CLK, NANDPLL, 0),
	CLOCKNODE_S2(PRESD2_CLK, DEV_CLK, NANDPLL, 0),
	CLOCKNODE_S1(SD0_CLK_2X, PRESD0_CLK),
	CLOCKNODE_S1(SD1_CLK_2X, PRESD1_CLK),
	CLOCKNODE_S1(SD2_CLK_2X, PRESD2_CLK),
	CLOCKNODE_S1(SD0_CLK, SD0_CLK_2X),
	CLOCKNODE_S1(SD1_CLK, SD1_CLK_2X),
	CLOCKNODE_S1(SD2_CLK, SD2_CLK_2X),
	CLOCKNODE_S2(UART0_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART1_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART2_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART3_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART4_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART5_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART6_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(PWM0_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S2(PWM1_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S2(PWM2_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S2(PWM3_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S2(PWM4_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S2(PWM5_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S1(RMII_REF_CLK, ETHERNETPLL),
	CLOCKNODE_S1(I2C0_CLK, ETHERNETPLL),
	CLOCKNODE_S1(I2C1_CLK, ETHERNETPLL),
	CLOCKNODE_S1(I2C2_CLK, ETHERNETPLL),
	CLOCKNODE_S1(I2C3_CLK, ETHERNETPLL),
	CLOCKNODE_S1(25M_CLK, ETHERNETPLL),
	CLOCKNODE_S1(LENS_CLK, HOSC),
	CLOCKNODE_S1(HDMI24M, HOSC),
	CLOCKNODE_S1(TIMER_CLK, HOSC),
	CLOCKNODE_S1(SS_CLK, HOSC),
	CLOCKNODE_S1(SPS_CLK, HOSC),
	CLOCKNODE_S1(IRC_CLK, HOSC),
	CLOCKNODE_S1(TV24M, HOSC),
	CLOCKNODE_S1(MIPI24M, HOSC),
	CLOCKNODE_S1(LENS24M, HOSC),
	CLOCKNODE_S5(GPU3D_SYSCLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, CVBSPLL, 0),
	CLOCKNODE_S5(GPU3D_HYDCLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, CVBSPLL, 0),
	CLOCKNODE_S5(GPU3D_NIC_MEMCLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, CVBSPLL, 0),
	CLOCKNODE_S5(GPU3D_CORECLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, CVBSPLL, 0),
};

static struct owl_pll pllnode[PLL__MAX] = {
    [PLL__COREPLL] = {
        .type = PLL_T_STEP,
        .range_from = 48/12,
        .range_to = 1608/12,
        .freq.step.step = 12 * MEGA,
        .sel = -1,
        .delay = OWL_COREPLL_DELAY,
        .reg_pllen = &pllbit_COREPLLEN,
        .reg_pllfreq = &pllbit_COREPLLFREQ,
        .reg_plllock = &pllbit_COREPLLLOCK,
    },
    [PLL__DEVPLL] = {
        .type = PLL_T_STEP,
        .range_from = 48/6,
        .range_to = 756/6,
        .freq.step.step = 6 * MEGA,
        .sel = -1,
        .delay = OWL_DEVPLL_DELAY,
        .reg_pllen = &pllbit_DEVPLLEN,
        .reg_pllfreq = &pllbit_DEVPLLFREQ,
        .reg_plllock = &pllbit_DEVPLLLOCK,
    },
    [PLL__DDRPLL] = {
        .type = PLL_T_STEP,
        .range_from = 12/12,
        .range_to = 804/12,
        .freq.step.step = 12 * MEGA,
        .sel = -1,
        .delay = OWL_DDRPLL_DELAY,
        .reg_pllen = &pllbit_DDRPLLEN,
        .reg_pllfreq = &pllbit_DDRPLLFREQ,
        .reg_plllock = &pllbit_DDRPLLLOCK,
    },
    [PLL__NANDPLL] = {
        .type = PLL_T_STEP,
        .range_from = 12/6,
        .range_to = 516/6,
        .freq.step.step = 6 * MEGA,
        .sel = -1,
        .delay = OWL_NANDPLL_DELAY,
        .reg_pllen = &pllbit_NANDPLLEN,
        .reg_pllfreq = &pllbit_NANDPLLFREQ,
        .reg_plllock = &pllbit_NANDPLLLOCK,
    },
    [PLL__DISPLAYPLL] = {
        .type = PLL_T_STEP,
        .range_from = 12/6,
        .range_to = 756/6,
        .freq.step.step = 6 * MEGA,
        .sel = -1,
        .delay = OWL_DISPLAYPLL_DELAY,
        .reg_pllen = &pllbit_DISPLAYPLLEN,
        .reg_pllfreq = &pllbit_DISPALYPLLFREQ,
        .reg_plllock = &pllbit_DISPLAYPLLLOCK,
    },

    [PLL__AUDIOPLL] = {
        .type = PLL_T_FREQ,
        .range_from = 0,
        .range_to = 1,
        .freq.freqtab = {45158400, 49152*KILO},
        .sel = -1,
        .delay = OWL_AUDIOPLL_DELAY,
        .reg_pllen = &pllbit_AUDIOPLLEN,
        .reg_pllfreq = &pllbit_AUDIOPLLFREQ_SEL,
        .reg_plllock = &pllbit_AUDIOPLLLOCK,
    },
    [PLL__TVOUTPLL] = {
        .type = PLL_T_FREQ,
        .range_from = 0,
        .range_to = 7,
        .freq.freqtab = {25200*KILO, 27*MEGA, 50400*KILO, 54*MEGA, 74250*KILO, 108*MEGA, 148500*KILO, 297*MEGA},
        .sel = -1,
        .delay = OWL_TVOUTPLL_DELAY,
        .reg_pllen = &pllbit_TVOUTPLLEN,
        .reg_pllfreq = &pllbit_TVOUTPLLFREQ_SEL,
        .reg_plllock = &pllbit_TVOUTPLLLOCK,
    },
    [PLL__ETHERNETPLL] = {
        .type = PLL_T_FREQ,
        .range_from = 0,
        .range_to = 0,
        .freq.freqtab = {500 * MEGA},
        .sel = -1,
        .delay = OWL_ETHERNETPLL_DELAY,
        .reg_pllen = &pllbit_ENTRNETPLL_EN,
        .reg_plllock = &pllbit_ETHERNETPLLLOCK,
    },
    [PLL__CVBSPLL] = {
        .type = PLL_T_STEP,
        .range_from = (348/12)-2,
        .range_to = (540/12)-2,
        .freq.step.step = 12 * MEGA,
        .freq.step.offset = 2,
        .sel = -1,
        .delay = OWL_CVBSPLL_DELAY,
        .reg_pllen = &pllbit_CVBSPLL_EN,
        .reg_pllfreq = &pllbit_CVBSPLLREQ,
        .reg_plllock = &pllbit_CVBSPLLLOCK,
    },
};

static struct owl_cmumod modnode[MOD__MAX] = {
    [MOD__ROOT]         = {"CMUMOD_DEVCLKS",    NULL,                           NULL                        },
    [MOD__GPU3D]        = {"CMUMOD_GPU3D",      &enablebit_MODULE_GPU3D,        &resetbit_MODULE_GPU3D      },
    [MOD__SHARESRAM]    = {"CMUMOD_SHARESRAM",  &enablebit_MODULE_SHARESRAM,    NULL                        },
    [MOD__HDCP2X]       = {"CMUMOD_HDCP2X",     &enablebit_MODULE_HDCP2X,       &resetbit_MODULE_HDCP2X                        },
    [MOD__VCE]          = {"CMUMOD_VCE",        &enablebit_MODULE_VCE,          &resetbit_MODULE_VCE        },
    [MOD__VDE]          = {"CMUMOD_VDE",        &enablebit_MODULE_VDE,          &resetbit_MODULE_VDE        },
    [MOD__PCM0]         = {"CMUMOD_PCM0",       &enablebit_MODULE_PCM0,         &resetbit_MODULE_PCM0       },
    [MOD__SPDIF]        = {"CMUMOD_SPDIF",      &enablebit_MODULE_SPDIF,        NULL                        },
    [MOD__HDMIA]        = {"CMUMOD_HDMIA",      &enablebit_MODULE_HDMIA,        NULL                        },
    [MOD__I2SRX]        = {"CMUMOD_I2SRX",      &enablebit_MODULE_I2SRX,        NULL                        },
    [MOD__I2STX]        = {"CMUMOD_I2STX",      &enablebit_MODULE_I2STX,        NULL                        },
    [MOD__GPIO]         = {"CMUMOD_GPIO",       &enablebit_MODULE_GPIO,         &resetbit_MODULE_GPIO       },
    [MOD__KEY]          = {"CMUMOD_KEY",        &enablebit_MODULE_KEY,          &resetbit_MODULE_KEY        },
    [MOD__LENS]         = {"CMUMOD_LENS",       &enablebit_MODULE_LENS,         &resetbit_MODULE_LENS       },
    [MOD__BISP]         = {"CMUMOD_BISP",       &enablebit_MODULE_BISP,         &resetbit_MODULE_BISP       },
    [MOD__CSI]          = {"CMUMOD_CSI",        &enablebit_MODULE_CSI,          &resetbit_MODULE_CSI        },
    [MOD__DSI]          = {"CMUMOD_DSI",        &enablebit_MODULE_DSI,          &resetbit_MODULE_DSI        },
    [MOD__LVDS]         = {"CMUMOD_LVDS",       &enablebit_MODULE_LVDS,         NULL                        },
    [MOD__LCD1]         = {"CMUMOD_LCD1",       &enablebit_MODULE_LCD1,         NULL                        },
    [MOD__LCD0]         = {"CMUMOD_LCD0",       &enablebit_MODULE_LCD0,         NULL                        },
    [MOD__DE]           = {"CMUMOD_DE",         &enablebit_MODULE_DE,           &resetbit_MODULE_DE         },
    [MOD__SD2]          = {"CMUMOD_SD2",        &enablebit_MODULE_SD2,          &resetbit_MODULE_SD2        },
    [MOD__SD1]          = {"CMUMOD_SD1",        &enablebit_MODULE_SD1,          &resetbit_MODULE_SD1        },
    [MOD__SD0]          = {"CMUMOD_SD0",        &enablebit_MODULE_SD0,          &resetbit_MODULE_SD0        },
    [MOD__NANDC]        = {"CMUMOD_NANDC",      &enablebit_MODULE_NANDC,        &resetbit_MODULE_NANDC      },
    [MOD__DDRCH0]       = {"CMUMOD_DDRCH0",     &enablebit_MODULE_DDRCH0,       NULL                        },
    [MOD__NOR]          = {"CMUMOD_NOR",        &enablebit_MODULE_NOR,          NULL                        },
    [MOD__DMAC]         = {"CMUMOD_DMAC",       &enablebit_MODULE_DMAC,         &resetbit_MODULE_DMAC       },
    [MOD__DDRCH1]       = {"CMUMOD_DDRCH1",     &enablebit_MODULE_DDRCH1,       NULL                        },
    [MOD__I2C3]         = {"CMUMOD_I2C3",       &enablebit_MODULE_I2C3,         &resetbit_MODULE_I2C3       },
    [MOD__I2C2]         = {"CMUMOD_I2C2",       &enablebit_MODULE_I2C2,         &resetbit_MODULE_I2C2       },
    [MOD__TIMER]        = {"CMUMOD_TIMER",      &enablebit_MODULE_TIMER,        NULL                        },
    [MOD__PWM5]         = {"CMUMOD_PWM5",       &enablebit_MODULE_PWM5,         NULL                        },
    [MOD__PWM4]         = {"CMUMOD_PWM4",       &enablebit_MODULE_PWM4,         NULL                        },
    [MOD__PWM3]         = {"CMUMOD_PWM3",       &enablebit_MODULE_PWM3,         NULL                        },
    [MOD__PWM2]         = {"CMUMOD_PWM2",       &enablebit_MODULE_PWM2,         NULL                        },
    [MOD__PWM1]         = {"CMUMOD_PWM1",       &enablebit_MODULE_PWM1,         NULL                        },
    [MOD__PWM0]         = {"CMUMOD_PWM0",       &enablebit_MODULE_PWM0,         NULL                        },
    [MOD__ETHERNET]     = {"CMUMOD_ETHERNET",   &enablebit_MODULE_ETHERNET,     &resetbit_MODULE_ETHERNET   },
    [MOD__UART5]        = {"CMUMOD_UART5",      &enablebit_MODULE_UART5,        &resetbit_MODULE_UART5      },
    [MOD__UART4]        = {"CMUMOD_UART4",      &enablebit_MODULE_UART4,        &resetbit_MODULE_UART4      },
    [MOD__UART3]        = {"CMUMOD_UART3",      &enablebit_MODULE_UART3,        &resetbit_MODULE_UART3      },
    [MOD__UART6]        = {"CMUMOD_UART6",      &enablebit_MODULE_UART6,        &resetbit_MODULE_UART6      },
    [MOD__PCM1]         = {"CMUMOD_PCM1",       &enablebit_MODULE_PCM1,         &resetbit_MODULE_PCM1       },
    [MOD__I2C1]         = {"CMUMOD_I2C1",       &enablebit_MODULE_I2C1,         &resetbit_MODULE_I2C1       },
    [MOD__I2C0]         = {"CMUMOD_I2C0",       &enablebit_MODULE_I2C0,         &resetbit_MODULE_I2C0       },
    [MOD__SPI3]         = {"CMUMOD_SPI3",       &enablebit_MODULE_SPI3,         &resetbit_MODULE_SPI3       },
    [MOD__SPI2]         = {"CMUMOD_SPI2",       &enablebit_MODULE_SPI2,         &resetbit_MODULE_SPI2       },
    [MOD__SPI1]         = {"CMUMOD_SPI1",       &enablebit_MODULE_SPI1,         &resetbit_MODULE_SPI1       },
    [MOD__SPI0]         = {"CMUMOD_SPI0",       &enablebit_MODULE_SPI0,         &resetbit_MODULE_SPI0       },
    [MOD__IRC]          = {"CMUMOD_IRC",        &enablebit_MODULE_IRC,          NULL                        },
    [MOD__UART2]        = {"CMUMOD_UART2",      &enablebit_MODULE_UART2,        &resetbit_MODULE_UART2      },
    [MOD__UART1]        = {"CMUMOD_UART1",      &enablebit_MODULE_UART1,        &resetbit_MODULE_UART1      },
    [MOD__UART0]        = {"CMUMOD_UART0",      &enablebit_MODULE_UART0,        &resetbit_MODULE_UART0      },
    [MOD__HDMI]         = {"CMUMOD_HDMI",       &enablebit_MODULE_HDMI,         &resetbit_MODULE_HDMI       },
    [MOD__SS]           = {"CMUMOD_SS",         &enablebit_MODULE_SS,           NULL                        },
    [MOD__TV24M]        = {"CMUMOD_TV24M",      &enablebit_MODULE_TV24M,        NULL                        },
    [MOD__CVBS_CLK108M]        = {"CMUMOD_CVBS_CLK108M",      &enablebit_MODULE_CVBS_CLK108M,        NULL                        },
    [MOD__TVOUT]        = {"CMUMOD_TVOUT",      &enablebit_MODULE_TVOUT,                           &resetbit_MODULE_TVOUT                        },

    [MOD__PERIPHRESET]  = {"CMUMOD_PERIPHRESET",NULL,                           &resetbit_MODULE_PERIPHRESET},
    [MOD__NIC301]       = {"CMUMOD_NIC301",     NULL,                           &resetbit_MODULE_NIC301     },
    [MOD__AUDIO]        = {"CMUMOD_AUDIO",      NULL,                           &resetbit_MODULE_AUDIO      },
    [MOD__LCD]          = {"CMUMOD_LCD",        NULL,                           &resetbit_MODULE_LCD        },
    [MOD__DDR]          = {"CMUMOD_DDR",        NULL,                           &resetbit_MODULE_DDR        },
    [MOD__NORIF]        = {"CMUMOD_NORIF",      NULL,                           &resetbit_MODULE_NORIF      },
    [MOD__DBG3RESET]    = {"CMUMOD_DBG3RESET",  NULL,                           &resetbit_MODULE_DBG3RESET  },
    [MOD__DBG2RESET]    = {"CMUMOD_DBG2RESET",  NULL,                           &resetbit_MODULE_DBG2RESET  },
    [MOD__DBG1RESET]    = {"CMUMOD_DBG1RESET",  NULL,                           &resetbit_MODULE_DBG1RESET  },
    [MOD__DBG0RESET]    = {"CMUMOD_DBG0RESET",  NULL,                           &resetbit_MODULE_DBG0RESET  },
    [MOD__WD3RESET]     = {"CMUMOD_WD3RESET",   NULL,                           &resetbit_MODULE_WD3RESET   },
    [MOD__WD2RESET]     = {"CMUMOD_WD2RESET",   NULL,                           &resetbit_MODULE_WD2RESET   },
    [MOD__WD1RESET]     = {"CMUMOD_WD1RESET",   NULL,                           &resetbit_MODULE_WD1RESET   },
    [MOD__WD0RESET]     = {"CMUMOD_WD0RESET",   NULL,                           &resetbit_MODULE_WD0RESET   },
    [MOD__CHIPID]       = {"CMUMOD_CHIPID",     NULL,                           &resetbit_MODULE_CHIPID     },
    [MOD__USB3]         = {"CMUMOD_USB3",       NULL,                           &resetbit_MODULE_USB3       },
    [MOD__USB2_0]       = {"CMUMOD_USB2_0",     NULL,                           &resetbit_MODULE_USB2_0     },
    [MOD__USB2_1]       = {"CMUMOD_USB2_1",     NULL,                           &resetbit_MODULE_USB2_1     },
};

static struct owl_refertab T_lcd    = {{1, 7, -1}, 0};
static struct owl_refertab T_dsipro = {{3 << 16 | 4, 4, -1}, 1};
static struct owl_refertab T_sdx2   = {{1, 128, -1}, 0};
static struct owl_refertab T_uart   = {{624, -1}, 0};
static struct owl_refertab T_rmii   = {{4, 10, -1}, 0};
static struct owl_refertab T_de    = {{1, 2<<16 | 3, 2, 2<<16 | 5, 3, 4, 6, 8, 12}, 1};
static struct owl_refertab T_vde    = {{1, 2<<16 | 3, 2, 2<<16 | 5, 3, 4, 6, 8, -1}, 1};
static struct owl_refertab T_vce    = {{1, 2<<16 | 3, 2, 2<<16 | 5, 3, 4, 6, 8, -1}, 1};
static struct owl_refertab T_gpu    = {{1, 2<<16 | 3, 2, 2<<16 | 5, 3, 4, 6, 8, -1}, 1};

static struct owl_seqtab S_default  = {9, {1, 2, 3, 4, 6, 8, 12, 16, 24} };

static struct owl_compdiv C_dsipro = {
    .sections[0] = {DIV_T_NATURE, 0, 1, {0} },
    .sections[1] = {
        .type = DIV_T_TABLE,
        .range_from = 2,
        .range_to = 3,
        .ext = { .tab = &T_dsipro, },
    },
};


static struct owl_compdiv C_uart = {
    .sections[0] = {DIV_T_NATURE, 0, 312, {0} },
    .sections[1] = {
        .type = DIV_T_TABLE,
        .range_from = 313,
        .range_to = 313,
        .ext = { .tab = &T_uart, },
    },
};


static struct owl_div divider_SPDIF_CLK = {
    .type = DIV_T_SEQ,
    .range_from = 0,
    .range_to = 8,
    .ext = {.seq = &S_default,},
    .reg = &divbit_SPDIF_CLK,
};
static struct owl_div divider_HDMIA_CLK = {
    .type = DIV_T_SEQ,
    .range_from = 0,
    .range_to = 8,
    .ext = {.seq = &S_default,},
    .reg = &divbit_HDMIA_CLK,
};
static struct owl_div divider_I2SRX_CLK = {
    .type = DIV_T_SEQ,
    .range_from = 0,
    .range_to = 8,
    .ext = {.seq = &S_default,},
    .reg = &divbit_I2SRX_CLK,
};
static struct owl_div divider_I2STX_CLK = {
    .type = DIV_T_SEQ,
    .range_from = 0,
    .range_to = 8,
    .ext = {.seq = &S_default,},
    .reg = &divbit_I2STX_CLK,
};
static struct owl_div divider_APBDBG_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 1,
    .range_to = 7,
    .reg = &divbit_APBDBG_CLK,
};
static struct owl_div divider_ACP_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 1,
    .range_to = 3,
    .reg = &divbit_ACP_CLK,
};
static struct owl_div divider_PERIPH_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 1,
    .range_to = 7,
    .reg = &divbit_PERIPH_CLK,
};
static struct owl_div divider_NIC_DIV_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 1, /* reserve NIC_DIV_CLK divisor 1 */
    .range_to = 3,
    .reg = &divbit_NIC_DIV_CLK,
};
static struct owl_div divider_NIC_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 3,
    .reg = &divbit_NIC_CLK,
};
static struct owl_div divider_AHBPREDIV_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 3,
    .reg = &divbit_AHBPREDIV_CLK,
};
static struct owl_div divider_H_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 1,  /* reserve H_CLK divsor 1 */
    .range_to = 3,
    .reg = &divbit_H_CLK,
};
static struct owl_div divider_APB30_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 3,
    .reg = &divbit_APB30_CLK,
};
static struct owl_div divider_APB20_CLK = {
    .type = DIV_T_SEQ_D2,
    .range_from = 0,
    .range_to = 5,
    .ext = {.seq = &S_default,},
    .reg = &divbit_APB20_CLK,
};
static struct owl_div divider_SENSOR_CLKOUT0 = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_SENSOR_CLKOUT0,
};
static struct owl_div divider_SENSOR_CLKOUT1 = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_SENSOR_CLKOUT1,
};
static struct owl_div divider_LCD_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 1,
    .ext = {.tab = &T_lcd,},
    .reg = &divbit_LCD_CLK,
};
static struct owl_div divider_LCD1_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_LCD1_CLK,
};
static struct owl_div divider_LCD0_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_LCD0_CLK,
};
static struct owl_div divider_DSI_HCLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 3,
    .reg = &divbit_DSI_HCLK,
};
static struct owl_div divider_PRO_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 3,
    .ext = {.comp = &C_dsipro,},
    .reg = &divbit_PRO_CLK,
};
static struct owl_div divider_CSI_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_CSI_CLK,
};
static struct owl_div divider_DE1_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 8,
    .ext = {.tab = &T_de,},
    .reg = &divbit_DE1_CLK,
};
static struct owl_div divider_DE2_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 8,
    .ext = {.tab = &T_de,},
    .reg = &divbit_DE2_CLK,
};
static struct owl_div divider_BISP_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_BISP_CLK,
};
static struct owl_div divider_VDE_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_vde,},
    .reg = &divbit_VDE_CLK,
};
static struct owl_div divider_VCE_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_vce,},
    .reg = &divbit_VCE_CLK,
};
static struct owl_div divider_NANDC_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 7,
    .reg = &divbit_NANDC_CLK,
};
static struct owl_div divider_ECC_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 7,
    .reg = &divbit_ECC_CLK,
};
static struct owl_div divider_PRESD0_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 24,
    .reg = &divbit_PRESD0_CLK,
};
static struct owl_div divider_PRESD1_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 24,
    .reg = &divbit_PRESD1_CLK,
};
static struct owl_div divider_PRESD2_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 24,
    .reg = &divbit_PRESD2_CLK,
};
static struct owl_div divider_SD0_CLK_2X = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 1,
    .ext = {.tab = &T_sdx2,},
    .reg = &divbit_SD0_CLK_2X,
};
static struct owl_div divider_SD1_CLK_2X = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 1,
    .ext = {.tab = &T_sdx2,},
    .reg = &divbit_SD1_CLK_2X,
};
static struct owl_div divider_SD2_CLK_2X = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 1,
    .ext = {.tab = &T_sdx2,},
    .reg = &divbit_SD2_CLK_2X,
};
static struct owl_div divider_UART0_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART0_CLK,
};
static struct owl_div divider_UART1_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART1_CLK,
};
static struct owl_div divider_UART2_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART2_CLK,
};
static struct owl_div divider_UART3_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART3_CLK,
};
static struct owl_div divider_UART4_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART4_CLK,
};
static struct owl_div divider_UART5_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART5_CLK,
};
static struct owl_div divider_UART6_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART6_CLK,
};
static struct owl_div divider_PWM0_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM0_CLK,
};
static struct owl_div divider_PWM1_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM1_CLK,
};
static struct owl_div divider_PWM2_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM2_CLK,
};
static struct owl_div divider_PWM3_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM3_CLK,
};
static struct owl_div divider_PWM4_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM4_CLK,
};
static struct owl_div divider_PWM5_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM5_CLK,
};
static struct owl_div divider_RMII_REF_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 1,
    .ext = {.tab = &T_rmii,},
    .reg = &divbit_RMII_REF_CLK,
};
static struct owl_div divider_LENS_CLK = {
    .type = DIV_T_EXP_D2,
    .range_from = 0,
    .range_to = 7,
    .reg = &divbit_LENS_CLK,
};
static struct owl_div divider_GPU3D_SYSCLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_gpu,},
    .reg = &divbit_GPU3D_SYSCLK,
};
static struct owl_div divider_GPU3D_HYDCLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_gpu,},
    .reg = &divbit_GPU3D_HYDCLK,
};
static struct owl_div divider_GPU3D_NIC_MEMCLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_gpu,},
    .reg = &divbit_GPU3D_NIC_MEMCLK,
};
static struct owl_div divider_GPU3D_CORECLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_gpu,},
    .reg = &divbit_GPU3D_CORECLK,
};
static struct owl_div divider_SS_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_SS_CLK,
};


/*
 * parent names init data for common clocks
 */
static const char *parent_names_COREPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_DEVPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_DDRPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_NANDPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_DISPLAYPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_AUDIOPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_TVOUTPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_ETHERNETPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_CVBSPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_DEV_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_DDR_CLK_0[] = {
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_DDR_CLK_90[] = {
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_DDR_CLK_180[] = {
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_DDR_CLK_270[] = {
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_DDR_CLK_CH0[] = {
	clocks[CLOCK__DDR_CLK_0].name,
};
static const char *parent_names_DDR_CLK_CH1[] = {
	clocks[CLOCK__DDR_CLK_0].name,
	clocks[CLOCK__DDR_CLK_90].name,
	clocks[CLOCK__DDR_CLK_180].name,
	clocks[CLOCK__DDR_CLK_270].name,
};
static const char *parent_names_DDR_CLK[] = {
	clocks[CLOCK__DDR_CLK_0].name,
};
static const char *parent_names_SPDIF_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_HDMIA_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_I2SRX_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_I2STX_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_PCM0_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_PCM1_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_CLK_PIXEL[] = {
	clocks[CLOCK__TVOUTPLL].name,
};
static const char *parent_names_CLK_TMDS[] = {
	clocks[CLOCK__TVOUTPLL].name,
};
static const char *parent_names_CLK_TMDS_PHY_P[] = {
	clocks[CLOCK__CLK_TMDS].name,
};
static const char *parent_names_L2_NIC_CLK[] = {
	clocks[CLOCK__CORE_CLK].name,
};
static const char *parent_names_APBDBG_CLK[] = {
	clocks[CLOCK__CPU_CLK].name,
};
static const char *parent_names_L2_CLK[] = {
	clocks[CLOCK__CPU_CLK].name,
};
static const char *parent_names_ACP_CLK[] = {
	clocks[CLOCK__CPU_CLK].name,
};
static const char *parent_names_PERIPH_CLK[] = {
	clocks[CLOCK__CPU_CLK].name,
};
static const char *parent_names_NIC_DIV_CLK[] = {
	clocks[CLOCK__NIC_CLK].name,
};
static const char *parent_names_NIC_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_AHBPREDIV_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
};

static const char *parent_names_H_CLK[] = {
	clocks[CLOCK__AHBPREDIV_CLK].name,
};
static const char *parent_names_APB30_CLK[] = {
	clocks[CLOCK__NIC_CLK].name,
};
static const char *parent_names_APB20_CLK[] = {
	clocks[CLOCK__NIC_CLK].name,
};
static const char *parent_names_AHB_CLK[] = {
	clocks[CLOCK__H_CLK].name,
};
static const char *parent_names_CORE_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__COREPLL].name,
	clocks[CLOCK__VCE_CLK].name,
};
static const char *parent_names_CPU_CLK[] = {
	clocks[CLOCK__CORE_CLK].name,
};
static const char *parent_names_SENSOR_CLKOUT0[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__ISPBP_CLK].name,
};
static const char *parent_names_SENSOR_CLKOUT1[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__ISPBP_CLK].name,
};
static const char *parent_names_LCD_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
};
static const char *parent_names_LVDS_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
};
static const char *parent_names_CKA_LCD_H[] = {
	clocks[CLOCK__LVDS_CLK].name,
};
static const char *parent_names_LCD1_CLK[] = {
	clocks[CLOCK__LCD_CLK].name,
};
static const char *parent_names_LCD0_CLK[] = {
	clocks[CLOCK__LCD_CLK].name,
};
static const char *parent_names_DSI_HCLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
};
static const char *parent_names_DSI_HCLK90[] = {
	clocks[CLOCK__DSI_HCLK].name,
};
static const char *parent_names_PRO_CLK[] = {
	clocks[CLOCK__DSI_HCLK90].name,
};
static const char *parent_names_PHY_CLK[] = {
	clocks[CLOCK__DSI_HCLK90].name,
};
static const char *parent_names_CSI_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
};
static const char *parent_names_DE1_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
};
static const char *parent_names_DE2_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
};
static const char *parent_names_BISP_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
};
static const char *parent_names_ISPBP_CLK[] = {
	clocks[CLOCK__BISP_CLK].name,
};
static const char *parent_names_IMG5_CLK[] = {
	clocks[CLOCK__LCD1_CLK].name,
	clocks[CLOCK__LCD0_CLK].name,
};
static const char *parent_names_VDE_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_VCE_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_NANDC_CLK[] = {
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_ECC_CLK[] = {
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_PRESD0_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__NANDPLL].name,
};
static const char *parent_names_PRESD1_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__NANDPLL].name,
};
static const char *parent_names_PRESD2_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__NANDPLL].name,
};
static const char *parent_names_SD0_CLK_2X[] = {
	clocks[CLOCK__PRESD0_CLK].name,
};
static const char *parent_names_SD1_CLK_2X[] = {
	clocks[CLOCK__PRESD1_CLK].name,
};
static const char *parent_names_SD2_CLK_2X[] = {
	clocks[CLOCK__PRESD2_CLK].name,
};
static const char *parent_names_SD0_CLK[] = {
	clocks[CLOCK__SD0_CLK_2X].name,
};
static const char *parent_names_SD1_CLK[] = {
	clocks[CLOCK__SD1_CLK_2X].name,
};
static const char *parent_names_SD2_CLK[] = {
	clocks[CLOCK__SD2_CLK_2X].name,
};
static const char *parent_names_UART0_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART1_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART2_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART3_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART4_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART5_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART6_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_PWM0_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_PWM1_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_PWM2_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_PWM3_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_PWM4_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_PWM5_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_RMII_REF_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_I2C0_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_I2C1_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_I2C2_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_I2C3_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_25M_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_LENS_CLK[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_HDMI24M[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_TIMER_CLK[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_SS_CLK[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_SPS_CLK[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_IRC_CLK[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_TV24M[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_CVBS_CLK108M[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_MIPI24M[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_LENS24M[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_GPU3D_SYSCLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
	clocks[CLOCK__CVBSPLL].name,
};
static const char *parent_names_GPU3D_HYDCLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
	clocks[CLOCK__CVBSPLL].name,
};
static const char *parent_names_GPU3D_NIC_MEMCLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
	clocks[CLOCK__CVBSPLL].name,
};
static const char *parent_names_GPU3D_CORECLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
	clocks[CLOCK__CVBSPLL].name,
};

static const char *parent_name_CMUMOD_DEVCLKS[] = {
	modnode[MOD__ROOT].modname,
};

static struct clk_onecell_data clk_data;

/*
 * common clk static init.
 */
static struct clk *owl_clks[CLOCK__MAX + MOD__MAX_IN_CLK];

/*
 * clock imp: "real" clocks
 */
static struct owl_clk_foo clk_foo_clocks[CLOCK__MAX] = {
    [CLOCK__HOSC] = {
        .clock = CLOCK__HOSC,
    },
    [CLOCK__IC_32K] = {
        .clock = CLOCK__IC_32K,
    },
    [CLOCK__COREPLL] = {
        .clock = CLOCK__COREPLL,
    },
    [CLOCK__DEVPLL] = {
        .clock = CLOCK__DEVPLL,
    },
    [CLOCK__DDRPLL] = {
        .clock = CLOCK__DDRPLL,
    },
    [CLOCK__NANDPLL] = {
        .clock = CLOCK__NANDPLL,
    },
    [CLOCK__DISPLAYPLL] = {
        .clock = CLOCK__DISPLAYPLL,
    },
    [CLOCK__AUDIOPLL] = {
        .clock = CLOCK__AUDIOPLL,
    },
    [CLOCK__TVOUTPLL] = {
        .clock = CLOCK__TVOUTPLL,
    },
    [CLOCK__ETHERNETPLL] = {
        .clock = CLOCK__ETHERNETPLL,
    },
    [CLOCK__CVBSPLL] = {
        .clock = CLOCK__CVBSPLL,
    },
    [CLOCK__DEV_CLK] = {
        .clock = CLOCK__DEV_CLK,
    },
    [CLOCK__DDR_CLK_0] = {
        .clock = CLOCK__DDR_CLK_0,
    },
    [CLOCK__DDR_CLK_90] = {
        .clock = CLOCK__DDR_CLK_90,
    },
    [CLOCK__DDR_CLK_180] = {
        .clock = CLOCK__DDR_CLK_180,
    },
    [CLOCK__DDR_CLK_270] = {
        .clock = CLOCK__DDR_CLK_270,
    },
    [CLOCK__DDR_CLK_CH0] = {
        .clock = CLOCK__DDR_CLK_CH0,
    },
    [CLOCK__DDR_CLK_CH1] = {
        .clock = CLOCK__DDR_CLK_CH1,
    },
    [CLOCK__DDR_CLK] = {
        .clock = CLOCK__DDR_CLK,
    },
    [CLOCK__SPDIF_CLK] = {
        .clock = CLOCK__SPDIF_CLK,
    },
    [CLOCK__HDMIA_CLK] = {
        .clock = CLOCK__HDMIA_CLK,
    },
    [CLOCK__I2SRX_CLK] = {
        .clock = CLOCK__I2SRX_CLK,
    },
    [CLOCK__I2STX_CLK] = {
        .clock = CLOCK__I2STX_CLK,
    },
    [CLOCK__PCM0_CLK] = {
        .clock = CLOCK__PCM0_CLK,
    },
    [CLOCK__PCM1_CLK] = {
        .clock = CLOCK__PCM1_CLK,
    },
    [CLOCK__CLK_PIXEL] = {
        .clock = CLOCK__CLK_PIXEL,
    },
    [CLOCK__CLK_TMDS] = {
        .clock = CLOCK__CLK_TMDS,
    },
    [CLOCK__CLK_TMDS_PHY_P] = {
        .clock = CLOCK__CLK_TMDS_PHY_P,
    },
    [CLOCK__L2_NIC_CLK] = {
        .clock = CLOCK__L2_NIC_CLK,
    },
    [CLOCK__APBDBG_CLK] = {
        .clock = CLOCK__APBDBG_CLK,
    },
    [CLOCK__L2_CLK] = {
        .clock = CLOCK__L2_CLK,
    },
    [CLOCK__ACP_CLK] = {
        .clock = CLOCK__ACP_CLK,
    },
    [CLOCK__PERIPH_CLK] = {
        .clock = CLOCK__PERIPH_CLK,
    },
    [CLOCK__NIC_DIV_CLK] = {
        .clock = CLOCK__NIC_DIV_CLK,
    },
    [CLOCK__NIC_CLK] = {
        .clock = CLOCK__NIC_CLK,
    },
    [CLOCK__AHBPREDIV_CLK] = {
        .clock = CLOCK__AHBPREDIV_CLK,
    },
    [CLOCK__H_CLK] = {
        .clock = CLOCK__H_CLK,
    },
    [CLOCK__APB30_CLK] = {
        .clock = CLOCK__APB30_CLK,
    },
    [CLOCK__APB20_CLK] = {
        .clock = CLOCK__APB20_CLK,
    },
    [CLOCK__AHB_CLK] = {
        .clock = CLOCK__AHB_CLK,
    },
    [CLOCK__CORE_CLK] = {
        .clock = CLOCK__CORE_CLK,
    },
    [CLOCK__CPU_CLK] = {
        .clock = CLOCK__CPU_CLK,
    },
    [CLOCK__SENSOR_CLKOUT0] = {
        .clock = CLOCK__SENSOR_CLKOUT0,
    },
    [CLOCK__SENSOR_CLKOUT1] = {
        .clock = CLOCK__SENSOR_CLKOUT1,
    },
    [CLOCK__LCD_CLK] = {
        .clock = CLOCK__LCD_CLK,
    },
    [CLOCK__LVDS_CLK] = {
        .clock = CLOCK__LVDS_CLK,
    },
    [CLOCK__CKA_LCD_H] = {
        .clock = CLOCK__CKA_LCD_H,
    },
    [CLOCK__LCD1_CLK] = {
        .clock = CLOCK__LCD1_CLK,
    },
    [CLOCK__LCD0_CLK] = {
        .clock = CLOCK__LCD0_CLK,
    },
    [CLOCK__DSI_HCLK] = {
        .clock = CLOCK__DSI_HCLK,
    },
    [CLOCK__DSI_HCLK90] = {
        .clock = CLOCK__DSI_HCLK90,
    },
    [CLOCK__PRO_CLK] = {
        .clock = CLOCK__PRO_CLK,
    },
    [CLOCK__PHY_CLK] = {
        .clock = CLOCK__PHY_CLK,
    },
    [CLOCK__CSI_CLK] = {
        .clock = CLOCK__CSI_CLK,
    },
    [CLOCK__DE1_CLK] = {
        .clock = CLOCK__DE1_CLK,
    },
    [CLOCK__DE2_CLK] = {
        .clock = CLOCK__DE2_CLK,
    },
    [CLOCK__BISP_CLK] = {
        .clock = CLOCK__BISP_CLK,
    },
    [CLOCK__ISPBP_CLK] = {
        .clock = CLOCK__ISPBP_CLK,
    },
    [CLOCK__IMG5_CLK] = {
        .clock = CLOCK__IMG5_CLK,
    },
    [CLOCK__VDE_CLK] = {
        .clock = CLOCK__VDE_CLK,
    },
    [CLOCK__VCE_CLK] = {
        .clock = CLOCK__VCE_CLK,
    },
    [CLOCK__NANDC_CLK] = {
        .clock = CLOCK__NANDC_CLK,
    },
    [CLOCK__ECC_CLK] = {
        .clock = CLOCK__ECC_CLK,
    },
    [CLOCK__PRESD0_CLK] = {
        .clock = CLOCK__PRESD0_CLK,
    },
    [CLOCK__PRESD1_CLK] = {
        .clock = CLOCK__PRESD1_CLK,
    },
    [CLOCK__PRESD2_CLK] = {
        .clock = CLOCK__PRESD2_CLK,
    },
    [CLOCK__SD0_CLK_2X] = {
        .clock = CLOCK__SD0_CLK_2X,
    },
    [CLOCK__SD1_CLK_2X] = {
        .clock = CLOCK__SD1_CLK_2X,
    },
    [CLOCK__SD2_CLK_2X] = {
        .clock = CLOCK__SD2_CLK_2X,
    },
    [CLOCK__SD0_CLK] = {
        .clock = CLOCK__SD0_CLK,
    },
    [CLOCK__SD1_CLK] = {
        .clock = CLOCK__SD1_CLK,
    },
    [CLOCK__SD2_CLK] = {
        .clock = CLOCK__SD2_CLK,
    },
    [CLOCK__UART0_CLK] = {
        .clock = CLOCK__UART0_CLK,
    },
    [CLOCK__UART1_CLK] = {
        .clock = CLOCK__UART1_CLK,
    },
    [CLOCK__UART2_CLK] = {
        .clock = CLOCK__UART2_CLK,
    },
    [CLOCK__UART3_CLK] = {
        .clock = CLOCK__UART3_CLK,
    },
    [CLOCK__UART4_CLK] = {
        .clock = CLOCK__UART4_CLK,
    },
    [CLOCK__UART5_CLK] = {
        .clock = CLOCK__UART5_CLK,
    },
    [CLOCK__UART6_CLK] = {
        .clock = CLOCK__UART6_CLK,
    },
    [CLOCK__PWM0_CLK] = {
        .clock = CLOCK__PWM0_CLK,
    },
    [CLOCK__PWM1_CLK] = {
        .clock = CLOCK__PWM1_CLK,
    },
    [CLOCK__PWM2_CLK] = {
        .clock = CLOCK__PWM2_CLK,
    },
    [CLOCK__PWM3_CLK] = {
        .clock = CLOCK__PWM3_CLK,
    },
    [CLOCK__PWM4_CLK] = {
        .clock = CLOCK__PWM4_CLK,
    },
    [CLOCK__PWM5_CLK] = {
        .clock = CLOCK__PWM5_CLK,
    },
    [CLOCK__RMII_REF_CLK] = {
        .clock = CLOCK__RMII_REF_CLK,
    },
    [CLOCK__I2C0_CLK] = {
        .clock = CLOCK__I2C0_CLK,
    },
    [CLOCK__I2C1_CLK] = {
        .clock = CLOCK__I2C1_CLK,
    },
    [CLOCK__I2C2_CLK] = {
        .clock = CLOCK__I2C2_CLK,
    },
    [CLOCK__I2C3_CLK] = {
        .clock = CLOCK__I2C3_CLK,
    },
    [CLOCK__25M_CLK] = {
        .clock = CLOCK__25M_CLK,
    },
    [CLOCK__LENS_CLK] = {
        .clock = CLOCK__LENS_CLK,
    },
    [CLOCK__HDMI24M] = {
        .clock = CLOCK__HDMI24M,
    },
    [CLOCK__TIMER_CLK] = {
        .clock = CLOCK__TIMER_CLK,
    },
    [CLOCK__SS_CLK] = {
        .clock = CLOCK__SS_CLK,
    },
    [CLOCK__SPS_CLK] = {
        .clock = CLOCK__SPS_CLK,
    },
    [CLOCK__IRC_CLK] = {
        .clock = CLOCK__IRC_CLK,
    },
    [CLOCK__TV24M] = {
        .clock = CLOCK__TV24M,
    },
    [CLOCK__CVBS_CLK108M] = {
        .clock = CLOCK__CVBS_CLK108M,
    },
    [CLOCK__MIPI24M] = {
        .clock = CLOCK__MIPI24M,
    },
    [CLOCK__LENS24M] = {
        .clock = CLOCK__LENS24M,
    },
    [CLOCK__GPU3D_SYSCLK] = {
        .clock = CLOCK__GPU3D_SYSCLK,
    },
    [CLOCK__GPU3D_HYDCLK] = {
        .clock = CLOCK__GPU3D_HYDCLK,
    },
    [CLOCK__GPU3D_NIC_MEMCLK] = {
        .clock = CLOCK__GPU3D_NIC_MEMCLK,
    },
    [CLOCK__GPU3D_CORECLK] = {
        .clock = CLOCK__GPU3D_CORECLK,
    },
};

/*
 * clock imp: module level clocks, treat as a "virtual" clock
 */
static struct  owl_clk_foo clk_foo_modules[MOD__MAX_IN_CLK] = {
    [MOD__ROOT] = {
        .clock = MOD__ROOT,
    },
    [MOD__GPU3D] = {
        .clock = MOD__GPU3D,
    },
    [MOD__SHARESRAM] = {
        .clock = MOD__SHARESRAM,
    },
    [MOD__HDCP2X] = {
        .clock = MOD__HDCP2X,
    },
    [MOD__VCE] = {
        .clock = MOD__VCE,
    },
    [MOD__VDE] = {
        .clock = MOD__VDE,
    },
    [MOD__PCM0] = {
        .clock = MOD__PCM0,
    },
    [MOD__SPDIF] = {
        .clock = MOD__SPDIF,
    },
    [MOD__HDMIA] = {
        .clock = MOD__HDMIA,
    },
    [MOD__I2SRX] = {
        .clock = MOD__I2SRX,
    },
    [MOD__I2STX] = {
        .clock = MOD__I2STX,
    },
    [MOD__GPIO] = {
        .clock = MOD__GPIO,
    },
    [MOD__KEY] = {
        .clock = MOD__KEY,
    },
    [MOD__LENS] = {
        .clock = MOD__LENS,
    },
    [MOD__BISP] = {
        .clock = MOD__BISP,
    },
    [MOD__CSI] = {
        .clock = MOD__CSI,
    },
    [MOD__DSI] = {
        .clock = MOD__DSI,
    },
    [MOD__LVDS] = {
        .clock = MOD__LVDS,
    },
    [MOD__LCD1] = {
        .clock = MOD__LCD1,
    },
    [MOD__LCD0] = {
        .clock = MOD__LCD0,
    },
    [MOD__DE] = {
        .clock = MOD__DE,
    },
    [MOD__SD2] = {
        .clock = MOD__SD2,
    },
    [MOD__SD1] = {
        .clock = MOD__SD1,
    },
    [MOD__SD0] = {
        .clock = MOD__SD0,
    },
    [MOD__NANDC] = {
        .clock = MOD__NANDC,
    },
    [MOD__DDRCH0] = {
        .clock = MOD__DDRCH0,
    },
    [MOD__NOR] = {
        .clock = MOD__NOR,
    },
    [MOD__DMAC] = {
        .clock = MOD__DMAC,
    },
    [MOD__DDRCH1] = {
        .clock = MOD__DDRCH1,
    },
    [MOD__I2C3] = {
        .clock = MOD__I2C3,
    },
    [MOD__I2C2] = {
        .clock = MOD__I2C2,
    },
    [MOD__TIMER] = {
        .clock = MOD__TIMER,
    },
    [MOD__PWM5] = {
        .clock = MOD__PWM5,
    },
    [MOD__PWM4] = {
        .clock = MOD__PWM4,
    },
    [MOD__PWM3] = {
        .clock = MOD__PWM3,
    },
    [MOD__PWM2] = {
        .clock = MOD__PWM2,
    },
    [MOD__PWM1] = {
        .clock = MOD__PWM1,
    },
    [MOD__PWM0] = {
        .clock = MOD__PWM0,
    },
    [MOD__ETHERNET] = {
        .clock = MOD__ETHERNET,
    },
    [MOD__UART5] = {
        .clock = MOD__UART5,
    },
    [MOD__UART4] = {
        .clock = MOD__UART4,
    },
    [MOD__UART3] = {
        .clock = MOD__UART3,
    },
    [MOD__UART6] = {
        .clock = MOD__UART6,
    },
    [MOD__PCM1] = {
        .clock = MOD__PCM1,
    },
    [MOD__I2C1] = {
        .clock = MOD__I2C1,
    },
    [MOD__I2C0] = {
        .clock = MOD__I2C0,
    },
    [MOD__SPI3] = {
        .clock = MOD__SPI3,
    },
    [MOD__SPI2] = {
        .clock = MOD__SPI2,
    },
    [MOD__SPI1] = {
        .clock = MOD__SPI1,
    },
    [MOD__SPI0] = {
        .clock = MOD__SPI0,
    },
    [MOD__IRC] = {
        .clock = MOD__IRC,
    },
    [MOD__UART2] = {
        .clock = MOD__UART2,
    },
    [MOD__UART1] = {
        .clock = MOD__UART1,
    },
    [MOD__UART0] = {
        .clock = MOD__UART0,
    },
    [MOD__HDMI] = {
        .clock = MOD__HDMI,
    },
    [MOD__SS] = {
        .clock = MOD__SS,
    },
    [MOD__TV24M] = {
        .clock = MOD__TV24M,
    },
    [MOD__CVBS_CLK108M] = {
        .clock = MOD__CVBS_CLK108M,
    },
    [MOD__TVOUT] = {
        .clock = MOD__TVOUT,
    },
};

static int hoscops_enable(struct clk_hw *hw)
{
	write_clkreg_val(&enablebit_HOSC, 1);
	return 0;
}

static void hoscops_disable(struct clk_hw *hw)
{
	write_clkreg_val(&enablebit_HOSC, 1);
}

static int  hoscops_is_enabled(struct clk_hw *hw)
{
	int ret;
	ret = read_clkreg_val(&enablebit_HOSC);
	return ret;
}

#define owl_to_clk_fixed_rate(_hw) container_of(_hw, struct owl_clk_info, hw)

static unsigned long owl_clk_fixed_rate_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	return owl_to_clk_fixed_rate(hw)->fixed_rate;
}

static unsigned long owl_clk_fixed_rate_recalc_accuracy(struct clk_hw *hw,
		unsigned long parent_accuracy)
{
	return owl_to_clk_fixed_rate(hw)->fixed_accuracy;
}

static struct clk_ops clk_ops_gate_hosc = {
	.enable = hoscops_enable,
	.disable = hoscops_disable,
	.is_enabled = hoscops_is_enabled,
	.recalc_rate = owl_clk_fixed_rate_recalc_rate,
	.recalc_accuracy = owl_clk_fixed_rate_recalc_accuracy,
};

static struct clk_ops clk_ops_gate_ic32k = {
	.recalc_rate = owl_clk_fixed_rate_recalc_rate,
	.recalc_accuracy = owl_clk_fixed_rate_recalc_accuracy,
};

struct clk *owl_clk_register_fixed_rate_with_accuracy(struct device *dev,
		const char *name, const char *parent_name, unsigned long flags, struct clk_ops *clk_ops,
		unsigned long fixed_rate, unsigned long fixed_accuracy, u32 clk_id)
{
	struct owl_clk_info *info;
	struct clk *clk;
	struct clk_init_data init;

	/* allocate fixed-rate clock */
	info = kzalloc(sizeof(struct owl_clk_info), GFP_KERNEL);
	if (!info) {
		pr_err("%s: could not allocate fixed clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	init.ops = clk_ops;
	init.flags = flags | CLK_IS_BASIC;
	init.parent_names = (parent_name ? &parent_name: NULL);
	init.num_parents = (parent_name ? 1 : 0);

	/* struct clk_fixed_rate assignments */
	info->fixed_rate = fixed_rate;
	info->fixed_accuracy = fixed_accuracy;
	info->hw.init = &init;
	info->clock_id = clk_id;

	/* register the clock */
	clk = clk_register(dev, &info->hw);
	if (IS_ERR(clk))
		kfree(info);

	return clk;
}


static struct clk *owl_clk_register_gate(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags, struct clk_ops *clk_ops,
		void __iomem *reg, u8 bit_idx,
		u8 clk_gate_flags, spinlock_t *lock, u32 clk_id)
{
	struct owl_clk_info *info;
	struct clk *clk;
	struct clk_init_data init;

	if (clk_gate_flags & CLK_GATE_HIWORD_MASK) {
		if (bit_idx > 15) {
			pr_err("gate bit exceeds LOWORD field\n");
			return ERR_PTR(-EINVAL);
		}
	}

	/* allocate the gate */
	info = kzalloc(sizeof(struct owl_clk_info), GFP_KERNEL);
	if (!info) {
		pr_err("%s: could not allocate info clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	init.ops = clk_ops;
	init.flags = flags | CLK_IS_BASIC;
	init.parent_names = (parent_name ? &parent_name: NULL);
	init.num_parents = (parent_name ? 1 : 0);

	/* struct clk_gate assignments */
	info->reg = reg;
	info->bit_idx = bit_idx;
	info->flags = clk_gate_flags;
	info->lock = lock;
	info->hw.init = &init;
	info->clock_id = clk_id;

	clk = clk_register(dev, &info->hw);

	if (IS_ERR(clk))
		kfree(info);

	return clk;
}

static struct clk *owl_clk_register_mux_table(struct device *dev, const char *name,
		const char **parent_names, u8 num_parents, unsigned long flags, struct clk_ops *clk_ops,
		void __iomem *reg, u8 shift, u32 mask,
		u8 clk_mux_flags, u32 *table, spinlock_t *lock, u32 clk_id)
{
	struct owl_clk_info *info;
	struct clk *clk;
	struct clk_init_data init;
	u8 width = 0;

	if (clk_mux_flags & CLK_MUX_HIWORD_MASK) {
		width = fls(mask) - ffs(mask) + 1;
		if (width + shift > 16) {
			pr_err("mux value exceeds LOWORD field\n");
			return ERR_PTR(-EINVAL);
		}
	}

	/* allocate the mux */
	info = kzalloc(sizeof(struct owl_clk_info), GFP_KERNEL);
	if (!info) {
		pr_err("%s: could not allocate info clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	if (clk_mux_flags & CLK_MUX_READ_ONLY)
		init.ops = clk_ops;
	else
		init.ops = clk_ops;
	init.flags = flags | CLK_IS_BASIC;
	init.parent_names = parent_names;
	init.num_parents = num_parents;

	/* struct owl_clk_info assignments */
	info->reg = reg;
	info->shift = shift;
	info->mask = mask;
	info->flags = clk_mux_flags;
	info->lock = lock;
	info->table = table;
	info->hw.init = &init;
	info->clock_id = clk_id;

	clk = clk_register(dev, &info->hw);

	if (IS_ERR(clk))
		kfree(info);

	return clk;
}

static struct clk_lookup lookup_table[CLOCK__MAX + MOD__MAX_IN_CLK] = {
    [CLOCK__HOSC] = {
        .con_id = clocks[CLOCK__HOSC].name,
    },
    [CLOCK__IC_32K] = {
        .con_id = clocks[CLOCK__IC_32K].name,
    },
    [CLOCK__COREPLL] = {
        .con_id = clocks[CLOCK__COREPLL].name,
    },
    [CLOCK__DEVPLL] = {
        .con_id = clocks[CLOCK__DEVPLL].name,
    },
    [CLOCK__DDRPLL] = {
        .con_id = clocks[CLOCK__DDRPLL].name,
    },
    [CLOCK__NANDPLL] = {
        .con_id = NULL, /* hide nandpll to ease debug work */
    },
    [CLOCK__DISPLAYPLL] = {
        .con_id = clocks[CLOCK__DISPLAYPLL].name,
    },
    [CLOCK__AUDIOPLL] = {
        .con_id = clocks[CLOCK__AUDIOPLL].name,
    },
    [CLOCK__TVOUTPLL] = {
        .con_id = clocks[CLOCK__TVOUTPLL].name,
    },
    [CLOCK__ETHERNETPLL] = {
        .con_id = clocks[CLOCK__ETHERNETPLL].name,
    },
    [CLOCK__CVBSPLL] = {
        .con_id = clocks[CLOCK__CVBSPLL].name,
    },
    [CLOCK__DEV_CLK] = {
        .con_id = clocks[CLOCK__DEV_CLK].name,
    },
    [CLOCK__DDR_CLK_0] = {
        .con_id = clocks[CLOCK__DDR_CLK_0].name,
    },
    [CLOCK__DDR_CLK_90] = {
        .con_id = clocks[CLOCK__DDR_CLK_90].name,
    },
    [CLOCK__DDR_CLK_180] = {
        .con_id = clocks[CLOCK__DDR_CLK_180].name,
    },
    [CLOCK__DDR_CLK_270] = {
        .con_id = clocks[CLOCK__DDR_CLK_270].name,
    },
    [CLOCK__DDR_CLK_CH0] = {
        .con_id = clocks[CLOCK__DDR_CLK_CH0].name,
    },
    [CLOCK__DDR_CLK_CH1] = {
        .con_id = clocks[CLOCK__DDR_CLK_CH1].name,
    },
    [CLOCK__DDR_CLK] = {
        .con_id = clocks[CLOCK__DDR_CLK].name,
    },
    [CLOCK__SPDIF_CLK] = {
        .con_id = clocks[CLOCK__SPDIF_CLK].name,
    },
    [CLOCK__HDMIA_CLK] = {
        .con_id = clocks[CLOCK__HDMIA_CLK].name,
    },
    [CLOCK__I2SRX_CLK] = {
        .con_id = clocks[CLOCK__I2SRX_CLK].name,
    },
    [CLOCK__I2STX_CLK] = {
        .con_id = clocks[CLOCK__I2STX_CLK].name,
    },
    [CLOCK__PCM0_CLK] = {
        .con_id = clocks[CLOCK__PCM0_CLK].name,
    },
    [CLOCK__PCM1_CLK] = {
        .con_id = clocks[CLOCK__PCM1_CLK].name,
    },
    [CLOCK__CLK_PIXEL] = {
        .con_id = clocks[CLOCK__CLK_PIXEL].name,
    },
    [CLOCK__CLK_TMDS] = {
        .con_id = clocks[CLOCK__CLK_TMDS].name,
    },
    [CLOCK__CLK_TMDS_PHY_P] = {
        .con_id = clocks[CLOCK__CLK_TMDS_PHY_P].name,
    },
    [CLOCK__L2_NIC_CLK] = {
        .con_id = clocks[CLOCK__L2_NIC_CLK].name,
    },
    [CLOCK__APBDBG_CLK] = {
        .con_id = clocks[CLOCK__APBDBG_CLK].name,
    },
    [CLOCK__L2_CLK] = {
        .con_id = clocks[CLOCK__L2_CLK].name,
    },
    [CLOCK__ACP_CLK] = {
        .con_id = clocks[CLOCK__ACP_CLK].name,
    },
    [CLOCK__PERIPH_CLK] = {
        .con_id = clocks[CLOCK__PERIPH_CLK].name,
    },
    [CLOCK__NIC_DIV_CLK] = {
        .con_id = clocks[CLOCK__NIC_DIV_CLK].name,
    },
    [CLOCK__NIC_CLK] = {
        .con_id = clocks[CLOCK__NIC_CLK].name,
    },
    [CLOCK__AHBPREDIV_CLK] = {
        .con_id = clocks[CLOCK__AHBPREDIV_CLK].name,
    },
    [CLOCK__H_CLK] = {
        .con_id = clocks[CLOCK__H_CLK].name,
    },
    [CLOCK__APB30_CLK] = {
        .con_id = clocks[CLOCK__APB30_CLK].name,
    },
    [CLOCK__APB20_CLK] = {
        .con_id = clocks[CLOCK__APB20_CLK].name,
    },
    [CLOCK__AHB_CLK] = {
        .con_id = clocks[CLOCK__AHB_CLK].name,
    },
    [CLOCK__CORE_CLK] = {
        .con_id = clocks[CLOCK__CORE_CLK].name,
    },
    [CLOCK__CPU_CLK] = {
        .con_id = clocks[CLOCK__CPU_CLK].name,
    },
    [CLOCK__SENSOR_CLKOUT0] = {
        .con_id = clocks[CLOCK__SENSOR_CLKOUT0].name,
    },
    [CLOCK__SENSOR_CLKOUT1] = {
        .con_id = clocks[CLOCK__SENSOR_CLKOUT1].name,
    },
    [CLOCK__LCD_CLK] = {
        .con_id = clocks[CLOCK__LCD_CLK].name,
    },
    [CLOCK__LVDS_CLK] = {
        .con_id = clocks[CLOCK__LVDS_CLK].name,
    },
    [CLOCK__CKA_LCD_H] = {
        .con_id = clocks[CLOCK__CKA_LCD_H].name,
    },
    [CLOCK__LCD1_CLK] = {
        .con_id = clocks[CLOCK__LCD1_CLK].name,
    },
    [CLOCK__LCD0_CLK] = {
        .con_id = clocks[CLOCK__LCD0_CLK].name,
    },
    [CLOCK__DSI_HCLK] = {
        .con_id = clocks[CLOCK__DSI_HCLK].name,
    },
    [CLOCK__DSI_HCLK90] = {
        .con_id = clocks[CLOCK__DSI_HCLK90].name,
    },
    [CLOCK__PRO_CLK] = {
        .con_id = clocks[CLOCK__PRO_CLK].name,
    },
    [CLOCK__PHY_CLK] = {
        .con_id = clocks[CLOCK__PHY_CLK].name,
    },
    [CLOCK__CSI_CLK] = {
        .con_id = clocks[CLOCK__CSI_CLK].name,
    },
    [CLOCK__DE1_CLK] = {
        .con_id = clocks[CLOCK__DE1_CLK].name,
    },
    [CLOCK__DE2_CLK] = {
        .con_id = clocks[CLOCK__DE2_CLK].name,
    },
    [CLOCK__BISP_CLK] = {
        .con_id = clocks[CLOCK__BISP_CLK].name,
    },
    [CLOCK__ISPBP_CLK] = {
        .con_id = clocks[CLOCK__ISPBP_CLK].name,
    },
    [CLOCK__IMG5_CLK] = {
        .con_id = clocks[CLOCK__IMG5_CLK].name,
    },
    [CLOCK__VDE_CLK] = {
        .con_id = clocks[CLOCK__VDE_CLK].name,
    },
    [CLOCK__VCE_CLK] = {
        .con_id = clocks[CLOCK__VCE_CLK].name,
    },
    [CLOCK__NANDC_CLK] = {
        .con_id = clocks[CLOCK__NANDC_CLK].name,
    },
    [CLOCK__ECC_CLK] = {
        .con_id = clocks[CLOCK__ECC_CLK].name,
    },
    [CLOCK__PRESD0_CLK] = {
        .con_id = clocks[CLOCK__PRESD0_CLK].name,
    },
    [CLOCK__PRESD1_CLK] = {
        .con_id = clocks[CLOCK__PRESD1_CLK].name,
    },
    [CLOCK__PRESD2_CLK] = {
        .con_id = clocks[CLOCK__PRESD2_CLK].name,
    },
    [CLOCK__SD0_CLK_2X] = {
        .con_id = clocks[CLOCK__SD0_CLK_2X].name,
    },
    [CLOCK__SD1_CLK_2X] = {
        .con_id = clocks[CLOCK__SD1_CLK_2X].name,
    },
    [CLOCK__SD2_CLK_2X] = {
        .con_id = clocks[CLOCK__SD2_CLK_2X].name,
    },
    [CLOCK__SD0_CLK] = {
        .con_id = clocks[CLOCK__SD0_CLK].name,
    },
    [CLOCK__SD1_CLK] = {
        .con_id = clocks[CLOCK__SD1_CLK].name,
    },
    [CLOCK__SD2_CLK] = {
        .con_id = clocks[CLOCK__SD2_CLK].name,
    },
    [CLOCK__UART0_CLK] = {
        .con_id = clocks[CLOCK__UART0_CLK].name,
    },
    [CLOCK__UART1_CLK] = {
        .con_id = clocks[CLOCK__UART1_CLK].name,
    },
    [CLOCK__UART2_CLK] = {
        .con_id = clocks[CLOCK__UART2_CLK].name,
    },
    [CLOCK__UART3_CLK] = {
        .con_id = clocks[CLOCK__UART3_CLK].name,
    },
    [CLOCK__UART4_CLK] = {
        .con_id = clocks[CLOCK__UART4_CLK].name,
    },
    [CLOCK__UART5_CLK] = {
        .con_id = clocks[CLOCK__UART5_CLK].name,
    },
    [CLOCK__UART6_CLK] = {
        .con_id = clocks[CLOCK__UART6_CLK].name,
    },
    [CLOCK__PWM0_CLK] = {
        .con_id = clocks[CLOCK__PWM0_CLK].name,
    },
    [CLOCK__PWM1_CLK] = {
        .con_id = clocks[CLOCK__PWM1_CLK].name,
    },
    [CLOCK__PWM2_CLK] = {
        .con_id = clocks[CLOCK__PWM2_CLK].name,
    },
    [CLOCK__PWM3_CLK] = {
        .con_id = clocks[CLOCK__PWM3_CLK].name,
    },
    [CLOCK__PWM4_CLK] = {
        .con_id = clocks[CLOCK__PWM4_CLK].name,
    },
    [CLOCK__PWM5_CLK] = {
        .con_id = clocks[CLOCK__PWM5_CLK].name,
    },
    [CLOCK__RMII_REF_CLK] = {
        .con_id = clocks[CLOCK__RMII_REF_CLK].name,
    },
    [CLOCK__I2C0_CLK] = {
        .con_id = clocks[CLOCK__I2C0_CLK].name,
    },
    [CLOCK__I2C1_CLK] = {
        .con_id = clocks[CLOCK__I2C1_CLK].name,
    },
    [CLOCK__I2C2_CLK] = {
        .con_id = clocks[CLOCK__I2C2_CLK].name,
    },
    [CLOCK__I2C3_CLK] = {
        .con_id = clocks[CLOCK__I2C3_CLK].name,
    },
    [CLOCK__25M_CLK] = {
        .con_id = clocks[CLOCK__25M_CLK].name,
    },
    [CLOCK__LENS_CLK] = {
        .con_id = clocks[CLOCK__LENS_CLK].name,
    },
    [CLOCK__HDMI24M] = {
        .con_id = clocks[CLOCK__HDMI24M].name,
    },
    [CLOCK__TIMER_CLK] = {
        .con_id = clocks[CLOCK__TIMER_CLK].name,
    },
    [CLOCK__SS_CLK] = {
        .con_id = clocks[CLOCK__SS_CLK].name,
    },
    [CLOCK__SPS_CLK] = {
        .con_id = clocks[CLOCK__SPS_CLK].name,
    },
    [CLOCK__IRC_CLK] = {
        .con_id = clocks[CLOCK__IRC_CLK].name,
    },
    [CLOCK__TV24M] = {
        .con_id = clocks[CLOCK__TV24M].name,
    },
    [CLOCK__CVBS_CLK108M] = {
        .con_id = clocks[CLOCK__CVBS_CLK108M].name,
    },
    [CLOCK__MIPI24M] = {
        .con_id = clocks[CLOCK__MIPI24M].name,
    },
    [CLOCK__LENS24M] = {
        .con_id = clocks[CLOCK__LENS24M].name,
    },
    [CLOCK__GPU3D_SYSCLK] = {
        .con_id = clocks[CLOCK__GPU3D_SYSCLK].name,
    },
    [CLOCK__GPU3D_HYDCLK] = {
        .con_id = clocks[CLOCK__GPU3D_HYDCLK].name,
    },
    [CLOCK__GPU3D_NIC_MEMCLK] = {
        .con_id = clocks[CLOCK__GPU3D_NIC_MEMCLK].name,
    },
    [CLOCK__GPU3D_CORECLK] = {
        .con_id = clocks[CLOCK__GPU3D_CORECLK].name,
    },
    [CLOCK__MAX + MOD__ROOT] = {
        .con_id = modnode[MOD__ROOT].modname,
    },
    [CLOCK__MAX + MOD__GPU3D] = {
        .con_id = modnode[MOD__GPU3D].modname,
    },
    [CLOCK__MAX + MOD__SHARESRAM] = {
        .con_id = modnode[MOD__SHARESRAM].modname,
    },
    [CLOCK__MAX + MOD__HDCP2X] = {
        .con_id = modnode[MOD__HDCP2X].modname,
    },
    [CLOCK__MAX + MOD__VCE] = {
        .con_id = modnode[MOD__VCE].modname,
    },
    [CLOCK__MAX + MOD__VDE] = {
        .con_id = modnode[MOD__VDE].modname,
    },
    [CLOCK__MAX + MOD__PCM0] = {
        .con_id = modnode[MOD__PCM0].modname,
    },
    [CLOCK__MAX + MOD__SPDIF] = {
        .con_id = modnode[MOD__SPDIF].modname,
    },
    [CLOCK__MAX + MOD__HDMIA] = {
        .con_id = modnode[MOD__HDMIA].modname,
    },
    [CLOCK__MAX + MOD__I2SRX] = {
        .con_id = modnode[MOD__I2SRX].modname,
    },
    [CLOCK__MAX + MOD__I2STX] = {
        .con_id = modnode[MOD__I2STX].modname,
    },
    [CLOCK__MAX + MOD__GPIO] = {
        .con_id = modnode[MOD__GPIO].modname,
    },
    [CLOCK__MAX + MOD__KEY] = {
        .con_id = modnode[MOD__KEY].modname,
    },
    [CLOCK__MAX + MOD__LENS] = {
        .con_id = modnode[MOD__LENS].modname,
    },
    [CLOCK__MAX + MOD__BISP] = {
        .con_id = modnode[MOD__BISP].modname,
    },
    [CLOCK__MAX + MOD__CSI] = {
        .con_id = modnode[MOD__CSI].modname,
    },
    [CLOCK__MAX + MOD__DSI] = {
        .con_id = modnode[MOD__DSI].modname,
    },
    [CLOCK__MAX + MOD__LVDS] = {
        .con_id = modnode[MOD__LVDS].modname,
    },
    [CLOCK__MAX + MOD__LCD1] = {
        .con_id = modnode[MOD__LCD1].modname,
    },
    [CLOCK__MAX + MOD__LCD0] = {
        .con_id = modnode[MOD__LCD0].modname,
    },
    [CLOCK__MAX + MOD__DE] = {
        .con_id = modnode[MOD__DE].modname,
    },
    [CLOCK__MAX + MOD__SD2] = {
        .con_id = modnode[MOD__SD2].modname,
    },
    [CLOCK__MAX + MOD__SD1] = {
        .con_id = modnode[MOD__SD1].modname,
    },
    [CLOCK__MAX + MOD__SD0] = {
        .con_id = modnode[MOD__SD0].modname,
    },
    [CLOCK__MAX + MOD__NANDC] = {
        .con_id = modnode[MOD__NANDC].modname,
    },
    [CLOCK__MAX + MOD__DDRCH0] = {
        .con_id = modnode[MOD__DDRCH0].modname,
    },
    [CLOCK__MAX + MOD__NOR] = {
        .con_id = modnode[MOD__NOR].modname,
    },
    [CLOCK__MAX + MOD__DMAC] = {
        .con_id = modnode[MOD__DMAC].modname,
    },
    [CLOCK__MAX + MOD__DDRCH1] = {
        .con_id = modnode[MOD__DDRCH1].modname,
    },
    [CLOCK__MAX + MOD__I2C3] = {
        .con_id = modnode[MOD__I2C3].modname,
    },
    [CLOCK__MAX + MOD__I2C2] = {
        .con_id = modnode[MOD__I2C2].modname,
    },
    [CLOCK__MAX + MOD__TIMER] = {
        .con_id = modnode[MOD__TIMER].modname,
    },
    [CLOCK__MAX + MOD__PWM5] = {
        .con_id = modnode[MOD__PWM5].modname,
    },
    [CLOCK__MAX + MOD__PWM4] = {
        .con_id = modnode[MOD__PWM4].modname,
    },
    [CLOCK__MAX + MOD__PWM3] = {
        .con_id = modnode[MOD__PWM3].modname,
    },
    [CLOCK__MAX + MOD__PWM2] = {
        .con_id = modnode[MOD__PWM2].modname,
    },
    [CLOCK__MAX + MOD__PWM1] = {
        .con_id = modnode[MOD__PWM1].modname,
    },
    [CLOCK__MAX + MOD__PWM0] = {
        .con_id = modnode[MOD__PWM0].modname,
    },
    [CLOCK__MAX + MOD__ETHERNET] = {
        .con_id = modnode[MOD__ETHERNET].modname,
    },
    [CLOCK__MAX + MOD__UART5] = {
        .con_id = modnode[MOD__UART5].modname,
    },
    [CLOCK__MAX + MOD__UART4] = {
        .con_id = modnode[MOD__UART4].modname,
    },
    [CLOCK__MAX + MOD__UART3] = {
        .con_id = modnode[MOD__UART3].modname,
    },
    [CLOCK__MAX + MOD__UART6] = {
        .con_id = modnode[MOD__UART6].modname,
    },
    [CLOCK__MAX + MOD__PCM1] = {
        .con_id = modnode[MOD__PCM1].modname,
    },
    [CLOCK__MAX + MOD__I2C1] = {
        .con_id = modnode[MOD__I2C1].modname,
    },
    [CLOCK__MAX + MOD__I2C0] = {
        .con_id = modnode[MOD__I2C0].modname,
    },
    [CLOCK__MAX + MOD__SPI3] = {
        .con_id = modnode[MOD__SPI3].modname,
    },
    [CLOCK__MAX + MOD__SPI2] = {
        .con_id = modnode[MOD__SPI2].modname,
    },
    [CLOCK__MAX + MOD__SPI1] = {
        .con_id = modnode[MOD__SPI1].modname,
    },
    [CLOCK__MAX + MOD__SPI0] = {
        .con_id = modnode[MOD__SPI0].modname,
    },
    [CLOCK__MAX + MOD__IRC] = {
        .con_id = modnode[MOD__IRC].modname,
    },
    [CLOCK__MAX + MOD__UART2] = {
        .con_id = modnode[MOD__UART2].modname,
    },
    [CLOCK__MAX + MOD__UART1] = {
        .con_id = modnode[MOD__UART1].modname,
    },
    [CLOCK__MAX + MOD__UART0] = {
        .con_id = modnode[MOD__UART0].modname,
    },
    [CLOCK__MAX + MOD__HDMI] = {
        .con_id = modnode[MOD__HDMI].modname,
    },
    [CLOCK__MAX + MOD__SS] = {
        .con_id = modnode[MOD__SS].modname,
    },
    [CLOCK__MAX + MOD__TV24M] = {
        .con_id = modnode[MOD__TV24M].modname,
    },
    [CLOCK__MAX + MOD__CVBS_CLK108M] = {
        .con_id = modnode[MOD__CVBS_CLK108M].modname,
    },
    [CLOCK__MAX + MOD__TVOUT] = {
        .con_id = modnode[MOD__TVOUT].modname,
    },
};

static struct clocks_table clks_table = 
{
    .clocks = clocks,
    .rvregs = rvregs,
    .pllnode = pllnode,
    .owl_clks = owl_clks,
    .modnode = modnode,
    .clk_foo_clocks = clk_foo_clocks,
    .lookup_table = lookup_table,
};

struct clocks_table * atm7059_get_clocktree(void)
{
	return &clks_table;
}

void atm7059_init_clocktree(struct device_node *cum_node)
{
	int i;
	int pll;
	int frequency = 0;

	clocks[CLOCK__L2_NIC_CLK].actdiv            = NULL;
	clocks[CLOCK__L2_NIC_CLK].divider           = 1;
	clocks[CLOCK__L2_NIC_CLK].harddivider       = 1;
	clocks[CLOCK__L2_CLK].actdiv                = NULL;
	clocks[CLOCK__L2_CLK].divider               = 1;
	clocks[CLOCK__L2_CLK].harddivider           = 1;

	clocks[CLOCK__CLK_PIXEL].actdiv             = NULL;
	clocks[CLOCK__CLK_PIXEL].divider            = 1;
	clocks[CLOCK__CLK_PIXEL].harddivider        = 1;
	clocks[CLOCK__CLK_TMDS].actdiv              = NULL;
	clocks[CLOCK__CLK_TMDS].divider             = 1;
	clocks[CLOCK__CLK_TMDS].harddivider         = 1;
	
	clocks[CLOCK__PCM0_CLK].divider             = 2;
	clocks[CLOCK__PCM1_CLK].divider             = 2;
	clocks[CLOCK__PHY_CLK].divider              = 4;
	clocks[CLOCK__SD0_CLK].divider              = 2;
	clocks[CLOCK__SD1_CLK].divider              = 2;
	clocks[CLOCK__SD2_CLK].divider              = 2;
	clocks[CLOCK__I2C0_CLK].divider             = 5;
	clocks[CLOCK__I2C1_CLK].divider             = 5;
	clocks[CLOCK__I2C2_CLK].divider             = 5;
	clocks[CLOCK__I2C3_CLK].divider             = 5;
	clocks[CLOCK__25M_CLK].divider              = 20;
	clocks[CLOCK__IRC_CLK].divider              = 120;
	clocks[CLOCK__SPS_CLK].divider              = 12;
                                    
	clocks[CLOCK__PCM0_CLK].harddivider         = 2;
	clocks[CLOCK__PCM1_CLK].harddivider         = 2;
	clocks[CLOCK__PHY_CLK].harddivider          = 4;
	clocks[CLOCK__SD0_CLK].harddivider          = 2;
	clocks[CLOCK__SD1_CLK].harddivider          = 2;
	clocks[CLOCK__SD2_CLK].harddivider          = 2;
	clocks[CLOCK__I2C0_CLK].harddivider         = 5;
	clocks[CLOCK__I2C1_CLK].harddivider         = 5;
	clocks[CLOCK__I2C2_CLK].harddivider         = 5;
	clocks[CLOCK__I2C3_CLK].harddivider         = 5;
	clocks[CLOCK__25M_CLK].harddivider          = 20;
	clocks[CLOCK__IRC_CLK].harddivider          = 120;
	clocks[CLOCK__SPS_CLK].harddivider          = 12;

	clocks[CLOCK__HOSC].harddivider             = 1;
	clocks[CLOCK__IC_32K].harddivider           = 1;
	clocks[CLOCK__DEV_CLK].harddivider          = 1;
	clocks[CLOCK__DDR_CLK_0].harddivider        = 1;
	clocks[CLOCK__DDR_CLK_90].harddivider       = 1;
	clocks[CLOCK__DDR_CLK_180].harddivider      = 1;
	clocks[CLOCK__DDR_CLK_270].harddivider      = 1;
	clocks[CLOCK__DDR_CLK_CH0].harddivider      = 1;
	clocks[CLOCK__DDR_CLK_CH1].harddivider      = 1;
	clocks[CLOCK__DDR_CLK].harddivider          = 1;
	clocks[CLOCK__CLK_TMDS_PHY_P].harddivider   = 1;
	clocks[CLOCK__AHB_CLK].harddivider          = 1;
	clocks[CLOCK__CORE_CLK].harddivider         = 1;
	clocks[CLOCK__CPU_CLK].harddivider          = 1;
	clocks[CLOCK__LVDS_CLK].harddivider         = 1;
	clocks[CLOCK__CKA_LCD_H].harddivider        = 1;
	clocks[CLOCK__ISPBP_CLK].harddivider        = 1;
	clocks[CLOCK__IMG5_CLK].harddivider         = 1;
	clocks[CLOCK__HDMI24M].harddivider          = 1;
	clocks[CLOCK__TIMER_CLK].harddivider        = 1;
	clocks[CLOCK__TV24M].harddivider            = 1;
	clocks[CLOCK__CVBS_CLK108M].harddivider            = 1;
	clocks[CLOCK__MIPI24M].harddivider          = 1;
	clocks[CLOCK__LENS24M].harddivider          = 1;
	clocks[CLOCK__DSI_HCLK90].harddivider       = 1;

	clocks[CLOCK__COREPLL].harddivider          = 1;
	clocks[CLOCK__DEVPLL].harddivider           = 1;
	clocks[CLOCK__DDRPLL].harddivider           = 1;
	clocks[CLOCK__NANDPLL].harddivider          = 1;
	clocks[CLOCK__DISPLAYPLL].harddivider       = 1;
	clocks[CLOCK__AUDIOPLL].harddivider         = 1;
	clocks[CLOCK__TVOUTPLL].harddivider         = 1;
	clocks[CLOCK__ETHERNETPLL].harddivider      = 1;
	clocks[CLOCK__CVBSPLL].harddivider      = 1;

	clocks[CLOCK__SPDIF_CLK].actdiv             = &divider_SPDIF_CLK;
	clocks[CLOCK__HDMIA_CLK].actdiv             = &divider_HDMIA_CLK;
	clocks[CLOCK__I2SRX_CLK].actdiv             = &divider_I2SRX_CLK;
	clocks[CLOCK__I2STX_CLK].actdiv             = &divider_I2STX_CLK;
	clocks[CLOCK__APBDBG_CLK].actdiv            = &divider_APBDBG_CLK;
	clocks[CLOCK__ACP_CLK].actdiv               = &divider_ACP_CLK;
	clocks[CLOCK__PERIPH_CLK].actdiv            = &divider_PERIPH_CLK;
	clocks[CLOCK__NIC_DIV_CLK].actdiv           = &divider_NIC_DIV_CLK;
	clocks[CLOCK__NIC_CLK].actdiv               = &divider_NIC_CLK;
	clocks[CLOCK__AHBPREDIV_CLK].actdiv         = &divider_AHBPREDIV_CLK;
	clocks[CLOCK__H_CLK].actdiv                 = &divider_H_CLK;
	clocks[CLOCK__APB30_CLK].actdiv             = &divider_APB30_CLK;
	clocks[CLOCK__APB20_CLK].actdiv             = &divider_APB20_CLK;
	clocks[CLOCK__SENSOR_CLKOUT0].actdiv        = &divider_SENSOR_CLKOUT0;
	clocks[CLOCK__SENSOR_CLKOUT1].actdiv        = &divider_SENSOR_CLKOUT1;
	clocks[CLOCK__LCD_CLK].actdiv               = &divider_LCD_CLK;
	clocks[CLOCK__LCD1_CLK].actdiv              = &divider_LCD1_CLK;
	clocks[CLOCK__LCD0_CLK].actdiv              = &divider_LCD0_CLK;
	clocks[CLOCK__DSI_HCLK].actdiv              = &divider_DSI_HCLK;
	clocks[CLOCK__PRO_CLK].actdiv               = &divider_PRO_CLK;
	clocks[CLOCK__CSI_CLK].actdiv               = &divider_CSI_CLK;
	clocks[CLOCK__DE1_CLK].actdiv               = &divider_DE1_CLK;
	clocks[CLOCK__DE2_CLK].actdiv               = &divider_DE2_CLK;
	clocks[CLOCK__BISP_CLK].actdiv              = &divider_BISP_CLK;
	clocks[CLOCK__VDE_CLK].actdiv               = &divider_VDE_CLK;
	clocks[CLOCK__VCE_CLK].actdiv               = &divider_VCE_CLK;
	clocks[CLOCK__NANDC_CLK].actdiv             = &divider_NANDC_CLK;
	clocks[CLOCK__ECC_CLK].actdiv               = &divider_ECC_CLK;
	clocks[CLOCK__PRESD0_CLK].actdiv            = &divider_PRESD0_CLK;
	clocks[CLOCK__PRESD1_CLK].actdiv            = &divider_PRESD1_CLK;
	clocks[CLOCK__PRESD2_CLK].actdiv            = &divider_PRESD2_CLK;
	clocks[CLOCK__SD0_CLK_2X].actdiv            = &divider_SD0_CLK_2X;
	clocks[CLOCK__SD1_CLK_2X].actdiv            = &divider_SD1_CLK_2X;
	clocks[CLOCK__SD2_CLK_2X].actdiv            = &divider_SD2_CLK_2X;
	clocks[CLOCK__UART0_CLK].actdiv             = &divider_UART0_CLK;
	clocks[CLOCK__UART1_CLK].actdiv             = &divider_UART1_CLK;
	clocks[CLOCK__UART2_CLK].actdiv             = &divider_UART2_CLK;
	clocks[CLOCK__UART3_CLK].actdiv             = &divider_UART3_CLK;
	clocks[CLOCK__UART4_CLK].actdiv             = &divider_UART4_CLK;
	clocks[CLOCK__UART5_CLK].actdiv             = &divider_UART5_CLK;
	clocks[CLOCK__UART6_CLK].actdiv             = &divider_UART6_CLK;
	clocks[CLOCK__PWM0_CLK].actdiv              = &divider_PWM0_CLK;
	clocks[CLOCK__PWM1_CLK].actdiv              = &divider_PWM1_CLK;
	clocks[CLOCK__PWM2_CLK].actdiv              = &divider_PWM2_CLK;
	clocks[CLOCK__PWM3_CLK].actdiv              = &divider_PWM3_CLK;
	clocks[CLOCK__PWM4_CLK].actdiv              = &divider_PWM4_CLK;
	clocks[CLOCK__PWM5_CLK].actdiv              = &divider_PWM5_CLK;
	clocks[CLOCK__RMII_REF_CLK].actdiv          = &divider_RMII_REF_CLK;
	clocks[CLOCK__LENS_CLK].actdiv              = &divider_LENS_CLK;
	clocks[CLOCK__GPU3D_SYSCLK].actdiv          = &divider_GPU3D_SYSCLK;
	clocks[CLOCK__GPU3D_HYDCLK].actdiv          = &divider_GPU3D_HYDCLK;
	clocks[CLOCK__GPU3D_NIC_MEMCLK].actdiv      = &divider_GPU3D_NIC_MEMCLK;
	clocks[CLOCK__GPU3D_CORECLK].actdiv         = &divider_GPU3D_CORECLK;
	clocks[CLOCK__SS_CLK].actdiv                = &divider_SS_CLK;

	for (i = 0; i < CLOCK__MAX; i++) {
		/*
		 * read pll frequency from cmu regs
		 * TVOUTPLL must read before DEEPCOLORPLL
		 */
		if (clocks[i].type == TYPE_PLL) {
			pll = i - CLOCK__COREPLL;
			if (pllnode[pll].reg_pllfreq) {
				pllnode[pll].sel = read_clkreg_val(pllnode[pll].reg_pllfreq);
			} else
				pllnode[pll].sel = 0;

			switch (pllnode[pll].type) {
			case PLL_T_D4DYN:
				pllnode[pll].sel |= 4;
			case PLL_T_STEP:
				frequency = (pllnode[pll].sel+pllnode[pll].freq.step.offset) * pllnode[pll].freq.step.step;
				break;
			case PLL_T_FREQ:
				frequency = pllnode[pll].freq.freqtab[pllnode[pll].sel];
				break;
			default:
				break;
			}
			if (pll == PLL__TVOUTPLL && frequency > 4) {
				pllnode[PLL__DEEPCOLORPLL].freq.step.step = frequency / 4;
			}
			clocks[i].frequency = frequency;
		}

		/*
		 * read clock divider from cmu regs
		 */
		write_clkreg_val(&busbit_DIVEN, 0);
		if (clocks[i].actdiv) {
			clocks[i].divsel = read_clkreg_val(clocks[i].actdiv->reg);
			clocks[i].divider = getdivider(clocks[i].actdiv, clocks[i].divsel);
			if (i == CLOCK__PRESD1_CLK)
				clocks[i].divider = clocks[CLOCK__PRESD0_CLK].divider;

			if (i == CLOCK__UART0_CLK) {
				clocks[i].divsel = read_clkreg_val(clocks[CLOCK__UART2_CLK].actdiv->reg);
				clocks[i].divider = getdivider(clocks[i].actdiv, clocks[i].divsel);
			}

			if (i == CLOCK__UART3_CLK)
				clocks[i].divider = clocks[CLOCK__UART2_CLK].divider;

			if (clocks[i].divider < 0) {
				clocks[i].divsel = getdivider_resetval(clocks[i].actdiv);
				write_clkreg_val(clocks[i].actdiv->reg, clocks[i].divsel);
				clocks[i].divider = getdivider(clocks[i].actdiv, clocks[i].divsel);
			}

			if (i == CLOCK__PRO_CLK && (clocks[i].divider & 0xffff0000)) {
				clocks[i].multipler = 3;
				clocks[i].divider = 4;
			}
		}
		write_clkreg_val(&busbit_DIVEN, 1);

		/*
		 * read clock dependence legacy
		 */
		if (clocks[i].reg_srcsel) {
			clocks[i].source_sel = read_clkreg_val(clocks[i].reg_srcsel);
			/* of cause source_sel must < source_lim */
			if (clocks[i].source_sel >= clocks[i].source_lim) {
				printk("error: clock %s parent index error\n",
					clocks[i].name);
				clocks[i].source_sel = 0;
			}
		}

		addclock(i);
		if (clocks[i].type == TYPE_DYNAMIC) {
			clocks[i].changed = 1;
		}
	}

	owl_clks[CLOCK__HOSC] = owl_clk_register_fixed_rate_with_accuracy(NULL, clocks[CLOCK__HOSC].name, NULL, 0,
							&clk_ops_gate_hosc, FREQUENCY_24M, 0, CLOCK__HOSC);
	owl_clks[CLOCK__IC_32K] = owl_clk_register_fixed_rate_with_accuracy(NULL, clocks[CLOCK__IC_32K].name, NULL, 0,
							&clk_ops_gate_ic32k, FREQUENCY_32K, 0, CLOCK__IC_32K);
	owl_clks[CLOCK__COREPLL] = owl_clk_register_gate(NULL, clocks[CLOCK__COREPLL].name, parent_names_COREPLL[0], 0,
							&clk_ops_corepll, NULL, 0, 0, NULL, CLOCK__COREPLL);
	owl_clks[CLOCK__DEVPLL] = owl_clk_register_gate(NULL, clocks[CLOCK__DEVPLL].name, parent_names_DEVPLL[0], 0,
							&clk_ops_pll, NULL, 0, 0, NULL, CLOCK__DEVPLL);
	owl_clks[CLOCK__DDRPLL] = owl_clk_register_gate(NULL, clocks[CLOCK__DDRPLL].name, parent_names_DDRPLL[0], 0,
							&clk_ops_pll, NULL, 0, 0, NULL, CLOCK__DDRPLL);
	owl_clks[CLOCK__NANDPLL] = owl_clk_register_gate(NULL, clocks[CLOCK__NANDPLL].name, parent_names_NANDPLL[0], 0,
							&clk_ops_pll, NULL, 0, 0, NULL, CLOCK__NANDPLL);
	owl_clks[CLOCK__DISPLAYPLL] = owl_clk_register_gate(NULL, clocks[CLOCK__DISPLAYPLL].name, parent_names_DISPLAYPLL[0], 0,
							&clk_ops_pll, NULL, 0, 0, NULL, CLOCK__DISPLAYPLL);
	owl_clks[CLOCK__AUDIOPLL] = owl_clk_register_gate(NULL, clocks[CLOCK__AUDIOPLL].name, parent_names_AUDIOPLL[0], 0,
							&clk_ops_pll, NULL, 0, 0, NULL, CLOCK__AUDIOPLL);
	owl_clks[CLOCK__TVOUTPLL] = owl_clk_register_gate(NULL, clocks[CLOCK__TVOUTPLL].name, parent_names_TVOUTPLL[0], 0,
							&clk_ops_pll, NULL, 0, 0, NULL, CLOCK__TVOUTPLL);
	owl_clks[CLOCK__ETHERNETPLL] = owl_clk_register_gate(NULL, clocks[CLOCK__ETHERNETPLL].name, parent_names_ETHERNETPLL[0], 0,
							&clk_ops_pll, NULL, 0, 0, NULL, CLOCK__ETHERNETPLL);
	owl_clks[CLOCK__CVBSPLL] = owl_clk_register_gate(NULL, clocks[CLOCK__CVBSPLL].name, parent_names_CVBSPLL[0], 0,
							&clk_ops_pll, NULL, 0, 0, NULL, CLOCK__CVBSPLL);
	owl_clks[CLOCK__DEV_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__DEV_CLK].name, parent_names_DEV_CLK, 2, 0,
							&clk_ops_direct_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__DEV_CLK);
	owl_clks[CLOCK__DDR_CLK_0] = owl_clk_register_gate(NULL, clocks[CLOCK__DDR_CLK_0].name, parent_names_DDR_CLK_0[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__DDR_CLK_0);
	owl_clks[CLOCK__DDR_CLK_90] = owl_clk_register_gate(NULL, clocks[CLOCK__DDR_CLK_90].name, parent_names_DDR_CLK_90[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__DDR_CLK_90);
	owl_clks[CLOCK__DDR_CLK_180] = owl_clk_register_gate(NULL, clocks[CLOCK__DDR_CLK_180].name, parent_names_DDR_CLK_180[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__DDR_CLK_180);
	owl_clks[CLOCK__DDR_CLK_270] = owl_clk_register_gate(NULL, clocks[CLOCK__DDR_CLK_270].name, parent_names_DDR_CLK_270[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__DDR_CLK_270);
	owl_clks[CLOCK__DDR_CLK_CH0] = owl_clk_register_gate(NULL, clocks[CLOCK__DDR_CLK_CH0].name, parent_names_DDR_CLK_CH0[0], CLK_SET_RATE_PARENT,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__DDR_CLK_CH0);
	owl_clks[CLOCK__DDR_CLK_CH1] = owl_clk_register_mux_table(NULL, clocks[CLOCK__DDR_CLK_CH1].name, parent_names_DDR_CLK_CH1, 4, CLK_SET_RATE_PARENT,
							&clk_ops_direct_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__DDR_CLK_CH1);
	owl_clks[CLOCK__DDR_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__DDR_CLK].name, parent_names_DDR_CLK[0], CLK_SET_RATE_PARENT,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__DDR_CLK);
	owl_clks[CLOCK__SPDIF_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__SPDIF_CLK].name, parent_names_SPDIF_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__SPDIF_CLK);
	owl_clks[CLOCK__HDMIA_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__HDMIA_CLK].name, parent_names_HDMIA_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__HDMIA_CLK);
	owl_clks[CLOCK__I2SRX_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__I2SRX_CLK].name, parent_names_I2SRX_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__I2SRX_CLK);
	owl_clks[CLOCK__I2STX_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__I2STX_CLK].name, parent_names_I2STX_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__I2STX_CLK);
	owl_clks[CLOCK__PCM0_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__PCM0_CLK].name, parent_names_PCM0_CLK[0], 0,
							&clk_ops_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__PCM0_CLK);
	owl_clks[CLOCK__PCM1_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__PCM1_CLK].name, parent_names_PCM1_CLK[0], 0,
							&clk_ops_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__PCM1_CLK);
	owl_clks[CLOCK__CLK_PIXEL] = owl_clk_register_gate(NULL, clocks[CLOCK__CLK_PIXEL].name, parent_names_CLK_PIXEL[0], CLK_GET_RATE_NOCACHE,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__CLK_PIXEL);
	owl_clks[CLOCK__CLK_TMDS] = owl_clk_register_gate(NULL, clocks[CLOCK__CLK_TMDS].name, parent_names_CLK_TMDS[0], CLK_GET_RATE_NOCACHE,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__CLK_TMDS);
	owl_clks[CLOCK__CLK_TMDS_PHY_P] = owl_clk_register_gate(NULL, clocks[CLOCK__CLK_TMDS_PHY_P].name, parent_names_CLK_TMDS_PHY_P[0], CLK_SET_RATE_PARENT,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__CLK_TMDS_PHY_P);
	owl_clks[CLOCK__CORE_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__CORE_CLK].name, parent_names_CORE_CLK, 4, 0,
							&clk_ops_direct_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__CORE_CLK);
	owl_clks[CLOCK__L2_NIC_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__L2_NIC_CLK].name, parent_names_L2_NIC_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__L2_NIC_CLK);
	owl_clks[CLOCK__CPU_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__CPU_CLK].name, parent_names_CPU_CLK[0], CLK_SET_RATE_PARENT,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__CPU_CLK);
	owl_clks[CLOCK__APBDBG_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__APBDBG_CLK].name, parent_names_APBDBG_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__APBDBG_CLK);
	owl_clks[CLOCK__L2_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__L2_CLK].name, parent_names_L2_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__L2_CLK);
	owl_clks[CLOCK__ACP_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__ACP_CLK].name, parent_names_ACP_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__ACP_CLK);
	owl_clks[CLOCK__PERIPH_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__PERIPH_CLK].name, parent_names_PERIPH_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__PERIPH_CLK);
	owl_clks[CLOCK__NIC_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__NIC_CLK].name, parent_names_NIC_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__NIC_CLK);
	owl_clks[CLOCK__NIC_DIV_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__NIC_DIV_CLK].name, parent_names_NIC_DIV_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__NIC_DIV_CLK);
	owl_clks[CLOCK__AHBPREDIV_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__AHBPREDIV_CLK].name, parent_names_AHBPREDIV_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__AHBPREDIV_CLK);
	owl_clks[CLOCK__H_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__H_CLK].name, parent_names_H_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__H_CLK);
	owl_clks[CLOCK__APB30_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__APB30_CLK].name, parent_names_APB30_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__APB30_CLK);
	owl_clks[CLOCK__APB20_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__APB20_CLK].name, parent_names_APB20_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__APB20_CLK);
	owl_clks[CLOCK__AHB_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__AHB_CLK].name, parent_names_AHB_CLK[0], CLK_SET_RATE_PARENT,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__AHB_CLK);
	owl_clks[CLOCK__SENSOR_CLKOUT0] = owl_clk_register_mux_table(NULL, clocks[CLOCK__SENSOR_CLKOUT0].name, parent_names_SENSOR_CLKOUT0, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__SENSOR_CLKOUT0);
	owl_clks[CLOCK__SENSOR_CLKOUT1] = owl_clk_register_mux_table(NULL, clocks[CLOCK__SENSOR_CLKOUT1].name, parent_names_SENSOR_CLKOUT1, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__SENSOR_CLKOUT1);
	owl_clks[CLOCK__LCD_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__LCD_CLK].name, parent_names_LCD_CLK, 3, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__LCD_CLK);
	owl_clks[CLOCK__LVDS_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__LVDS_CLK].name, parent_names_LVDS_CLK[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__LVDS_CLK);
	owl_clks[CLOCK__CKA_LCD_H] = owl_clk_register_gate(NULL, clocks[CLOCK__CKA_LCD_H].name, parent_names_CKA_LCD_H[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__CKA_LCD_H);
	owl_clks[CLOCK__LCD1_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__LCD1_CLK].name, parent_names_LCD1_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__LCD1_CLK);
	owl_clks[CLOCK__LCD0_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__LCD0_CLK].name, parent_names_LCD0_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__LCD0_CLK);
	owl_clks[CLOCK__DSI_HCLK] = owl_clk_register_gate(NULL, clocks[CLOCK__DSI_HCLK].name, parent_names_DSI_HCLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__DSI_HCLK);
	owl_clks[CLOCK__DSI_HCLK90] = owl_clk_register_gate(NULL, clocks[CLOCK__DSI_HCLK90].name, parent_names_DSI_HCLK90[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__DSI_HCLK90);
	owl_clks[CLOCK__PRO_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__PRO_CLK].name, parent_names_PRO_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__PRO_CLK);
	owl_clks[CLOCK__PHY_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__PHY_CLK].name, parent_names_PHY_CLK[0], 0,
							&clk_ops_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__PHY_CLK);
	owl_clks[CLOCK__CSI_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__CSI_CLK].name, parent_names_CSI_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__CSI_CLK);
	owl_clks[CLOCK__DE1_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__DE1_CLK].name, parent_names_DE1_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__DE1_CLK);
	owl_clks[CLOCK__DE2_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__DE2_CLK].name, parent_names_DE2_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__DE2_CLK);
	owl_clks[CLOCK__BISP_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__BISP_CLK].name, parent_names_BISP_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__BISP_CLK);
	owl_clks[CLOCK__ISPBP_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__ISPBP_CLK].name, parent_names_ISPBP_CLK[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__ISPBP_CLK);
	owl_clks[CLOCK__IMG5_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__IMG5_CLK].name, parent_names_IMG5_CLK, 2, CLK_SET_RATE_PARENT,
							&clk_ops_direct_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__IMG5_CLK);
	owl_clks[CLOCK__VDE_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__VDE_CLK].name, parent_names_VDE_CLK, 4, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__VDE_CLK);
	owl_clks[CLOCK__VCE_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__VCE_CLK].name, parent_names_VCE_CLK, 4, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__VCE_CLK);
	owl_clks[CLOCK__NANDC_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__NANDC_CLK].name, parent_names_NANDC_CLK, 4, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__NANDC_CLK);
	owl_clks[CLOCK__ECC_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__ECC_CLK].name, parent_names_ECC_CLK, 4, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__ECC_CLK);
	owl_clks[CLOCK__PRESD0_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__PRESD0_CLK].name, parent_names_PRESD0_CLK, 2, 0,
							&clk_ops_h_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__PRESD0_CLK);
	owl_clks[CLOCK__PRESD1_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__PRESD1_CLK].name, parent_names_PRESD1_CLK, 2, 0,
							&clk_ops_h_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__PRESD1_CLK);
	owl_clks[CLOCK__PRESD2_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__PRESD2_CLK].name, parent_names_PRESD2_CLK, 2, 0,
							&clk_ops_h_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__PRESD2_CLK);
	owl_clks[CLOCK__SD0_CLK_2X] = owl_clk_register_gate(NULL, clocks[CLOCK__SD0_CLK_2X].name, parent_names_SD0_CLK_2X[0], CLK_SET_RATE_PARENT,
							&clk_ops_b_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__SD0_CLK_2X);
	owl_clks[CLOCK__SD1_CLK_2X] = owl_clk_register_gate(NULL, clocks[CLOCK__SD1_CLK_2X].name, parent_names_SD1_CLK_2X[0], CLK_SET_RATE_PARENT,
							&clk_ops_b_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__SD1_CLK_2X);
	owl_clks[CLOCK__SD2_CLK_2X] = owl_clk_register_gate(NULL, clocks[CLOCK__SD2_CLK_2X].name, parent_names_SD2_CLK_2X[0], CLK_SET_RATE_PARENT,
							&clk_ops_b_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__SD2_CLK_2X);
	owl_clks[CLOCK__SD0_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__SD0_CLK].name, parent_names_SD0_CLK[0], CLK_SET_RATE_PARENT,
							&clk_ops_b_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__SD0_CLK);
	owl_clks[CLOCK__SD1_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__SD1_CLK].name, parent_names_SD1_CLK[0], CLK_SET_RATE_PARENT,
							&clk_ops_b_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__SD1_CLK);
	owl_clks[CLOCK__SD2_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__SD2_CLK].name, parent_names_SD2_CLK[0], CLK_SET_RATE_PARENT,
							&clk_ops_b_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__SD2_CLK);
	owl_clks[CLOCK__UART0_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__UART0_CLK].name, parent_names_UART0_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__UART0_CLK);
	owl_clks[CLOCK__UART1_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__UART1_CLK].name, parent_names_UART1_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__UART1_CLK);
	owl_clks[CLOCK__UART2_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__UART2_CLK].name, parent_names_UART2_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__UART2_CLK);
	owl_clks[CLOCK__UART3_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__UART3_CLK].name, parent_names_UART3_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__UART3_CLK);
	owl_clks[CLOCK__UART4_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__UART4_CLK].name, parent_names_UART4_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__UART4_CLK);
	owl_clks[CLOCK__UART5_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__UART5_CLK].name, parent_names_UART5_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__UART5_CLK);
	owl_clks[CLOCK__UART6_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__UART6_CLK].name, parent_names_UART6_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__UART6_CLK);
	owl_clks[CLOCK__PWM0_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__PWM0_CLK].name, parent_names_PWM0_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__PWM0_CLK);
	owl_clks[CLOCK__PWM1_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__PWM1_CLK].name, parent_names_PWM1_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__PWM1_CLK);
	owl_clks[CLOCK__PWM2_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__PWM2_CLK].name, parent_names_PWM2_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__PWM2_CLK);
	owl_clks[CLOCK__PWM3_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__PWM3_CLK].name, parent_names_PWM3_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__PWM3_CLK);
	owl_clks[CLOCK__PWM4_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__PWM4_CLK].name, parent_names_PWM4_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__PWM4_CLK);
	owl_clks[CLOCK__PWM5_CLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__PWM5_CLK].name, parent_names_PWM5_CLK, 2, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__PWM5_CLK);
	owl_clks[CLOCK__RMII_REF_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__RMII_REF_CLK].name, parent_names_RMII_REF_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__RMII_REF_CLK);
	owl_clks[CLOCK__I2C0_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__I2C0_CLK].name, parent_names_I2C0_CLK[0], 0,
							&clk_ops_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__I2C0_CLK);
	owl_clks[CLOCK__I2C1_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__I2C1_CLK].name, parent_names_I2C1_CLK[0], 0,
							&clk_ops_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__I2C1_CLK);
	owl_clks[CLOCK__I2C2_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__I2C2_CLK].name, parent_names_I2C2_CLK[0], 0,
							&clk_ops_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__I2C2_CLK);
	owl_clks[CLOCK__I2C3_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__I2C3_CLK].name, parent_names_I2C3_CLK[0], 0,
							&clk_ops_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__I2C3_CLK);
	owl_clks[CLOCK__25M_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__25M_CLK].name, parent_names_25M_CLK[0], 0,
							&clk_ops_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__25M_CLK);
	owl_clks[CLOCK__LENS_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__LENS_CLK].name, parent_names_LENS_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__LENS_CLK);
	owl_clks[CLOCK__HDMI24M] = owl_clk_register_gate(NULL, clocks[CLOCK__HDMI24M].name, parent_names_HDMI24M[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__HDMI24M);
	owl_clks[CLOCK__TIMER_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__TIMER_CLK].name, parent_names_TIMER_CLK[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__TIMER_CLK);
	owl_clks[CLOCK__SS_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__SS_CLK].name, parent_names_SS_CLK[0], 0,
							&clk_ops_m_divider_s_parent, NULL, 0, 0, NULL, CLOCK__SS_CLK);
	owl_clks[CLOCK__SPS_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__SPS_CLK].name, parent_names_SPS_CLK[0], 0,
							&clk_ops_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__SPS_CLK);
	owl_clks[CLOCK__IRC_CLK] = owl_clk_register_gate(NULL, clocks[CLOCK__IRC_CLK].name, parent_names_IRC_CLK[0], 0,
							&clk_ops_s_divider_s_parent, NULL, 0, 0, NULL, CLOCK__IRC_CLK);
	owl_clks[CLOCK__TV24M] = owl_clk_register_gate(NULL, clocks[CLOCK__TV24M].name, parent_names_TV24M[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__TV24M);
	owl_clks[CLOCK__CVBS_CLK108M] = owl_clk_register_gate(NULL, clocks[CLOCK__CVBS_CLK108M].name, parent_names_CVBS_CLK108M[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__CVBS_CLK108M);
	owl_clks[CLOCK__MIPI24M] = owl_clk_register_gate(NULL, clocks[CLOCK__MIPI24M].name, parent_names_MIPI24M[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__MIPI24M);
	owl_clks[CLOCK__LENS24M] = owl_clk_register_gate(NULL, clocks[CLOCK__LENS24M].name, parent_names_LENS24M[0], 0,
							&clk_ops_direct_s_parent, NULL, 0, 0, NULL, CLOCK__LENS24M);
	owl_clks[CLOCK__GPU3D_SYSCLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__GPU3D_SYSCLK].name, parent_names_GPU3D_SYSCLK, 4, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__GPU3D_SYSCLK);
	owl_clks[CLOCK__GPU3D_HYDCLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__GPU3D_HYDCLK].name, parent_names_GPU3D_HYDCLK, 4, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__GPU3D_HYDCLK);
	owl_clks[CLOCK__GPU3D_NIC_MEMCLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__GPU3D_NIC_MEMCLK].name, parent_names_GPU3D_NIC_MEMCLK, 4, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__GPU3D_NIC_MEMCLK);
	owl_clks[CLOCK__GPU3D_CORECLK] = owl_clk_register_mux_table(NULL, clocks[CLOCK__GPU3D_CORECLK].name, parent_names_GPU3D_CORECLK, 4, 0,
							&clk_ops_m_divider_m_parent, NULL, 0, 0, 0, NULL, NULL, CLOCK__GPU3D_CORECLK);
	owl_clks[CLOCK__MAX + MOD__ROOT] = owl_clk_register_gate(NULL, modnode[MOD__ROOT].modname, NULL, 0,
							&clk_ops_foo, NULL, 0, 0, NULL, MOD__ROOT);
	owl_clks[CLOCK__MAX + MOD__GPU3D] = owl_clk_register_gate(NULL, modnode[MOD__GPU3D].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__GPU3D);
	owl_clks[CLOCK__MAX + MOD__SHARESRAM] = owl_clk_register_gate(NULL, modnode[MOD__SHARESRAM].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__SHARESRAM);
	owl_clks[CLOCK__MAX + MOD__HDCP2X] = owl_clk_register_gate(NULL, modnode[MOD__HDCP2X].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__HDCP2X);
	owl_clks[CLOCK__MAX + MOD__VCE] = owl_clk_register_gate(NULL, modnode[MOD__VCE].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__VCE);
	owl_clks[CLOCK__MAX + MOD__VDE] = owl_clk_register_gate(NULL, modnode[MOD__VDE].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__VDE);
	owl_clks[CLOCK__MAX + MOD__PCM0] = owl_clk_register_gate(NULL, modnode[MOD__PCM0].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__PCM0);
	owl_clks[CLOCK__MAX + MOD__SPDIF] = owl_clk_register_gate(NULL, modnode[MOD__SPDIF].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__SPDIF);
	owl_clks[CLOCK__MAX + MOD__HDMIA] = owl_clk_register_gate(NULL, modnode[MOD__HDMIA].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__HDMIA);
	owl_clks[CLOCK__MAX + MOD__I2SRX] = owl_clk_register_gate(NULL, modnode[MOD__I2SRX].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__I2SRX);
	owl_clks[CLOCK__MAX + MOD__I2STX] = owl_clk_register_gate(NULL, modnode[MOD__I2STX].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__I2STX);
	owl_clks[CLOCK__MAX + MOD__GPIO] = owl_clk_register_gate(NULL, modnode[MOD__GPIO].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__GPIO);
	owl_clks[CLOCK__MAX + MOD__KEY] = owl_clk_register_gate(NULL, modnode[MOD__KEY].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__KEY);
	owl_clks[CLOCK__MAX + MOD__LENS] = owl_clk_register_gate(NULL, modnode[MOD__LENS].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__LENS);
	owl_clks[CLOCK__MAX + MOD__BISP] = owl_clk_register_gate(NULL, modnode[MOD__BISP].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__BISP);
	owl_clks[CLOCK__MAX + MOD__CSI] = owl_clk_register_gate(NULL, modnode[MOD__CSI].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__CSI);
	owl_clks[CLOCK__MAX + MOD__DSI] = owl_clk_register_gate(NULL, modnode[MOD__DSI].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__DSI);
	owl_clks[CLOCK__MAX + MOD__LVDS] = owl_clk_register_gate(NULL, modnode[MOD__LVDS].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__LVDS);
	owl_clks[CLOCK__MAX + MOD__LCD1] = owl_clk_register_gate(NULL, modnode[MOD__LCD1].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__LCD1);
	owl_clks[CLOCK__MAX + MOD__LCD0] = owl_clk_register_gate(NULL, modnode[MOD__LCD0].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__LCD0);
	owl_clks[CLOCK__MAX + MOD__DE] = owl_clk_register_gate(NULL, modnode[MOD__DE].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__DE);
	owl_clks[CLOCK__MAX + MOD__SD2] = owl_clk_register_gate(NULL, modnode[MOD__SD2].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__SD2);
	owl_clks[CLOCK__MAX + MOD__SD1] = owl_clk_register_gate(NULL, modnode[MOD__SD1].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__SD1);
	owl_clks[CLOCK__MAX + MOD__SD0] = owl_clk_register_gate(NULL, modnode[MOD__SD0].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__SD0);
	owl_clks[CLOCK__MAX + MOD__NANDC] = owl_clk_register_gate(NULL, modnode[MOD__NANDC].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__NANDC);
	owl_clks[CLOCK__MAX + MOD__DDRCH0] = owl_clk_register_gate(NULL, modnode[MOD__DDRCH0].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__DDRCH0);
	owl_clks[CLOCK__MAX + MOD__NOR] = owl_clk_register_gate(NULL, modnode[MOD__NOR].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__NOR);
	owl_clks[CLOCK__MAX + MOD__DMAC] = owl_clk_register_gate(NULL, modnode[MOD__DMAC].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__DMAC);
	owl_clks[CLOCK__MAX + MOD__DDRCH1] = owl_clk_register_gate(NULL, modnode[MOD__DDRCH1].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__DDRCH1);
	owl_clks[CLOCK__MAX + MOD__I2C3] = owl_clk_register_gate(NULL, modnode[MOD__I2C3].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__I2C3);
	owl_clks[CLOCK__MAX + MOD__I2C2] = owl_clk_register_gate(NULL, modnode[MOD__I2C2].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__I2C2);
	owl_clks[CLOCK__MAX + MOD__TIMER] = owl_clk_register_gate(NULL, modnode[MOD__TIMER].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__TIMER);
	owl_clks[CLOCK__MAX + MOD__PWM5] = owl_clk_register_gate(NULL, modnode[MOD__PWM5].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__PWM5);
	owl_clks[CLOCK__MAX + MOD__PWM4] = owl_clk_register_gate(NULL, modnode[MOD__PWM4].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__PWM4);
	owl_clks[CLOCK__MAX + MOD__PWM3] = owl_clk_register_gate(NULL, modnode[MOD__PWM3].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__PWM3);
	owl_clks[CLOCK__MAX + MOD__PWM2] = owl_clk_register_gate(NULL, modnode[MOD__PWM2].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__PWM2);
	owl_clks[CLOCK__MAX + MOD__PWM1] = owl_clk_register_gate(NULL, modnode[MOD__PWM1].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__PWM1);
	owl_clks[CLOCK__MAX + MOD__PWM0] = owl_clk_register_gate(NULL, modnode[MOD__PWM0].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__PWM0);
	owl_clks[CLOCK__MAX + MOD__ETHERNET] = owl_clk_register_gate(NULL, modnode[MOD__ETHERNET].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__ETHERNET);
	owl_clks[CLOCK__MAX + MOD__UART5] = owl_clk_register_gate(NULL, modnode[MOD__UART5].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__UART5);
	owl_clks[CLOCK__MAX + MOD__UART4] = owl_clk_register_gate(NULL, modnode[MOD__UART4].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__UART4);
	owl_clks[CLOCK__MAX + MOD__UART3] = owl_clk_register_gate(NULL, modnode[MOD__UART3].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__UART3);
	owl_clks[CLOCK__MAX + MOD__UART6] = owl_clk_register_gate(NULL, modnode[MOD__UART6].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__UART6);
	owl_clks[CLOCK__MAX + MOD__PCM1] = owl_clk_register_gate(NULL, modnode[MOD__PCM1].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__PCM1);
	owl_clks[CLOCK__MAX + MOD__I2C1] = owl_clk_register_gate(NULL, modnode[MOD__I2C1].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__I2C1);
	owl_clks[CLOCK__MAX + MOD__I2C0] = owl_clk_register_gate(NULL, modnode[MOD__I2C0].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__I2C0);
	owl_clks[CLOCK__MAX + MOD__SPI3] = owl_clk_register_gate(NULL, modnode[MOD__SPI3].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__SPI3);
	owl_clks[CLOCK__MAX + MOD__SPI2] = owl_clk_register_gate(NULL, modnode[MOD__SPI2].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__SPI2);
	owl_clks[CLOCK__MAX + MOD__SPI1] = owl_clk_register_gate(NULL, modnode[MOD__SPI1].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__SPI1);
	owl_clks[CLOCK__MAX + MOD__SPI0] = owl_clk_register_gate(NULL, modnode[MOD__SPI0].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__SPI0);
	owl_clks[CLOCK__MAX + MOD__IRC] = owl_clk_register_gate(NULL, modnode[MOD__IRC].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__IRC);
	owl_clks[CLOCK__MAX + MOD__UART2] = owl_clk_register_gate(NULL, modnode[MOD__UART2].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__UART2);
	owl_clks[CLOCK__MAX + MOD__UART1] = owl_clk_register_gate(NULL, modnode[MOD__UART1].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__UART1);
	owl_clks[CLOCK__MAX + MOD__UART0] = owl_clk_register_gate(NULL, modnode[MOD__UART0].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__UART0);
	owl_clks[CLOCK__MAX + MOD__HDMI] = owl_clk_register_gate(NULL, modnode[MOD__HDMI].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__HDMI);
	owl_clks[CLOCK__MAX + MOD__SS] = owl_clk_register_gate(NULL, modnode[MOD__SS].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__SS);
	owl_clks[CLOCK__MAX + MOD__TV24M] = owl_clk_register_gate(NULL, modnode[MOD__TV24M].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__TV24M);
	owl_clks[CLOCK__MAX + MOD__CVBS_CLK108M] = owl_clk_register_gate(NULL, modnode[MOD__CVBS_CLK108M].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__CVBS_CLK108M);
	owl_clks[CLOCK__MAX + MOD__TVOUT] = owl_clk_register_gate(NULL, modnode[MOD__TVOUT].modname, parent_name_CMUMOD_DEVCLKS[0], 0,
							&clk_ops_gate_module, NULL, 0, 0, NULL, MOD__TVOUT);

	clk_data.clks = owl_clks;
	clk_data.clk_num = ARRAY_SIZE(owl_clks);
	of_clk_add_provider(cum_node, of_clk_src_onecell_get, &clk_data);

	clk_foo_clocks[CLOCK__HOSC].hw.clk = owl_clks[CLOCK__HOSC];
	clk_foo_clocks[CLOCK__IC_32K].hw.clk = owl_clks[CLOCK__IC_32K];
	clk_foo_clocks[CLOCK__COREPLL].hw.clk = owl_clks[CLOCK__COREPLL];
	clk_foo_clocks[CLOCK__DEVPLL].hw.clk = owl_clks[CLOCK__DEVPLL];
	clk_foo_clocks[CLOCK__DDRPLL].hw.clk = owl_clks[CLOCK__DDRPLL];
	clk_foo_clocks[CLOCK__NANDPLL].hw.clk = owl_clks[CLOCK__NANDPLL];
	clk_foo_clocks[CLOCK__DISPLAYPLL].hw.clk = owl_clks[CLOCK__DISPLAYPLL];
	clk_foo_clocks[CLOCK__AUDIOPLL].hw.clk = owl_clks[CLOCK__AUDIOPLL];
	clk_foo_clocks[CLOCK__TVOUTPLL].hw.clk = owl_clks[CLOCK__TVOUTPLL];
	clk_foo_clocks[CLOCK__ETHERNETPLL].hw.clk = owl_clks[CLOCK__ETHERNETPLL];
	clk_foo_clocks[CLOCK__CVBSPLL].hw.clk = owl_clks[CLOCK__CVBSPLL];
	clk_foo_clocks[CLOCK__DEV_CLK].hw.clk = owl_clks[CLOCK__DEV_CLK];
	clk_foo_clocks[CLOCK__DDR_CLK_0].hw.clk = owl_clks[CLOCK__DDR_CLK_0];
	clk_foo_clocks[CLOCK__DDR_CLK_90].hw.clk = owl_clks[CLOCK__DDR_CLK_90];
	clk_foo_clocks[CLOCK__DDR_CLK_180].hw.clk = owl_clks[CLOCK__DDR_CLK_180];
	clk_foo_clocks[CLOCK__DDR_CLK_270].hw.clk = owl_clks[CLOCK__DDR_CLK_270];
	clk_foo_clocks[CLOCK__DDR_CLK_CH0].hw.clk = owl_clks[CLOCK__DDR_CLK_CH0];
	clk_foo_clocks[CLOCK__DDR_CLK_CH1].hw.clk = owl_clks[CLOCK__DDR_CLK_CH1];
	clk_foo_clocks[CLOCK__DDR_CLK].hw.clk = owl_clks[CLOCK__DDR_CLK];
	clk_foo_clocks[CLOCK__SPDIF_CLK].hw.clk = owl_clks[CLOCK__SPDIF_CLK];
	clk_foo_clocks[CLOCK__HDMIA_CLK].hw.clk = owl_clks[CLOCK__HDMIA_CLK];
	clk_foo_clocks[CLOCK__I2SRX_CLK].hw.clk = owl_clks[CLOCK__I2SRX_CLK];
	clk_foo_clocks[CLOCK__I2STX_CLK].hw.clk = owl_clks[CLOCK__I2STX_CLK];
	clk_foo_clocks[CLOCK__PCM0_CLK].hw.clk = owl_clks[CLOCK__PCM0_CLK];
	clk_foo_clocks[CLOCK__PCM1_CLK].hw.clk = owl_clks[CLOCK__PCM1_CLK];
	clk_foo_clocks[CLOCK__CLK_PIXEL].hw.clk = owl_clks[CLOCK__CLK_PIXEL];
	clk_foo_clocks[CLOCK__CLK_TMDS].hw.clk = owl_clks[CLOCK__CLK_TMDS];
	clk_foo_clocks[CLOCK__CLK_TMDS_PHY_P].hw.clk = owl_clks[CLOCK__CLK_TMDS_PHY_P];
	clk_foo_clocks[CLOCK__L2_NIC_CLK].hw.clk = owl_clks[CLOCK__L2_NIC_CLK];
	clk_foo_clocks[CLOCK__APBDBG_CLK].hw.clk = owl_clks[CLOCK__APBDBG_CLK];
	clk_foo_clocks[CLOCK__L2_CLK].hw.clk = owl_clks[CLOCK__L2_CLK];
	clk_foo_clocks[CLOCK__ACP_CLK].hw.clk = owl_clks[CLOCK__ACP_CLK];
	clk_foo_clocks[CLOCK__PERIPH_CLK].hw.clk = owl_clks[CLOCK__PERIPH_CLK];
	clk_foo_clocks[CLOCK__NIC_DIV_CLK].hw.clk = owl_clks[CLOCK__NIC_DIV_CLK];
	clk_foo_clocks[CLOCK__NIC_CLK].hw.clk = owl_clks[CLOCK__NIC_CLK];
	clk_foo_clocks[CLOCK__AHBPREDIV_CLK].hw.clk = owl_clks[CLOCK__AHBPREDIV_CLK];
	clk_foo_clocks[CLOCK__H_CLK].hw.clk = owl_clks[CLOCK__H_CLK];
	clk_foo_clocks[CLOCK__APB30_CLK].hw.clk = owl_clks[CLOCK__APB30_CLK];
	clk_foo_clocks[CLOCK__APB20_CLK].hw.clk = owl_clks[CLOCK__APB20_CLK];
	clk_foo_clocks[CLOCK__AHB_CLK].hw.clk = owl_clks[CLOCK__AHB_CLK];
	clk_foo_clocks[CLOCK__CORE_CLK].hw.clk = owl_clks[CLOCK__CORE_CLK];
	clk_foo_clocks[CLOCK__CPU_CLK].hw.clk = owl_clks[CLOCK__CPU_CLK];
	clk_foo_clocks[CLOCK__SENSOR_CLKOUT0].hw.clk = owl_clks[CLOCK__SENSOR_CLKOUT0];
	clk_foo_clocks[CLOCK__SENSOR_CLKOUT1].hw.clk = owl_clks[CLOCK__SENSOR_CLKOUT1];
	clk_foo_clocks[CLOCK__LCD_CLK].hw.clk = owl_clks[CLOCK__LCD_CLK];
	clk_foo_clocks[CLOCK__LVDS_CLK].hw.clk = owl_clks[CLOCK__LVDS_CLK];
	clk_foo_clocks[CLOCK__CKA_LCD_H].hw.clk = owl_clks[CLOCK__CKA_LCD_H];
	clk_foo_clocks[CLOCK__LCD1_CLK].hw.clk = owl_clks[CLOCK__LCD1_CLK];
	clk_foo_clocks[CLOCK__LCD0_CLK].hw.clk = owl_clks[CLOCK__LCD0_CLK];
	clk_foo_clocks[CLOCK__DSI_HCLK].hw.clk = owl_clks[CLOCK__DSI_HCLK];
	clk_foo_clocks[CLOCK__DSI_HCLK90].hw.clk = owl_clks[CLOCK__DSI_HCLK90];
	clk_foo_clocks[CLOCK__PRO_CLK].hw.clk = owl_clks[CLOCK__PRO_CLK];
	clk_foo_clocks[CLOCK__PHY_CLK].hw.clk = owl_clks[CLOCK__PHY_CLK];
	clk_foo_clocks[CLOCK__CSI_CLK].hw.clk = owl_clks[CLOCK__CSI_CLK];
	clk_foo_clocks[CLOCK__DE1_CLK].hw.clk = owl_clks[CLOCK__DE1_CLK];
	clk_foo_clocks[CLOCK__DE2_CLK].hw.clk = owl_clks[CLOCK__DE2_CLK];
	clk_foo_clocks[CLOCK__BISP_CLK].hw.clk = owl_clks[CLOCK__BISP_CLK];
	clk_foo_clocks[CLOCK__ISPBP_CLK].hw.clk = owl_clks[CLOCK__ISPBP_CLK];
	clk_foo_clocks[CLOCK__IMG5_CLK].hw.clk = owl_clks[CLOCK__IMG5_CLK];
	clk_foo_clocks[CLOCK__VDE_CLK].hw.clk = owl_clks[CLOCK__VDE_CLK];
	clk_foo_clocks[CLOCK__VCE_CLK].hw.clk = owl_clks[CLOCK__VCE_CLK];
	clk_foo_clocks[CLOCK__NANDC_CLK].hw.clk = owl_clks[CLOCK__NANDC_CLK];
	clk_foo_clocks[CLOCK__ECC_CLK].hw.clk = owl_clks[CLOCK__ECC_CLK];
	clk_foo_clocks[CLOCK__PRESD0_CLK].hw.clk = owl_clks[CLOCK__PRESD0_CLK];
	clk_foo_clocks[CLOCK__PRESD1_CLK].hw.clk = owl_clks[CLOCK__PRESD1_CLK];
	clk_foo_clocks[CLOCK__PRESD2_CLK].hw.clk = owl_clks[CLOCK__PRESD2_CLK];
	clk_foo_clocks[CLOCK__SD0_CLK_2X].hw.clk = owl_clks[CLOCK__SD0_CLK_2X];
	clk_foo_clocks[CLOCK__SD1_CLK_2X].hw.clk = owl_clks[CLOCK__SD1_CLK_2X];
	clk_foo_clocks[CLOCK__SD2_CLK_2X].hw.clk = owl_clks[CLOCK__SD2_CLK_2X];
	clk_foo_clocks[CLOCK__SD0_CLK].hw.clk = owl_clks[CLOCK__SD0_CLK];
	clk_foo_clocks[CLOCK__SD1_CLK].hw.clk = owl_clks[CLOCK__SD1_CLK];
	clk_foo_clocks[CLOCK__SD2_CLK].hw.clk = owl_clks[CLOCK__SD2_CLK];
	clk_foo_clocks[CLOCK__UART0_CLK].hw.clk = owl_clks[CLOCK__UART0_CLK];
	clk_foo_clocks[CLOCK__UART1_CLK].hw.clk = owl_clks[CLOCK__UART1_CLK];
	clk_foo_clocks[CLOCK__UART2_CLK].hw.clk = owl_clks[CLOCK__UART2_CLK];
	clk_foo_clocks[CLOCK__UART3_CLK].hw.clk = owl_clks[CLOCK__UART3_CLK];
	clk_foo_clocks[CLOCK__UART4_CLK].hw.clk = owl_clks[CLOCK__UART4_CLK];
	clk_foo_clocks[CLOCK__UART5_CLK].hw.clk = owl_clks[CLOCK__UART5_CLK];
	clk_foo_clocks[CLOCK__UART6_CLK].hw.clk = owl_clks[CLOCK__UART6_CLK];
	clk_foo_clocks[CLOCK__PWM0_CLK].hw.clk = owl_clks[CLOCK__PWM0_CLK];
	clk_foo_clocks[CLOCK__PWM1_CLK].hw.clk = owl_clks[CLOCK__PWM1_CLK];
	clk_foo_clocks[CLOCK__PWM2_CLK].hw.clk = owl_clks[CLOCK__PWM2_CLK];
	clk_foo_clocks[CLOCK__PWM3_CLK].hw.clk = owl_clks[CLOCK__PWM3_CLK];
	clk_foo_clocks[CLOCK__PWM4_CLK].hw.clk = owl_clks[CLOCK__PWM4_CLK];
	clk_foo_clocks[CLOCK__PWM5_CLK].hw.clk = owl_clks[CLOCK__PWM5_CLK];
	clk_foo_clocks[CLOCK__RMII_REF_CLK].hw.clk = owl_clks[CLOCK__RMII_REF_CLK];
	clk_foo_clocks[CLOCK__I2C0_CLK].hw.clk = owl_clks[CLOCK__I2C0_CLK];
	clk_foo_clocks[CLOCK__I2C1_CLK].hw.clk = owl_clks[CLOCK__I2C1_CLK];
	clk_foo_clocks[CLOCK__I2C2_CLK].hw.clk = owl_clks[CLOCK__I2C2_CLK];
	clk_foo_clocks[CLOCK__I2C3_CLK].hw.clk = owl_clks[CLOCK__I2C3_CLK];
	clk_foo_clocks[CLOCK__25M_CLK].hw.clk = owl_clks[CLOCK__25M_CLK];
	clk_foo_clocks[CLOCK__LENS_CLK].hw.clk = owl_clks[CLOCK__LENS_CLK];
	clk_foo_clocks[CLOCK__HDMI24M].hw.clk = owl_clks[CLOCK__HDMI24M];
	clk_foo_clocks[CLOCK__TIMER_CLK].hw.clk = owl_clks[CLOCK__TIMER_CLK];
	clk_foo_clocks[CLOCK__SS_CLK].hw.clk = owl_clks[CLOCK__SS_CLK];
	clk_foo_clocks[CLOCK__SPS_CLK].hw.clk = owl_clks[CLOCK__SPS_CLK];
	clk_foo_clocks[CLOCK__IRC_CLK].hw.clk = owl_clks[CLOCK__IRC_CLK];
	clk_foo_clocks[CLOCK__TV24M].hw.clk = owl_clks[CLOCK__TV24M];
	clk_foo_clocks[CLOCK__CVBS_CLK108M].hw.clk = owl_clks[CLOCK__CVBS_CLK108M];
	clk_foo_clocks[CLOCK__MIPI24M].hw.clk = owl_clks[CLOCK__MIPI24M];
	clk_foo_clocks[CLOCK__LENS24M].hw.clk = owl_clks[CLOCK__LENS24M];
	clk_foo_clocks[CLOCK__GPU3D_SYSCLK].hw.clk = owl_clks[CLOCK__GPU3D_SYSCLK];
	clk_foo_clocks[CLOCK__GPU3D_HYDCLK].hw.clk = owl_clks[CLOCK__GPU3D_HYDCLK];
	clk_foo_clocks[CLOCK__GPU3D_NIC_MEMCLK].hw.clk = owl_clks[CLOCK__GPU3D_NIC_MEMCLK];
	clk_foo_clocks[CLOCK__GPU3D_CORECLK].hw.clk = owl_clks[CLOCK__GPU3D_CORECLK];

	clk_foo_modules[MOD__ROOT].hw.clk = owl_clks[CLOCK__MAX + MOD__ROOT];
	clk_foo_modules[MOD__GPU3D].hw.clk = owl_clks[CLOCK__MAX + MOD__GPU3D];
	clk_foo_modules[MOD__SHARESRAM].hw.clk = owl_clks[CLOCK__MAX + MOD__SHARESRAM];
	clk_foo_modules[MOD__HDCP2X].hw.clk = owl_clks[CLOCK__MAX + MOD__HDCP2X];
	clk_foo_modules[MOD__VCE].hw.clk = owl_clks[CLOCK__MAX + MOD__VCE];
	clk_foo_modules[MOD__VDE].hw.clk = owl_clks[CLOCK__MAX + MOD__VDE];
	clk_foo_modules[MOD__PCM0].hw.clk = owl_clks[CLOCK__MAX + MOD__PCM0];
	clk_foo_modules[MOD__SPDIF].hw.clk = owl_clks[CLOCK__MAX + MOD__SPDIF];
	clk_foo_modules[MOD__HDMIA].hw.clk = owl_clks[CLOCK__MAX + MOD__HDMIA];
	clk_foo_modules[MOD__I2SRX].hw.clk = owl_clks[CLOCK__MAX + MOD__I2SRX];
	clk_foo_modules[MOD__I2STX].hw.clk = owl_clks[CLOCK__MAX + MOD__I2STX];
	clk_foo_modules[MOD__GPIO].hw.clk = owl_clks[CLOCK__MAX + MOD__GPIO];
	clk_foo_modules[MOD__KEY].hw.clk = owl_clks[CLOCK__MAX + MOD__KEY];
	clk_foo_modules[MOD__LENS].hw.clk = owl_clks[CLOCK__MAX + MOD__LENS];
	clk_foo_modules[MOD__BISP].hw.clk = owl_clks[CLOCK__MAX + MOD__BISP];
	clk_foo_modules[MOD__CSI].hw.clk = owl_clks[CLOCK__MAX + MOD__CSI];
	clk_foo_modules[MOD__DSI].hw.clk = owl_clks[CLOCK__MAX + MOD__DSI];
	clk_foo_modules[MOD__LVDS].hw.clk = owl_clks[CLOCK__MAX + MOD__LVDS];
	clk_foo_modules[MOD__LCD1].hw.clk = owl_clks[CLOCK__MAX + MOD__LCD1];
	clk_foo_modules[MOD__LCD0].hw.clk = owl_clks[CLOCK__MAX + MOD__LCD0];
	clk_foo_modules[MOD__DE].hw.clk = owl_clks[CLOCK__MAX + MOD__DE];
	clk_foo_modules[MOD__SD2].hw.clk = owl_clks[CLOCK__MAX + MOD__SD2];
	clk_foo_modules[MOD__SD1].hw.clk = owl_clks[CLOCK__MAX + MOD__SD1];
	clk_foo_modules[MOD__SD0].hw.clk = owl_clks[CLOCK__MAX + MOD__SD0];
	clk_foo_modules[MOD__NANDC].hw.clk = owl_clks[CLOCK__MAX + MOD__NANDC];
	clk_foo_modules[MOD__DDRCH0].hw.clk = owl_clks[CLOCK__MAX + MOD__DDRCH0];
	clk_foo_modules[MOD__NOR].hw.clk = owl_clks[CLOCK__MAX + MOD__NOR];
	clk_foo_modules[MOD__DMAC].hw.clk = owl_clks[CLOCK__MAX + MOD__DMAC];
	clk_foo_modules[MOD__DDRCH1].hw.clk = owl_clks[CLOCK__MAX + MOD__DDRCH1];
	clk_foo_modules[MOD__I2C3].hw.clk = owl_clks[CLOCK__MAX + MOD__I2C3];
	clk_foo_modules[MOD__I2C2].hw.clk = owl_clks[CLOCK__MAX + MOD__I2C2];
	clk_foo_modules[MOD__TIMER].hw.clk = owl_clks[CLOCK__MAX + MOD__TIMER];
	clk_foo_modules[MOD__PWM5].hw.clk = owl_clks[CLOCK__MAX + MOD__PWM5];
	clk_foo_modules[MOD__PWM4].hw.clk = owl_clks[CLOCK__MAX + MOD__PWM4];
	clk_foo_modules[MOD__PWM3].hw.clk = owl_clks[CLOCK__MAX + MOD__PWM3];
	clk_foo_modules[MOD__PWM2].hw.clk = owl_clks[CLOCK__MAX + MOD__PWM2];
	clk_foo_modules[MOD__PWM1].hw.clk = owl_clks[CLOCK__MAX + MOD__PWM1];
	clk_foo_modules[MOD__PWM0].hw.clk = owl_clks[CLOCK__MAX + MOD__PWM0];
	clk_foo_modules[MOD__ETHERNET].hw.clk = owl_clks[CLOCK__MAX + MOD__ETHERNET];
	clk_foo_modules[MOD__UART5].hw.clk = owl_clks[CLOCK__MAX + MOD__UART5];
	clk_foo_modules[MOD__UART4].hw.clk = owl_clks[CLOCK__MAX + MOD__UART4];
	clk_foo_modules[MOD__UART3].hw.clk = owl_clks[CLOCK__MAX + MOD__UART3];
	clk_foo_modules[MOD__UART6].hw.clk = owl_clks[CLOCK__MAX + MOD__UART6];
	clk_foo_modules[MOD__PCM1].hw.clk = owl_clks[CLOCK__MAX + MOD__PCM1];
	clk_foo_modules[MOD__I2C1].hw.clk = owl_clks[CLOCK__MAX + MOD__I2C1];
	clk_foo_modules[MOD__I2C0].hw.clk = owl_clks[CLOCK__MAX + MOD__I2C0];
	clk_foo_modules[MOD__SPI3].hw.clk = owl_clks[CLOCK__MAX + MOD__SPI3];
	clk_foo_modules[MOD__SPI2].hw.clk = owl_clks[CLOCK__MAX + MOD__SPI2];
	clk_foo_modules[MOD__SPI1].hw.clk = owl_clks[CLOCK__MAX + MOD__SPI1];
	clk_foo_modules[MOD__SPI0].hw.clk = owl_clks[CLOCK__MAX + MOD__SPI0];
	clk_foo_modules[MOD__IRC].hw.clk = owl_clks[CLOCK__MAX + MOD__IRC];
	clk_foo_modules[MOD__UART2].hw.clk = owl_clks[CLOCK__MAX + MOD__UART2];
	clk_foo_modules[MOD__UART1].hw.clk = owl_clks[CLOCK__MAX + MOD__UART1];
	clk_foo_modules[MOD__UART0].hw.clk = owl_clks[CLOCK__MAX + MOD__UART0];
	clk_foo_modules[MOD__HDMI].hw.clk = owl_clks[CLOCK__MAX + MOD__HDMI];
	clk_foo_modules[MOD__SS].hw.clk = owl_clks[CLOCK__MAX + MOD__SS];
	clk_foo_modules[MOD__TV24M].hw.clk = owl_clks[CLOCK__MAX + MOD__TV24M];
	clk_foo_modules[MOD__CVBS_CLK108M].hw.clk = owl_clks[CLOCK__MAX + MOD__CVBS_CLK108M];
	clk_foo_modules[MOD__TVOUT].hw.clk = owl_clks[CLOCK__MAX + MOD__TVOUT];

	lookup_table[CLOCK__HOSC].clk = owl_clks[CLOCK__HOSC];
	lookup_table[CLOCK__IC_32K].clk = owl_clks[CLOCK__IC_32K];
	lookup_table[CLOCK__COREPLL].clk = owl_clks[CLOCK__COREPLL];
	lookup_table[CLOCK__DEVPLL].clk = owl_clks[CLOCK__DEVPLL];
	lookup_table[CLOCK__DDRPLL].clk = owl_clks[CLOCK__DDRPLL];
	lookup_table[CLOCK__NANDPLL].clk = owl_clks[CLOCK__NANDPLL];
	lookup_table[CLOCK__DISPLAYPLL].clk = owl_clks[CLOCK__DISPLAYPLL];
	lookup_table[CLOCK__AUDIOPLL].clk = owl_clks[CLOCK__AUDIOPLL];
	lookup_table[CLOCK__TVOUTPLL].clk = owl_clks[CLOCK__TVOUTPLL];
	lookup_table[CLOCK__ETHERNETPLL].clk = owl_clks[CLOCK__ETHERNETPLL];
	lookup_table[CLOCK__CVBSPLL].clk = owl_clks[CLOCK__CVBSPLL];
	lookup_table[CLOCK__DEV_CLK].clk = owl_clks[CLOCK__DEV_CLK];
	lookup_table[CLOCK__DDR_CLK_0].clk = owl_clks[CLOCK__DDR_CLK_0];
	lookup_table[CLOCK__DDR_CLK_90].clk = owl_clks[CLOCK__DDR_CLK_90];
	lookup_table[CLOCK__DDR_CLK_180].clk = owl_clks[CLOCK__DDR_CLK_180];
	lookup_table[CLOCK__DDR_CLK_270].clk = owl_clks[CLOCK__DDR_CLK_270];
	lookup_table[CLOCK__DDR_CLK_CH0].clk = owl_clks[CLOCK__DDR_CLK_CH0];
	lookup_table[CLOCK__DDR_CLK_CH1].clk = owl_clks[CLOCK__DDR_CLK_CH1];
	lookup_table[CLOCK__DDR_CLK].clk = owl_clks[CLOCK__DDR_CLK];
	lookup_table[CLOCK__SPDIF_CLK].clk = owl_clks[CLOCK__SPDIF_CLK];
	lookup_table[CLOCK__HDMIA_CLK].clk = owl_clks[CLOCK__HDMIA_CLK];
	lookup_table[CLOCK__I2SRX_CLK].clk = owl_clks[CLOCK__I2SRX_CLK];
	lookup_table[CLOCK__I2STX_CLK].clk = owl_clks[CLOCK__I2STX_CLK];
	lookup_table[CLOCK__PCM0_CLK].clk = owl_clks[CLOCK__PCM0_CLK];
	lookup_table[CLOCK__PCM1_CLK].clk = owl_clks[CLOCK__PCM1_CLK];
	lookup_table[CLOCK__CLK_PIXEL].clk = owl_clks[CLOCK__CLK_PIXEL];
	lookup_table[CLOCK__CLK_TMDS].clk = owl_clks[CLOCK__CLK_TMDS];
	lookup_table[CLOCK__CLK_TMDS_PHY_P].clk = owl_clks[CLOCK__CLK_TMDS_PHY_P];
	lookup_table[CLOCK__L2_NIC_CLK].clk = owl_clks[CLOCK__L2_NIC_CLK];
	lookup_table[CLOCK__APBDBG_CLK].clk = owl_clks[CLOCK__APBDBG_CLK];
	lookup_table[CLOCK__L2_CLK].clk = owl_clks[CLOCK__L2_CLK];
	lookup_table[CLOCK__ACP_CLK].clk = owl_clks[CLOCK__ACP_CLK];
	lookup_table[CLOCK__PERIPH_CLK].clk = owl_clks[CLOCK__PERIPH_CLK];
	lookup_table[CLOCK__NIC_DIV_CLK].clk = owl_clks[CLOCK__NIC_DIV_CLK];
	lookup_table[CLOCK__NIC_CLK].clk = owl_clks[CLOCK__NIC_CLK];
	lookup_table[CLOCK__AHBPREDIV_CLK].clk = owl_clks[CLOCK__AHBPREDIV_CLK];
	lookup_table[CLOCK__H_CLK].clk = owl_clks[CLOCK__H_CLK];
	lookup_table[CLOCK__APB30_CLK].clk = owl_clks[CLOCK__APB30_CLK];
	lookup_table[CLOCK__APB20_CLK].clk = owl_clks[CLOCK__APB20_CLK];
	lookup_table[CLOCK__AHB_CLK].clk = owl_clks[CLOCK__AHB_CLK];
	lookup_table[CLOCK__CORE_CLK].clk = owl_clks[CLOCK__CORE_CLK];
	lookup_table[CLOCK__CPU_CLK].clk = owl_clks[CLOCK__CPU_CLK];
	lookup_table[CLOCK__SENSOR_CLKOUT0].clk = owl_clks[CLOCK__SENSOR_CLKOUT0];
	lookup_table[CLOCK__SENSOR_CLKOUT1].clk = owl_clks[CLOCK__SENSOR_CLKOUT1];
	lookup_table[CLOCK__LCD_CLK].clk = owl_clks[CLOCK__LCD_CLK];
	lookup_table[CLOCK__LVDS_CLK].clk = owl_clks[CLOCK__LVDS_CLK];
	lookup_table[CLOCK__CKA_LCD_H].clk = owl_clks[CLOCK__CKA_LCD_H];
	lookup_table[CLOCK__LCD1_CLK].clk = owl_clks[CLOCK__LCD1_CLK];
	lookup_table[CLOCK__LCD0_CLK].clk = owl_clks[CLOCK__LCD0_CLK];
	lookup_table[CLOCK__DSI_HCLK].clk = owl_clks[CLOCK__DSI_HCLK];
	lookup_table[CLOCK__DSI_HCLK90].clk = owl_clks[CLOCK__DSI_HCLK90];
	lookup_table[CLOCK__PRO_CLK].clk = owl_clks[CLOCK__PRO_CLK];
	lookup_table[CLOCK__PHY_CLK].clk = owl_clks[CLOCK__PHY_CLK];
	lookup_table[CLOCK__CSI_CLK].clk = owl_clks[CLOCK__CSI_CLK];
	lookup_table[CLOCK__DE1_CLK].clk = owl_clks[CLOCK__DE1_CLK];
	lookup_table[CLOCK__DE2_CLK].clk = owl_clks[CLOCK__DE2_CLK];
	lookup_table[CLOCK__BISP_CLK].clk = owl_clks[CLOCK__BISP_CLK];
	lookup_table[CLOCK__ISPBP_CLK].clk = owl_clks[CLOCK__ISPBP_CLK];
	lookup_table[CLOCK__IMG5_CLK].clk = owl_clks[CLOCK__IMG5_CLK];
	lookup_table[CLOCK__VDE_CLK].clk = owl_clks[CLOCK__VDE_CLK];
	lookup_table[CLOCK__VCE_CLK].clk = owl_clks[CLOCK__VCE_CLK];
	lookup_table[CLOCK__NANDC_CLK].clk = owl_clks[CLOCK__NANDC_CLK];
	lookup_table[CLOCK__ECC_CLK].clk = owl_clks[CLOCK__ECC_CLK];
	lookup_table[CLOCK__PRESD0_CLK].clk = owl_clks[CLOCK__PRESD0_CLK];
	lookup_table[CLOCK__PRESD1_CLK].clk = owl_clks[CLOCK__PRESD1_CLK];
	lookup_table[CLOCK__PRESD2_CLK].clk = owl_clks[CLOCK__PRESD2_CLK];
	lookup_table[CLOCK__SD0_CLK_2X].clk = owl_clks[CLOCK__SD0_CLK_2X];
	lookup_table[CLOCK__SD1_CLK_2X].clk = owl_clks[CLOCK__SD1_CLK_2X];
	lookup_table[CLOCK__SD2_CLK_2X].clk = owl_clks[CLOCK__SD2_CLK_2X];
	lookup_table[CLOCK__SD0_CLK].clk = owl_clks[CLOCK__SD0_CLK];
	lookup_table[CLOCK__SD1_CLK].clk = owl_clks[CLOCK__SD1_CLK];
	lookup_table[CLOCK__SD2_CLK].clk = owl_clks[CLOCK__SD2_CLK];
	lookup_table[CLOCK__UART0_CLK].clk = owl_clks[CLOCK__UART0_CLK];
	lookup_table[CLOCK__UART1_CLK].clk = owl_clks[CLOCK__UART1_CLK];
	lookup_table[CLOCK__UART2_CLK].clk = owl_clks[CLOCK__UART2_CLK];
	lookup_table[CLOCK__UART3_CLK].clk = owl_clks[CLOCK__UART3_CLK];
	lookup_table[CLOCK__UART4_CLK].clk = owl_clks[CLOCK__UART4_CLK];
	lookup_table[CLOCK__UART5_CLK].clk = owl_clks[CLOCK__UART5_CLK];
	lookup_table[CLOCK__UART6_CLK].clk = owl_clks[CLOCK__UART6_CLK];
	lookup_table[CLOCK__PWM0_CLK].clk = owl_clks[CLOCK__PWM0_CLK];
	lookup_table[CLOCK__PWM1_CLK].clk = owl_clks[CLOCK__PWM1_CLK];
	lookup_table[CLOCK__PWM2_CLK].clk = owl_clks[CLOCK__PWM2_CLK];
	lookup_table[CLOCK__PWM3_CLK].clk = owl_clks[CLOCK__PWM3_CLK];
	lookup_table[CLOCK__PWM4_CLK].clk = owl_clks[CLOCK__PWM4_CLK];
	lookup_table[CLOCK__PWM5_CLK].clk = owl_clks[CLOCK__PWM5_CLK];
	lookup_table[CLOCK__RMII_REF_CLK].clk = owl_clks[CLOCK__RMII_REF_CLK];
	lookup_table[CLOCK__I2C0_CLK].clk = owl_clks[CLOCK__I2C0_CLK];
	lookup_table[CLOCK__I2C1_CLK].clk = owl_clks[CLOCK__I2C1_CLK];
	lookup_table[CLOCK__I2C2_CLK].clk = owl_clks[CLOCK__I2C2_CLK];
	lookup_table[CLOCK__I2C3_CLK].clk = owl_clks[CLOCK__I2C3_CLK];
	lookup_table[CLOCK__25M_CLK].clk = owl_clks[CLOCK__25M_CLK];
	lookup_table[CLOCK__LENS_CLK].clk = owl_clks[CLOCK__LENS_CLK];
	lookup_table[CLOCK__HDMI24M].clk = owl_clks[CLOCK__HDMI24M];
	lookup_table[CLOCK__TIMER_CLK].clk = owl_clks[CLOCK__TIMER_CLK];
	lookup_table[CLOCK__SS_CLK].clk = owl_clks[CLOCK__SS_CLK];
	lookup_table[CLOCK__SPS_CLK].clk = owl_clks[CLOCK__SPS_CLK];
	lookup_table[CLOCK__IRC_CLK].clk = owl_clks[CLOCK__IRC_CLK];
	lookup_table[CLOCK__TV24M].clk = owl_clks[CLOCK__TV24M];
	lookup_table[CLOCK__CVBS_CLK108M].clk = owl_clks[CLOCK__CVBS_CLK108M];
	lookup_table[CLOCK__MIPI24M].clk = owl_clks[CLOCK__MIPI24M];
	lookup_table[CLOCK__LENS24M].clk = owl_clks[CLOCK__LENS24M];
	lookup_table[CLOCK__GPU3D_SYSCLK].clk = owl_clks[CLOCK__GPU3D_SYSCLK];
	lookup_table[CLOCK__GPU3D_HYDCLK].clk = owl_clks[CLOCK__GPU3D_HYDCLK];
	lookup_table[CLOCK__GPU3D_NIC_MEMCLK].clk = owl_clks[CLOCK__GPU3D_NIC_MEMCLK];
	lookup_table[CLOCK__GPU3D_CORECLK].clk = owl_clks[CLOCK__GPU3D_CORECLK];
	lookup_table[CLOCK__MAX + MOD__ROOT].clk = owl_clks[CLOCK__MAX + MOD__ROOT];
	lookup_table[CLOCK__MAX + MOD__GPU3D].clk = owl_clks[CLOCK__MAX + MOD__GPU3D];
	lookup_table[CLOCK__MAX + MOD__SHARESRAM].clk = owl_clks[CLOCK__MAX + MOD__SHARESRAM];
	lookup_table[CLOCK__MAX + MOD__HDCP2X].clk = owl_clks[CLOCK__MAX + MOD__HDCP2X];
	lookup_table[CLOCK__MAX + MOD__VCE].clk = owl_clks[CLOCK__MAX + MOD__VCE];
	lookup_table[CLOCK__MAX + MOD__VDE].clk = owl_clks[CLOCK__MAX + MOD__VDE];
	lookup_table[CLOCK__MAX + MOD__PCM0].clk = owl_clks[CLOCK__MAX + MOD__PCM0];
	lookup_table[CLOCK__MAX + MOD__SPDIF].clk = owl_clks[CLOCK__MAX + MOD__SPDIF];
	lookup_table[CLOCK__MAX + MOD__HDMIA].clk = owl_clks[CLOCK__MAX + MOD__HDMIA];
	lookup_table[CLOCK__MAX + MOD__I2SRX].clk = owl_clks[CLOCK__MAX + MOD__I2SRX];
	lookup_table[CLOCK__MAX + MOD__I2STX].clk = owl_clks[CLOCK__MAX + MOD__I2STX];
	lookup_table[CLOCK__MAX + MOD__GPIO].clk = owl_clks[CLOCK__MAX + MOD__GPIO];
	lookup_table[CLOCK__MAX + MOD__KEY].clk = owl_clks[CLOCK__MAX + MOD__KEY];
	lookup_table[CLOCK__MAX + MOD__LENS].clk = owl_clks[CLOCK__MAX + MOD__LENS];
	lookup_table[CLOCK__MAX + MOD__BISP].clk = owl_clks[CLOCK__MAX + MOD__BISP];
	lookup_table[CLOCK__MAX + MOD__CSI].clk = owl_clks[CLOCK__MAX + MOD__CSI];
	lookup_table[CLOCK__MAX + MOD__DSI].clk = owl_clks[CLOCK__MAX + MOD__DSI];
	lookup_table[CLOCK__MAX + MOD__LVDS].clk = owl_clks[CLOCK__MAX + MOD__LVDS];
	lookup_table[CLOCK__MAX + MOD__LCD1].clk = owl_clks[CLOCK__MAX + MOD__LCD1];
	lookup_table[CLOCK__MAX + MOD__LCD0].clk = owl_clks[CLOCK__MAX + MOD__LCD0];
	lookup_table[CLOCK__MAX + MOD__DE].clk = owl_clks[CLOCK__MAX + MOD__DE];
	lookup_table[CLOCK__MAX + MOD__SD2].clk = owl_clks[CLOCK__MAX + MOD__SD2];
	lookup_table[CLOCK__MAX + MOD__SD1].clk = owl_clks[CLOCK__MAX + MOD__SD1];
	lookup_table[CLOCK__MAX + MOD__SD0].clk = owl_clks[CLOCK__MAX + MOD__SD0];
	lookup_table[CLOCK__MAX + MOD__NANDC].clk = owl_clks[CLOCK__MAX + MOD__NANDC];
	lookup_table[CLOCK__MAX + MOD__DDRCH0].clk = owl_clks[CLOCK__MAX + MOD__DDRCH0];
	lookup_table[CLOCK__MAX + MOD__NOR].clk = owl_clks[CLOCK__MAX + MOD__NOR];
	lookup_table[CLOCK__MAX + MOD__DMAC].clk = owl_clks[CLOCK__MAX + MOD__DMAC];
	lookup_table[CLOCK__MAX + MOD__DDRCH1].clk = owl_clks[CLOCK__MAX + MOD__DDRCH1];
	lookup_table[CLOCK__MAX + MOD__I2C3].clk = owl_clks[CLOCK__MAX + MOD__I2C3];
	lookup_table[CLOCK__MAX + MOD__I2C2].clk = owl_clks[CLOCK__MAX + MOD__I2C2];
	lookup_table[CLOCK__MAX + MOD__TIMER].clk = owl_clks[CLOCK__MAX + MOD__TIMER];
	lookup_table[CLOCK__MAX + MOD__PWM5].clk = owl_clks[CLOCK__MAX + MOD__PWM5];
	lookup_table[CLOCK__MAX + MOD__PWM4].clk = owl_clks[CLOCK__MAX + MOD__PWM4];
	lookup_table[CLOCK__MAX + MOD__PWM3].clk = owl_clks[CLOCK__MAX + MOD__PWM3];
	lookup_table[CLOCK__MAX + MOD__PWM2].clk = owl_clks[CLOCK__MAX + MOD__PWM2];
	lookup_table[CLOCK__MAX + MOD__PWM1].clk = owl_clks[CLOCK__MAX + MOD__PWM1];
	lookup_table[CLOCK__MAX + MOD__PWM0].clk = owl_clks[CLOCK__MAX + MOD__PWM0];
	lookup_table[CLOCK__MAX + MOD__ETHERNET].clk = owl_clks[CLOCK__MAX + MOD__ETHERNET];
	lookup_table[CLOCK__MAX + MOD__UART5].clk = owl_clks[CLOCK__MAX + MOD__UART5];
	lookup_table[CLOCK__MAX + MOD__UART4].clk = owl_clks[CLOCK__MAX + MOD__UART4];
	lookup_table[CLOCK__MAX + MOD__UART3].clk = owl_clks[CLOCK__MAX + MOD__UART3];
	lookup_table[CLOCK__MAX + MOD__UART6].clk = owl_clks[CLOCK__MAX + MOD__UART6];
	lookup_table[CLOCK__MAX + MOD__PCM1].clk = owl_clks[CLOCK__MAX + MOD__PCM1];
	lookup_table[CLOCK__MAX + MOD__I2C1].clk = owl_clks[CLOCK__MAX + MOD__I2C1];
	lookup_table[CLOCK__MAX + MOD__I2C0].clk = owl_clks[CLOCK__MAX + MOD__I2C0];
	lookup_table[CLOCK__MAX + MOD__SPI3].clk = owl_clks[CLOCK__MAX + MOD__SPI3];
	lookup_table[CLOCK__MAX + MOD__SPI2].clk = owl_clks[CLOCK__MAX + MOD__SPI2];
	lookup_table[CLOCK__MAX + MOD__SPI1].clk = owl_clks[CLOCK__MAX + MOD__SPI1];
	lookup_table[CLOCK__MAX + MOD__SPI0].clk = owl_clks[CLOCK__MAX + MOD__SPI0];
	lookup_table[CLOCK__MAX + MOD__IRC].clk = owl_clks[CLOCK__MAX + MOD__IRC];
	lookup_table[CLOCK__MAX + MOD__UART2].clk = owl_clks[CLOCK__MAX + MOD__UART2];
	lookup_table[CLOCK__MAX + MOD__UART1].clk = owl_clks[CLOCK__MAX + MOD__UART1];
	lookup_table[CLOCK__MAX + MOD__UART0].clk = owl_clks[CLOCK__MAX + MOD__UART0];
	lookup_table[CLOCK__MAX + MOD__HDMI].clk = owl_clks[CLOCK__MAX + MOD__HDMI];
	lookup_table[CLOCK__MAX + MOD__SS].clk = owl_clks[CLOCK__MAX + MOD__SS];
	lookup_table[CLOCK__MAX + MOD__TV24M].clk = owl_clks[CLOCK__MAX + MOD__TV24M];
	lookup_table[CLOCK__MAX + MOD__CVBS_CLK108M].clk = owl_clks[CLOCK__MAX + MOD__CVBS_CLK108M];
	lookup_table[CLOCK__MAX + MOD__TVOUT].clk = owl_clks[CLOCK__MAX + MOD__TVOUT];

	owl_clk_config_recursion(CLOCK__LCD0_CLK, 1);
	owl_clk_config_recursion(CLOCK__LCD1_CLK, 1);
	owl_clk_config_recursion(CLOCK__IMG5_CLK, 1);
	owl_clk_config_recursion(CLOCK__DSI_HCLK90, 1);
	owl_clk_config_recursion(CLOCK__PRO_CLK, 1);
	owl_clk_config_recursion(CLOCK__PHY_CLK, 1);
	owl_clk_config_recursion(CLOCK__LCD_CLK, 2);
	owl_clk_config_recursion(CLOCK__DSI_HCLK, 2);
}

void atm7059_prepare_clocktree(void)
{
    int i;

	/* recover refer count from cmu registers */
	clk_prepare(owl_clks[CLOCK__IC_32K]);
	clk_enable(owl_clks[CLOCK__IC_32K]);

	if (read_clkreg_val(&enablebit_HOSC) == 1) {
		clk_prepare(owl_clks[CLOCK__HOSC]);
		clk_enable(owl_clks[CLOCK__HOSC]);
	}

	if (__clk_is_enabled(owl_clks[CLOCK__HOSC])) {
		for (i = 0; i < PLL__MAX; i++) {
			if (pllnode[i].reg_pllen && read_clkreg_val(pllnode[i].reg_pllen) == 1) {
				clk_prepare(owl_clks[CLOCK__HOSC]);
				clk_enable(owl_clks[CLOCK__HOSC]);
				clk_prepare(owl_clks[CLOCK__COREPLL + i]);
				clk_enable(owl_clks[CLOCK__COREPLL + i]);
			}
		}
	}

	clk_prepare(owl_clks[CLOCK__MAX + MOD__ROOT]);
	clk_enable(owl_clks[CLOCK__MAX + MOD__ROOT]);
	for (i = 1; i < MOD__MAX_IN_CLK; i++) {
		if (modnode[i].reg_devclken && read_clkreg_val(modnode[i].reg_devclken) == 1) {
			clk_prepare(owl_clks[CLOCK__MAX + MOD__ROOT]);
			clk_enable(owl_clks[CLOCK__MAX + MOD__ROOT]);
			clk_prepare(owl_clks[CLOCK__MAX + i]);
			clk_enable(owl_clks[CLOCK__MAX + i]);
		}
	}
}
