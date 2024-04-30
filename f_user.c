// N.Kurz, EE, GSI,  3-Feb-2010
// N.Kurz, EE, GSI, 11-Sep-2014: small changes
// N.Kurz, EE, GSI, 27-Mar-2015: adopted for mbspex lib
// N.Kurz, EE, GSI, 15-Jun-2018: white rabbit timestamp read added
//  S.Minami J.Adamczewski-Musch EE, GSI, 27-Jun-2019: Zero suppression mode debugged
// N.Kurz, EE, GSI,  7-Sep-2020: USE_KINPEX_V5 activates new functions added to the kinpex firmware
//                               version 5.0 by S.Minami. reduces number of accesses to KINPEX register
//                               during token readout.                                   
// S.Minami,EE,GSI, 16-Nov_2020: USE_KINPEX_V5 mode debugged together with DATA_REDUCTION 


//#define DEBUG 1

//#define USE_KINPEX_V5 1

#define USE_MBSPEX_LIB 1

#define WR_TIME_STAMP        1

#define LONG_TRIGGER_WINDOW  1

#ifdef WR_TIME_STAMP
#define USE_TLU_FINE_TIME   1
#define WR_USE_TLU_DIRECT   1
#endif

#define SERIALIZE_IO __asm__ volatile ("mfence" ::: "memory")

//#define PATTERN_UNIT 1

#include "stdio.h"
#include "s_veshe.h"
#include "stdarg.h"
#include <sys/file.h>
#ifndef Linux
#include <mem.h>
#include <smem.h>
#else
#include "smem_mbs.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sched.h>
#endif

#include "sbs_def.h"
#include "error_mac.h"
#include "errnum_def.h"
#include "err_mask_def.h"
#include "f_ut_printm.h"
#include "f_user_trig_clear.h"

#include  "./pexor_gosip.h"

#ifdef USE_MBSPEX_LIB
#include "mbspex/libmbspex.h"
#endif

#ifdef WR_TIME_STAMP
#include <etherbone.h>
#include <gsi_tm_latch.h> // wishbone devices
#endif // WR_TIME_STAMP

// USER CHANGE AREA:

//----------------------------------------------------------------------------
// Readout system architecture
//----------------------------------------------------------------------------

// Nr of slaves on SFP         0  1  2  3
//                             |  |  |  |
#define NR_SLAVES            { 0, 7, 0, 0}

// TAMEX module type on SFP    0  1  2  3
//                             |  |  |  |
#define TAMEX_TM             { 0, 3, 0, 0}  //  1: TAMEX2 with passive signal input interface 
											//  2: TAMEX2 with PADI    signal input interface
											//  3: TAMEX-PADI1, TAMEX4
											// 10: TAMEX3
											// 41: TAMEX4 with PQDC1 Frontend


//#define DONT_RE_INITIALIZE_MODULES_AFTER_ERROR 1 // Henning

#define DATA_REDUCTION  0x1		// switch on meta data suppression for all readout tamex boards in case of no
//#define DATA_REDUCTION  0x0 	// switch on meta data suppression for all readout tamex boards in case of no
								// hit found in tamex:
								// 0x1: meta data reduction enabled
								// 0x0:                     disabled

#define DISABLE_CHANNEL    0x0  // expert setting, leave as it is
#define FIFO_ALMOSTFULL_TH 0x50 // expert setting, leave as it is
//#define FIFO_ALMOSTFULL_TH 0x30 // expert setting, leave as it is

//----------------------------------------------------------------------------
// Clock source specification for TDC clock
//----------------------------------------------------------------------------


								// TAMEX-PADI1 / TAMEX4
#define CLK_SRC_TDC_TAM4_PADI 0x4  // TAMEX-PADI1: 0x0 -> CLK from previous module via backplane
								   //                     (feed in 200 MHz on crate interface)
								   //              0x1 -> CLK from on-board oscillator
								   //              0x2 -> CLK from module front panel (feed in 200 MHz)
								   //              0x4 -> CLK from previous module via backplane
								   //                     (first module CLK-master with local CLK)
								   //              0x8 -> CLK from previous module via backplane
								   //                     (first module CLK-master with external CLK from front)

								   // TAMEX2:
#define CLK_SRC_TDC_TAM2      0x24 // TAMEX2: 0x20 -> External CLK via 2 pin lemo (200 MHz)
								   //         0x21 -> CLK from TRBus (25 MHz) via on-board PLL            ! to be tested
								   //         0x22 -> CLK from TRBus + Module 0 feeds 25 MHz CLK to TRBus ! to be tested
								   //         0x24 -> On-board oscillator (200 MHz)

								   // TAMEX3:
#define CLK_SRC_TDC_TAM3      0x2a // TAMEX3: 0x26 -> External CLK from backplane (200 MHz)
								   //         0x2a -> On-board oscillator (200 MHz)
								   //         0x22 -> CLK from TRBus (25 MHz) via on-board PLL
								   //                 (Module 0 feeds 25 MHz CLK to TRBus) 
								   //         0x20 -> External CLK from backplane (25 MHz)

//----------------------------------------------------------------------------
// PQDC/PADIWA Thresholds
//----------------------------------------------------------------------------

#define SET_PQDC_TH_AT_INIT 1

#define PQDC_DEF_TH_MV -80  // PQDC fast branch threshold in mV relative to baseline (signal AC coupled and amplified 20-30 times)
							 // neg signal >> negative value
							 // pos signal >> positive value

//----------------------------------------------------------------------------
// PADI discriminator settings (TAMEX-PADI1 & TAMEX2 with PADI add-on)
//----------------------------------------------------------------------------

#define SET_PADI_TH_AT_INIT 0 // Henning

#define PADI_DEF_TH 0xa000a000 // PADI thresholds set at startup 2x16 bits for PADI1/2

#define COMBINE_TRIG 1         // Must be 1 or 0:
							   // If 1, OR signals from both PADIs are combined to one OR_out signal

#define ENABLE_OR_TAM2 0       // Must be 1 or 0:
							   // If 1, NIM trigger signals are generated from PADI OR on TAMEX2

#define EN_TRIG_ASYNC 0 // Henning

//----------------------------------------------------------------------------
// TDC & Trigger settings
//----------------------------------------------------------------------------

#define EN_TDC_CH  0xFFFFFFFF // channel enable 32-1 (0xffffffff -> all channels enabled)

#define EN_TRIG_CH 0xFFFFFFFF // Trigger enable 32-1 (only on TAMEX-PADI1 so far)
							  // 0xffffffff -> all channels enabled for trigger output 0b1111
							  // 0xaaaaaaaa -> all slow channels enabled for trigger output 0b1010

#define DIS_TRIG_CH 0x00000000

#define TDC_CH_POL 0x00000000 // Input signal polarity (0: for negative, 1: for positive)

#define EN_REF_CH             // Measurement of AccTrig on CH0 - comment line to disable

//----------------------------------------------------------------------------
// Trigger Window
//----------------------------------------------------------------------------

#ifndef LONG_TRIGGER_WINDOW 
#define TRIG_WIN_EN 1           // 0 trigger window control is off,
#endif                           // everything will be written out
								 // with LONG_TRIGGER_WINDOW enabled trigger window control will be
								 // handled implicitly:
								 // OFF when PRE_TRIG_TIME and POST_TRIG_TIME are set to 0
								 // ON  when PRE_TRIG_TIME and POST_TRIG_TIME are set to not 0

#define PRE_TRIG_TIME     0x200  
// in nr of time slices a 5.00 ns: max 0x7ff := 2047 * 5.00 ns
#define POST_TRIG_TIME   0x300    
// in nr of time slices a 5.00 ns: max 0x7ff := 2047 * 5.00 ns
//when changing PRE/POST_TRIG_TIME, "TRIG_CVT" in "setup.usf" also should be modified. Talk to Jaehwan.

//----------------------------------------------------------------------------
// MBS settings
//----------------------------------------------------------------------------

#define STATISTIC   1000000

#define DEBUG 1

#define WAIT_FOR_DATA_READY_TOKEN 1    // - waits until data is ready before
									   //   sending data to PEXOR
									   // - otherwisse send data immediately
									   //   after token arrived at TAMEX

#define SEQUENTIAL_TOKEN_SEND 1        // - token sending and receiving is
									   //   sequential for all used SFPs
									   // - otherwise token sending and receiving
									   //   is done parallel for all used SFPs

//----------------------------------------------------------------------------
// --- END OF USER CHANGE AREA --- END OF USER CHANGE AREA ---
//----------------------------------------------------------------------------

#ifdef WR_TIME_STAMP
#define SUB_SYSTEM_ID      0x100
#define TS__ID_L16         0x3e1
#define TS__ID_M16         0x4e1
#define TS__ID_H16         0x5e1
#define TS__ID_X16         0x6e1

#define WR_DEVICE_NAME "dev/wbm0"     // pexaria5 pcie
#define WR_TLU_FIFO_NR       0        // pexaria5 pcie

#define PEXARIA_DEVICE_NAME "/dev/pcie_wb0"     // pexaria5 direct access handle
#endif // WR_TIME_STAMP

#ifdef SEQUENTIAL_TOKEN_SEND
#define DIRECT_DMA    1
#ifdef DIRECT_DMA
#define BURST_SIZE 128
#endif
#endif

#if defined (USE_MBSPEX_LIB) && ! defined (SEQUENTIAL_TOKEN_SEND)
#define USE_DRIVER_PARALLEL_TOKENREAD 1
#endif

#define PEXOR_PC_DRAM_DMA 1

#define USER_TRIG_CLEAR 1

#define CHECK_META_DATA 1

#ifdef CHECK_META_DATA
//#define CHECK_DATA_SIZE 1
#endif

//#define printm printf

#define MAX_TRIG_TYPE     16
#define MAX_SFP           4
#define MAX_SLAVE         16
#define GET_BAR0_BASE     0x1234
#define PEXDEV            "/dev/pexor"
#define PCI_BAR0_NAME     "PCI_BAR0_MBS"
#define PCI_BAR0_SIZE     0x800000  // in bytes
#define PEX_MEM_OFF       0x100000
#define PEX_REG_OFF       0x20000
#define PEX_SFP_OFF       0x40000
#define DATA_SEED         0x12CCE6F7
#define MAX_PAGE          10

#define REG_TAM_CTRL      0x200000
#define REG_TAM_TRG_WIN   0x200004
#define REG_TAM_EN_CH     0x200008
#define REG_TAM_EN_TR     0x33001C         // 0x20000c
#define REG_TAM_POLARITY  0x330018         // 0x200010
#define REG_TAM_MISC1     0x200014
#define REG_TAM_MISC2     0x200018
#define REG_TAM_MISC3     0x20001c
#define REG_TAM_CLK_SEL   0x311000
#define REG_TAM_BUS_EN    0x311008
#define REG_TAM_SPI_DAT   0x311018
#define REG_TAM_SPI_CTL   0x311014

#define RON  "\x1B[7m"
#define RES  "\x1B[0m"

/*****************************************************************************/

int  f_pex_slave_rd (long, long, long, long*);
int  f_pex_slave_wr (long, long, long,  long);
int  f_pex_slave_init (long, long);
#ifndef USE_MBSPEX_LIB
int  f_pex_send_and_receive_tok (long, long, long*, long*, long*);
int  f_pex_send_tok (long, long);
int  f_pex_receive_tok (long, long*, long*, long*);
#endif // USE_MBSPEX_LIB
void f_tam_init ();

#ifdef WR_TIME_STAMP
void f_wr_init ();
void f_wr_reset_tlu_fifo ();
#endif // WR_TIME_STAMP

static long          l_first = 0, l_first2 = 0, l_first3 = 0;
static unsigned long l_tr_ct[MAX_TRIG_TYPE];
static   INTU4    l_sfp_pat = 0;
static   INTS4    fd_pex;             // file descriptor for PEXOR device
static   INTS4    l_sfp_slaves  [MAX_SFP] = NR_SLAVES;
static   INTS4    l_sfp_tam_mode[MAX_SFP] = TAMEX_TM;

#ifndef Linux
static   INTS4    l_bar0_base;
#endif // Linux
static   INTU4  volatile *pl_virt_bar0;
static   s_pexor  sPEXOR;

static   int   l_i, l_j, l_k, l_p;
static  long  l_stat;
static  long  l_dat1, l_dat2, l_dat3;

#ifdef LONG_TRIGGER_WINDOW
static  long  l_trig_wind = (POST_TRIG_TIME << 16) + PRE_TRIG_TIME;
#else
static  long  l_trig_wind = (TRIG_WIN_EN << 31) + (POST_TRIG_TIME << 16) + PRE_TRIG_TIME;
#endif 

static  long  l_tog=1;   // start always with buffer 0 !!
static  long  l_tok_mode;
static  long  l_dummy;
static  long  l_tok_check;
static  long  l_n_slaves;
static  long  l_tam_head;
static  long  l_tdc_size;
static  long  l_tdc_head;
static  long  l_tdc_trail;
static  long  l_lec_check=0;
static  long  l_check_err=0;
static  long long l_err_prot_ct=0;
static  long  l_tam_init_ct=0;
static  long  l_tam_buf_off   [MAX_SFP][MAX_SLAVE][2];
static  long  l_tam_n_tdc    [MAX_SFP][MAX_SLAVE];
static  long  l_tam_tdc_off  [MAX_SFP][MAX_SLAVE];

static  long  l_trig_type;
static  long  l_sfp_id;
static  long  l_tam_id;
static  long  l_tdc_id;

static  INTU4 *pl_dat_save, *pl_tmp;
static  long  l_dat_len_sum[MAX_SFP];
static  long  l_dat_len_sum_long[MAX_SFP];
static  long  volatile *pl_pex_sfp_mem_base[MAX_SFP];

static  INTU4  volatile *pl_dma_source_base;
static  INTU4  volatile *pl_dma_target_base;
static  INTU4  volatile *pl_dma_trans_size;
static  INTU4  volatile *pl_dma_burst_size;
static  INTU4  volatile *pl_dma_stat;
static  long            l_dma_target_base;
static  long            l_dma_trans_size;
static  long            l_burst_size;
static  long            l_dat;
static  long            l_pex_sfp_phys_mem_base[MAX_SFP];
static  long            l_ct;
static  long            l_padd[MAX_SFP];
static struct dmachain *pl_page;
static  long l_diff_pipe_phys_virt;

static  long l_err_flg;
static  long l_i_err_flg   [MAX_SFP][MAX_SLAVE];

static  long l_tdc_head_lec_err=0;
static  long l_tdc_trail_lec_err=0;
static  long l_tam_trixor_trig_type_mism=0;
static  long l_tam_tdc_data_size_1_err=0;
static  long l_tam_tdc_data_size_3_err=0;

static  long l_fi_hw_trg=0;
static  long l_tdc_err_flg;


static  int  l_enable_or  = ENABLE_OR_TAM2  << 29;
static  int  l_combine_trig = COMBINE_TRIG << 28;

static  int  l_tam_fifo_almost_full_sh = FIFO_ALMOSTFULL_TH << 16; /// shizu 07.02.2021

static  int  l_en_async_trig = EN_TRIG_ASYNC << 29; // Henning

static int   l_initialize_tamex_modules = 1; // Henning

static float l_pqdc_threshold_rel = PQDC_DEF_TH_MV; // Henning

#ifdef WR_TIME_STAMP
static FILE* dactl_handle =0;
static int tlu_address = 0x4000100; // TLU base register to map into vme address space
									//static int tlu_direct_off = 0xFFFFFFFF; // -1, but better send decimal expression as string directly
#ifdef WR_USE_TLU_DIRECT
static   INTS4    fd_tlu;
static INTU4  volatile * p_TLU;

static INTS4* ch_select;
static INTS4* ch_fifosize;
static INTS4* fifoclear;
static INTS4* armset;

static INTS4* fifo_ready;
static INTS4* fifo_cnt;
static INTS4* ft_shi;
static INTS4* ft_slo;
static INTS4* ft_ssub;
static INTS4* fifo_pop;
#endif //WR_USE_TLU_DIRECT

static  long              l_eb_first1=0, l_eb_first2=0;
static  long              l_used_tlu_fifo = 1 << WR_TLU_FIFO_NR;
static  eb_status_t       eb_stat;
static  eb_device_t       eb_device;
static  eb_socket_t       eb_socket;
static  eb_cycle_t        eb_cycle;
static  struct sdb_device sdbDevice;
static  eb_address_t      wrTLU;
static  eb_address_t      wrTLUFIFO;
static  int               nDevices;

static  eb_data_t         eb_fifo_cha;
static  eb_data_t         eb_fifo_size;
static  eb_data_t         eb_tlu_high_ts; // high 32 bit 
static  eb_data_t         eb_tlu_low_ts;  // low  32 bit
static  eb_data_t         eb_tlu_fine_ts; // fine 3 bit  ! subject of change !
static  eb_data_t         eb_stat_before; // status before etherbone cycle
static  eb_data_t         eb_stat_after;  // status after  etherbone cycle
static  eb_data_t         eb_fifo_ct_brd; // TLU FIFO fill counter before TLU read
static  eb_data_t         eb_fifo_ct_ard; //TLU FIFO fill counter after TLU read

static  unsigned long long ll_ts_hi;
static  unsigned long long ll_ts_lo;
static  unsigned long long ll_ts_fi;
static  unsigned long long ll_ts;
static  unsigned long long ll_x16;
static  unsigned long long ll_h16;
static  unsigned long long ll_m16;
static  unsigned long long ll_l16;

static  unsigned long long ll_timestamp;
static  unsigned long long ll_actu_timestamp;
static  unsigned long long ll_prev_timestamp;
static  unsigned long long ll_diff_timestamp;

static INTU4  l_time_l16;
static INTU4  l_time_m16;
static INTU4  l_time_h16;
static INTU4  l_time_x16;

static long l_check_wr_err = 0;
static long l_wr_init_ct   = 0;
static long l_err_wr_ct    = 0;

#endif // WR_TIME_STAMP

//shizu
static long l_slave_stat            = 0;
static long l_slave_stat_prev[8]    = {0,0,0,0,0,0,0,0};
static long l_stat_stat             = 0;
static long l_dma_trans_size_prev   = 0;
static long bh_trig_typ_prev        = 0;

static  long  l_tam_rst_stat  [MAX_SFP][MAX_SLAVE];

#define IDATEN 1

#ifdef IDATEN

static unsigned long l_selftrig[7] = 
{
	0xaaaaaaaa^0x00000000 ,
	0xaaaaaaaa^0x00000000 ,
	0xaaaaaaaa^0x00000000 ,
	0xaaaaaaaa^(0x00000000) ,
	0xaaaaaaaa^(0x00000000 | 0x2<<(2*11)) ,
	0xaaaaaaaa^(0x00000000 | 0x2<<(2*10)) ,
	0xaaaaaaaa^(0x00000000 | 0x2<<(2* 9)) 
};
// use 0b1010 instead of 0b1111 to trigger only on slow channels
/*
static unsigned long l_selftrig[7] = 
{
	0xffffffff^0x00000000 ,
	0xffffffff^0x00000000 ,
	0xffffffff^0x00000000 ,
	0xffffffff^(0x00000000) ,
	0xffffffff^(0x00000000 | 0x3<<(2*11)) ,
	0xffffffff^(0x00000000 | 0x3<<(2*10)) ,
	0xffffffff^(0x00000000 | 0x3<<(2* 9)) 
};*/
#endif //IDATEN

#ifdef PATTERN_UNIT
static INTU4 l_pattern;
#endif //PATTERN_UNIT


/*****************************************************************************/

int f_user_get_virt_ptr (long  *pl_loc_hwacc, long  pl_rem_cam[])
{
#ifdef Linux
	int       prot;
	int       flags;
#endif
	INTS4     l_stat;

#ifdef WR_TIME_STAMP
	if (l_eb_first1 == 0)
	{
		l_eb_first1 = 1;

#ifdef WR_USE_TLU_DIRECT

		printm ("f_user_get_virt_ptr switches PEXARIA to DIRECT ACCESS TLU mode... \n");

		dactl_handle= fopen ("/sys/class/pcie_wb/pcie_wb0/dactl", "a");
		if(dactl_handle==NULL)
		{
			printm ("!!! Could not open dactl control sysfs handle!!! \n");
			exit(-1); // probably wrong driver?
		}
		else
		{
			fprintf(dactl_handle,"%d",tlu_address);
			fclose(dactl_handle);
			//sleep(1);
		}

		printm ("f_user_get_virt_ptr for DIRECT TLU \n");

		if ((fd_tlu = open (PEXARIA_DEVICE_NAME, O_RDWR)) == -1)
		{
			printm (RON"ERROR>>"RES" could not open %s device \n", PEXARIA_DEVICE_NAME);
			exit (0);
		}
		else
		{
			printm ("opened device: %s, fd = %d \n", PEXARIA_DEVICE_NAME, fd_tlu);
		}

		// map bar1 directly via pcie_wb driver and access TLU registers:
		prot  = PROT_WRITE | PROT_READ;
		flags = MAP_SHARED | MAP_LOCKED;
		if ((p_TLU = (INTU4*) mmap (NULL, 0x100, prot, flags, fd_tlu, 0)) == MAP_FAILED)
		{
			printm (RON"failed to mmap bar1 from pexaria"RES", return: 0x%x, %d \n", p_TLU, p_TLU);
			perror ("mmap");
			exit (-1);
		}

		// used in init function
		ch_select           = (INTS4*) ((char*)(p_TLU)      + GSI_TM_LATCH_CH_SELECT);
		ch_fifosize         = (INTS4*) ((char*)(p_TLU)      + GSI_TM_LATCH_CHNS_FIFOSIZE);
		fifoclear           = (INTS4*) ((char*)(p_TLU)      + GSI_TM_LATCH_FIFO_CLEAR);
		armset              = (INTS4*) ((char*)(p_TLU)      + GSI_TM_LATCH_TRIG_ARMSET);

		// set here pointers to mapped registers used in readout function:
		fifo_ready          = (INTS4*) ((char*)(p_TLU)      + GSI_TM_LATCH_FIFO_READY);
		fifo_cnt            = (INTS4*) ((char*)(p_TLU)      + GSI_TM_LATCH_FIFO_CNT);
		ft_shi              = (INTS4*) ((char*)(p_TLU)      + GSI_TM_LATCH_FIFO_FTSHI);
		ft_slo              = (INTS4*) ((char*)(p_TLU)      + GSI_TM_LATCH_FIFO_FTSLO);
		ft_ssub		= (INTS4*) ((char*)(p_TLU)      + GSI_TM_LATCH_FIFO_FTSSUB);
		fifo_pop		= (INTS4*) ((char*)(p_TLU)      + GSI_TM_LATCH_FIFO_POP);

#else  // WR_USE_TLU_DIRECT

		////////// JAM comment the following block out to check for old pcie_wb.ko driver
		printm ("f_user_get_virt_ptr  switches PEXARIA to etherbone access mode... \n");
		dactl_handle= fopen ("/sys/class/pcie_wb/pcie_wb0/dactl", "a");
		if(dactl_handle==NULL)
		{
			printm ("!!! Could not open dactl control sysfs handle!!! \n");
			exit(-1); // probably wrong driver?
		}
		else
		{
			//fprintf(dactl_handle,"%d",tlu_direct_off);
			fprintf(dactl_handle,"4294967295"); // -1 or hex 0xffffffff
			fclose(dactl_handle);
			//sleep(1);
		}
		///////////////////////

		if ((eb_stat = eb_socket_open(EB_ABI_CODE, 0, EB_ADDR32|EB_DATA32, &eb_socket)) != EB_OK)
		{
			printm (RON"ERROR>>"RES" etherbone eb_open_socket, status: %s \n", eb_status(eb_stat));
		}

		if ((eb_stat = eb_device_open(eb_socket, WR_DEVICE_NAME, EB_ADDR32|EB_DATA32, 3, &eb_device)) != EB_OK)
		{
			printm (RON"ERROR>>"RES" etherbone eb_device_open, status: %s \n", eb_status(eb_stat));
		}

		nDevices = 1;
		if ((eb_stat = eb_sdb_find_by_identity(eb_device, GSI_TM_LATCH_VENDOR,
						GSI_TM_LATCH_PRODUCT, &sdbDevice, &nDevices)) != EB_OK)
		{
			printm (RON"ERROR>>"RES" etherbone TLU eb_sdb_find_by_identity, status: %s \n", eb_status(eb_stat));
		}

		if (nDevices == 0)
		{
			printm (RON"ERROR>>"RES" etherbone no TLU found, status: %s \n", eb_status(eb_stat));
		}

		if (nDevices > 1)
		{
			printm (RON"ERROR>>"RES" etherbone too many TLUsfound, status: %s \n", eb_status(eb_stat));
		}

		/* Record the address of the device */
		wrTLU     = sdbDevice.sdb_component.addr_first;
		//wrTLUFIFO = wrTLU + GSI_TM_LATCH_FIFO_OFFSET + WR_TLU_FIFO_NR * GSI_TM_LATCH_FIFO_INCR;

#endif // TLU direct

	} // ebfirst
#endif // WR_TIME_STAMP

#ifdef USE_MBSPEX_LIB
	if (l_first2 == 0)
	{
		l_first2 = 1;

		if ((fd_pex = mbspex_open (0)) == -1)
		{
			printm (RON"ERROR>>"RES" could not open mbspex device \n");
			exit (0);
		}
		for (l_i=0; l_i<MAX_SFP; l_i++)
		{
			if (l_sfp_slaves[l_i] != 0)
			{
				l_sfp_pat |= (1<<l_i);
				l_pex_sfp_phys_mem_base[l_i] = (long)PEX_MEM_OFF + (long)(PEX_SFP_OFF * l_i);
			}
		}
		printm ("sfp pattern: 0x%x \n", l_sfp_pat);
		printm ("SFP id: %d, Pexor SFP physical memory base: 0x%8x \n",
				l_i, l_pex_sfp_phys_mem_base[l_i]);

	} // if (l_first2 == 0) // if defined USE_MBSPEX_LIB

#else // USE_MBSPEX_LIB

	if (l_first2 == 0)
	{
		l_first2 = 1;

		pl_page = (struct dmachain*) malloc (sizeof(struct dmachain*) * MAX_PAGE);

		if ((fd_pex = open (PEXDEV, O_RDWR)) == -1)
		{
			printm (RON"ERROR>>"RES" could not open %s device \n", PEXDEV);
			exit (0);
		}
		else
		{
			printm ("opened device: %s, fd = %d \n", PEXDEV, fd_pex);
		}

#ifdef Linux
		// map bar0 directly via pexor driver and access trixor base
		prot  = PROT_WRITE | PROT_READ;
		flags = MAP_SHARED | MAP_LOCKED;
		if ((pl_virt_bar0 = (INTU4 *) mmap (NULL, PCI_BAR0_SIZE, prot, flags, fd_pex, 0)) == MAP_FAILED)
		{
			printm (RON"failed to mmap bar0 from pexor"RES", return: 0x%x, %d \n", pl_virt_bar0, pl_virt_bar0);
			perror ("mmap");
			exit (-1);
		}
#ifdef DEBUG
		printm ("first mapped virtual address of bar0: 0x%p \n", pl_virt_bar0);
#endif // DEBUG

#else // Linux

		// get bar0 base:
		l_stat = ioctl (fd_pex, GET_BAR0_BASE, &l_bar0_base);
		if (l_stat == -1 )
		{
			printm (RON"ERROR>>"RES" ioctl GET_BAR0_BASE failed \n");
		}
		else
		{
			printm ("PEXOR bar0 base: 0x%x \n", l_bar0_base);
		}
		// open shared segment
		smem_remove(PCI_BAR0_NAME);
		if((pl_virt_bar0 = (INTU4 *) smem_create (PCI_BAR0_NAME,
						(char*) l_bar0_base, PCI_BAR0_SIZE, SM_READ | SM_WRITE))==NULL)
		{
			printm ("smem_create for PEXOR BAR0 failed");
			exit (-1);
		}
#endif // Linux

		// close pexor device
		l_stat = close (fd_pex);
		if (l_stat == -1 )
		{
			printm (RON"ERROR>>"RES" could not close PEXOR device \n");
		}

		for (l_i=0; l_i<MAX_SFP; l_i++)
		{
			if (l_sfp_slaves[l_i] != 0)
			{
				pl_pex_sfp_mem_base[l_i] = (INTU4 volatile*)
					((long)pl_virt_bar0 + (long)PEX_MEM_OFF + (long)(PEX_SFP_OFF * l_i));   
				l_pex_sfp_phys_mem_base[l_i] = (long)PEX_MEM_OFF + (long)(PEX_SFP_OFF * l_i);

				pl_dma_source_base = (INTU4 volatile*)((long)pl_virt_bar0 + (long)PEX_REG_OFF + (long) 0x0 );
				pl_dma_target_base = (INTU4 volatile*)((long)pl_virt_bar0 + (long)PEX_REG_OFF + (long) 0x4 );
				pl_dma_trans_size  = (INTU4 volatile*)((long)pl_virt_bar0 + (long)PEX_REG_OFF + (long) 0x8 );
				pl_dma_burst_size  = (INTU4 volatile*)((long)pl_virt_bar0 + (long)PEX_REG_OFF + (long) 0xc );
				pl_dma_stat        = (INTU4 volatile*)((long)pl_virt_bar0 + (long)PEX_REG_OFF + (long) 0x10);

				l_sfp_pat |= (1<<l_i);
			}
		}
		printm ("sfp pattern: 0x%x \n", l_sfp_pat);
	} // if (l_first2 == 0) // if NOT defined USE_MBSPEX_LIB
	printm ("pl_virt_bar0: 0x%p \n", pl_virt_bar0);
	for (l_i=0; l_i<MAX_SFP; l_i++)
	{
		if (l_sfp_slaves[l_i] != 0)
		{
			printm ("SFP id: %d, Pexor SFP virtual memory base: 0x%8x \n",
					l_i, pl_pex_sfp_mem_base[l_i]);
			printm ("                     physical:            0x%8x \n",
					l_pex_sfp_phys_mem_base[l_i]);
		}
	}
#endif // (else) USE_MBSPEX_LIB
	return (0);
}


/*****************************************************************************/

int f_user_init (unsigned char   bh_crate_nr,
		long           *pl_loc_hwacc,
		long           *pl_rem_cam,
		long           *pl_stat)

{
#ifdef WR_TIME_STAMP
	f_wr_init ();
#endif

#ifdef WAIT_FOR_DATA_READY_TOKEN
	l_tok_mode = 2;    // TAMEX wait for data ready
#else
	l_tok_mode = 0;    // TAMEX send data after token reception
#endif

#ifndef USE_MBSPEX_LIB
	PEXOR_GetPointer(0, pl_virt_bar0, &sPEXOR);
#ifdef USE_KINPEX_V5
	*pl_dma_stat = 0x20; // activates dma start by writing source base
#endif // USE_KINPEX_V5
#endif

#ifdef PATTERN_UNIT
	printm ("enable pattern unit SFP 3, FEBEX 0 \n");
	//system ("/mbs/driv/mbspex_4.9-64_DEB/bin/gosipcmd -i 3 1");
	l_stat = f_pex_slave_wr (3, 0, 0x2080c0, 0x40);
	if (l_stat == -1)
	{
		printm (RON"ERROR>>"RES" writing to 0x2080c0 (enable pattern unit) failed \n");
		l_err_prot_ct++;
	}
	//sleep (1);   
	l_stat = f_pex_slave_rd (3, 0, 0x2080c0, &l_pattern);
	if (l_pattern == 0x40)
	{
		printm ("pattern unit is enabled \n");
	}
	else
	{
		printm (RON"ERROR>>"RES" pattern unit failed to enable, exiting.. \n");
		exit (0);
	} 
#endif // PATTERN_UNIT

	f_tam_init();
	l_tog = 1;
	l_lec_check = 0;

	return (1);
}

/*****************************************************************************/

int f_user_readout (unsigned char   bh_trig_typ,
		unsigned char   bh_crate_nr,
		register long  *pl_loc_hwacc,
		register long  *pl_rem_cam,
		//                  long           *pl_dat,
		long           *pl_dat_long,
		s_veshe        *ps_veshe,
		long           *l_se_read_len,
		long           *l_read_stat)
{
	INTU4* pl_dat = (INTU4*) pl_dat_long;

	*l_se_read_len = 0;
	pl_dat_save = pl_dat;

	l_tr_ct[0]++;            // event/trigger counter
	l_tr_ct[bh_trig_typ]++;  // individual trigger counter
							 //printm ("trigger no: %d \n", l_tr_ct[0]);

#ifdef WR_TIME_STAMP
	if (l_check_wr_err == 1)
	{
		printm ("reset TLU fifo of white rabbit time stamp module pexaria \n");
		f_wr_reset_tlu_fifo ();
		l_wr_init_ct++;
		*l_read_stat = 0;
		sleep (1);
		goto bad_event;
	}
#endif //WR_TIME_STAMP

#ifdef CHECK_META_DATA
	if (l_check_err == 1)
	{
#ifdef WR_TIME_STAMP
		printm ("");
		printm ("reset TLU fifo of white rabbit time stamp module pexaria \n");
		f_wr_reset_tlu_fifo ();
		l_wr_init_ct++;
#endif //WR_TIME_STAMP

		printm ("");
		printm ("re-initialize all TAMEX modules \n");
		//l_check_err--;
		f_tam_init ();
		l_tam_init_ct++;
		l_tog = 1;

		l_lec_check =  0;
		*l_read_stat = 0;
		//sleep (1);
		goto bad_event;
	}
#endif // CHECK_META_DATA

#ifdef PATTERN_UNIT
	// read pattern unit before user trigger clear (no double buffer)
	// but place pattern data behind trigger window meta data

	l_stat = f_pex_slave_rd (3, 0, 0x208040, &l_pattern);
	if (l_stat == -1)
	{
		printm (RON"ERROR>>"RES" reading pattern unit failed \n");
		l_err_prot_ct++;
	}
#endif // PATTERN_UNIT

	// think about if and where you shall do this ....
	*l_read_stat = 0;
#ifdef USER_TRIG_CLEAR
	if (bh_trig_typ < 14)
	{
		*l_read_stat = TRIG__CLEARED;
		f_user_trig_clear (bh_trig_typ);
	}
#endif // USER_TRIG_CLEAR

#ifdef WR_TIME_STAMP
	if (bh_trig_typ < 14)
	{
		*pl_dat++ = SUB_SYSTEM_ID;  //*l_se_read_len =+ 4;

#ifdef WR_USE_TLU_DIRECT
		eb_stat_before= *fifo_ready;
		eb_fifo_ct_brd= *fifo_cnt;
		*fifo_pop=0xF;
		SERIALIZE_IO;
		eb_tlu_high_ts = (*ft_shi)  & 0xFFFFFFFF;
		eb_tlu_low_ts  = (*ft_slo)  & 0xFFFFFFFF;
		eb_tlu_fine_ts = (*ft_ssub) & 0xFFFFFFFF;
		eb_stat_after  =  *fifo_ready;
#else // WR_USE_TLU_DIRECT

		if ((eb_stat = eb_cycle_open(eb_device, 0, eb_block, &eb_cycle)) != EB_OK)
		{
			printm (RON"ERROR>>"RES" etherbone EP eb_cycle_open, status: %s \n", eb_status(eb_stat));
			//l_err_wr_ct++;
			//l_check_wr_err = 2; goto bad_event;
		}

		/* Queueing operations to a cycle never fails (EB_OOM is reported later) */
		/* read status of FIFOs */
		eb_cycle_read (eb_cycle, wrTLU + GSI_TM_LATCH_FIFO_READY, EB_BIG_ENDIAN|EB_DATA32, &eb_stat_before);
		/* read fifo fill counter */
		eb_cycle_read (eb_cycle, wrTLU + GSI_TM_LATCH_FIFO_CNT, EB_BIG_ENDIAN|EB_DATA32, &eb_fifo_ct_brd);
		/* read high word of latched timestamp */
		eb_cycle_read (eb_cycle, wrTLU + GSI_TM_LATCH_FIFO_FTSHI, EB_BIG_ENDIAN|EB_DATA32, &eb_tlu_high_ts);
		/* read low word of latched timestamp */
		eb_cycle_read (eb_cycle, wrTLU + GSI_TM_LATCH_FIFO_FTSLO, EB_BIG_ENDIAN|EB_DATA32, &eb_tlu_low_ts);
		/* read fine time word of latched timestamp */
		eb_cycle_read (eb_cycle, wrTLU + GSI_TM_LATCH_FIFO_FTSSUB, EB_BIG_ENDIAN|EB_DATA32, &eb_tlu_fine_ts);
		/* pop timestamp from FIFO */
		eb_cycle_write (eb_cycle, wrTLU + GSI_TM_LATCH_FIFO_POP, EB_BIG_ENDIAN|EB_DATA32, 0xF);
		/* read status of FIFOs */
		eb_cycle_read (eb_cycle, wrTLU + GSI_TM_LATCH_FIFO_READY, EB_BIG_ENDIAN|EB_DATA32, &eb_stat_after);

		/* Because the cycle was opened with eb_block, this is a blocking call.
		 * Upon termination, data and eb_tlu_high_ts will be valid.
		 * For higher performance, use multiple asynchronous cycles in a pipeline.
		 */

		if ((eb_stat = eb_cycle_close(eb_cycle)) != EB_OK)
		{
			printm (RON"ERROR>>"RES" etherbone EP eb_cycle_close, status: %s \n", eb_status(eb_stat));
			//l_err_wr_ct++;
			//l_check_wr_err = 2; goto bad_event;
		}

#endif // WR_USE_TLU_DIRECT

		if ((eb_stat_before & l_used_tlu_fifo) != l_used_tlu_fifo)
		{
			usleep (100000);
			printm (RON"ERROR>>"RES" TLU fifo %d is empty before time stamp read, stat: 0x%x\n", WR_TLU_FIFO_NR, eb_stat_before);
			l_err_wr_ct++;
			l_check_wr_err = 2; goto bad_event;
		}
		if ((eb_stat_after & l_used_tlu_fifo) != 0)
		{
			//printm (RON"ERROR>>"RES" TLU fifo %d is not empty after time stamp read, stat: 0x%x\n", WR_TLU_FIFO_NR, eb_stat_after);
		}

#ifdef USER_TRIG_CLEAR
		if (eb_fifo_ct_brd > 2)
		{
			printm (RON"ERROR>>"RES" TLU fill count: %d is bigger than 2\n", eb_fifo_ct_brd);
			l_err_wr_ct++;
			l_check_wr_err = 2; goto bad_event;
		}
#else
		if (eb_fifo_ct_brd > 1)
		{
			printm (RON"ERROR>>"RES" TLU fill count: %d is bigger than 1\n", eb_fifo_ct_brd);
			l_err_wr_ct++;
			l_check_wr_err = 2; goto bad_event;
		}
#endif // USER_TRIG_CLEAR

		// eb_tlu_low_ts   represents 8 ns in the least significant bit (125 mhz)
		// eb_tlu_fine_ts  represents 1 ns in the least significant bit (subject of change)
		// if 1 ns granualrity is required for time sorting USE_TLU_FINE_TIME must be defined 

#ifdef USE_TLU_FINE_TIME
		ll_ts_hi = (unsigned long long) eb_tlu_high_ts;
		ll_ts_lo = (unsigned long long) eb_tlu_low_ts;
		ll_ts_fi = (unsigned long long) eb_tlu_fine_ts;
		//ll_ts_fi = 0;

		ll_ts_hi = ll_ts_hi << 35;
		ll_ts_lo = ll_ts_lo <<  3;
		ll_ts_fi = ll_ts_fi & 0x7;
		ll_ts    = ll_ts_hi + ll_ts_lo + ll_ts_fi;
		ll_timestamp = ll_ts;

		ll_l16   = (ll_ts >>  0) & 0xffff;
		ll_m16   = (ll_ts >> 16) & 0xffff;
		ll_h16   = (ll_ts >> 32) & 0xffff;
		ll_x16   = (ll_ts >> 48) & 0xffff;

		l_time_l16 = (unsigned long) (ll_l16);
		l_time_m16 = (unsigned long) (ll_m16);
		l_time_h16 = (unsigned long) (ll_h16);
		l_time_x16 = (unsigned long) (ll_x16);

		l_time_l16 = (l_time_l16 & 0xffff) + (TS__ID_L16 << 16);
		l_time_m16 = (l_time_m16 & 0xffff) + (TS__ID_M16 << 16);
		l_time_h16 = (l_time_h16 & 0xffff) + (TS__ID_H16 << 16);
		l_time_x16 = (l_time_x16 & 0xffff) + (TS__ID_X16 << 16);

		*pl_dat++ = l_time_l16;
		*pl_dat++ = l_time_m16;
		*pl_dat++ = l_time_h16;
		*pl_dat++ = l_time_x16;
		//*l_se_read_len += 16;

#else // USE_TLU_FINE_TIME

		l_time_l16 = (eb_tlu_low_ts  >>  0) & 0xffff;
		l_time_m16 = (eb_tlu_low_ts  >> 16) & 0xffff;
		l_time_h16 = (eb_tlu_high_ts >>  0) & 0xffff;
		l_time_x16 = (eb_tlu_high_ts >> 16) & 0xffff;

		l_time_l16 = (l_time_l16 & 0xffff) + (TS__ID_L16 << 16);
		l_time_m16 = (l_time_m16 & 0xffff) + (TS__ID_M16 << 16);
		l_time_h16 = (l_time_h16 & 0xffff) + (TS__ID_H16 << 16);
		l_time_x16 = (l_time_x16 & 0xffff) + (TS__ID_X16 << 16);

		*pl_dat++ = l_time_l16;
		*pl_dat++ = l_time_m16;
		*pl_dat++ = l_time_h16;
		*pl_dat++ = l_time_x16;
		//*l_se_read_len += 16;

		ll_timestamp = (unsigned long long)eb_tlu_high_ts;
		ll_timestamp = (ll_timestamp << 32);
		ll_timestamp = ll_timestamp + (unsigned long long)eb_tlu_low_ts;
#endif // USE_TLU_FINE_TIME

		ll_actu_timestamp = ll_timestamp;
		ll_diff_timestamp = ll_actu_timestamp - ll_prev_timestamp;

		ll_prev_timestamp = ll_actu_timestamp;
	}
#endif // WR_TIME_STAMP

	// store trigger window in each event
	*pl_dat++ = (((PRE_TRIG_TIME) << 16) + (POST_TRIG_TIME));

#ifdef PATTERN_UNIT
	*pl_dat++ = 0xabcd1234;  
	*pl_dat++ = l_pattern;
	*pl_dat++ = 0x4567abcd;
#endif // PATTER_UNIT

	switch (bh_trig_typ)
	{
		case 1:
		case 2:
		case 3:

			if (l_tog == 1) { l_tog = 0; } else { l_tog = 1; }

			//#ifdef  WAIT_FOR_DATA_READY_TOKEN
#if defined (WAIT_FOR_DATA_READY_TOKEN) && ! (SEQUENTIAL_TOKEN_SEND) && ! defined (USE_DRIVER_PARALLEL_TOKENREAD)
			//printm ("send token in WAIT_FOR_DATA_READY_TOKEN mode \n");
			//printm ("l_tog | l_tok_mode: 0x%x \n", l_tog | l_tok_mode);
			//sleep (1);
#ifdef USE_MBSPEX_LIB
			l_stat =  mbspex_send_tok (fd_pex, l_sfp_pat,  l_tog | l_tok_mode);
#else
			l_stat = f_pex_send_tok (l_sfp_pat, l_tog | l_tok_mode);
#endif // USE_MBSPEX_LIB
#endif // (WAIT_FOR_DATA_READY_TOKEN) && ! (SEQUENTIAL_TOKEN_SEND) && ! defined(USE_DRIVER_PARALLEL_TOKENREAD) 

			//printm ("l_tog: %d \n", l_tog);
			l_lec_check++;
			//sleep (1);

			if (l_first3 == 0)
			{
				l_first3 = 1;
#ifndef Linux
				sleep (1);
				if ((vmtopm (getpid(), pl_page, (char*) pl_dat,
								(long)100 *sizeof(long))) == -1)
				{
					printm  (RON"ERROR>>"RES" calling vmtopm, exiting..\n");
					exit (0);
				}

				// get physical - and virtual pipe base
				// pipe is consecutive memory => const difference physical - virtual
				printm ("pl_dat: 0x%x, pl_dat_phys: 0x%x \n", pl_dat_save, pl_page->address);
				l_diff_pipe_phys_virt = (long)pl_page->address - (long)pl_dat;
#else
				l_diff_pipe_phys_virt = (long)pl_rem_cam;
#endif // Linux
				printm ("diff pipe base phys-virt: 0x%x \n", l_diff_pipe_phys_virt);

				printm ("\n");

				printm ("trigger window: before trigger: %4d ns \n", PRE_TRIG_TIME  * 5);
				printm ("                after  trigger: %4d ns \n", POST_TRIG_TIME * 5);
				printm ("                total:        : %4d ns \n", (PRE_TRIG_TIME + POST_TRIG_TIME) * 5);
			}

			// prepare token data sending
			if ((bh_trig_typ != 14) && (bh_trig_typ != 15))
			{
#if  defined (USE_DRIVER_PARALLEL_TOKENREAD) &&  ! (SEQUENTIAL_TOKEN_SEND)
				l_dma_target_base = (long) pl_dat + l_diff_pipe_phys_virt;

				l_stat=mbspex_send_and_receive_parallel_tok (fd_pex, l_sfp_pat, l_tog | l_tok_mode,
						(long) l_dma_target_base, (long unsigned*) &l_dma_trans_size, &l_dummy, &l_tok_check, &l_n_slaves);
				if (l_stat !=0)
				{
					printm (RON"ERROR>>"RES" mbspex_send_and_receive_parallel_tok to slave(s) / SFPs failed\n");
					l_err_prot_ct++;
					l_check_err = 2; goto bad_event;
				}

				// check here the case of all zero suppressed data, lenght should be below 4 words
				if(l_dma_trans_size <=16)
				{
					//printm("mbspex_send_and_receive_parallel_tok sees l_dma_trans_size , 0x%x \n", l_dma_trans_size);
					// we just discard the extra padding words send by the kernel module DMA
					l_dma_trans_size=0;
				}
				pl_dat += (l_dma_trans_size>>2); // l_dma_trans_size bytes to pointer units - int

#else // (USE_DRIVER_PARALLEL_TOKENREAD) &&  ! (SEQUENTIAL_TOKEN_SEND)

#ifdef SEQUENTIAL_TOKEN_SEND
				for (l_i=0; l_i<MAX_SFP; l_i++)
				{
					if (l_sfp_slaves[l_i] != 0)
					{

#ifdef DIRECT_DMA

						l_burst_size = BURST_SIZE;
						// target address is (must be) adjusted to burst size !
						l_padd[l_i] = 0;
						if ( ((long)pl_dat % l_burst_size) != 0)
						{
							l_padd[l_i] = l_burst_size - ((long)pl_dat % l_burst_size);
							l_dma_target_base = (long) pl_dat + l_diff_pipe_phys_virt + l_padd[l_i];
						}
						else
						{
							l_dma_target_base = (long) pl_dat + l_diff_pipe_phys_virt;
						}

#ifndef USE_MBSPEX_LIB
						// select SFP for PCI Express DMA
						*pl_dma_stat = 1 << (l_i+1);
#endif // USE_MBSPEX_LIB

#endif //DIRECT_DMA

#ifdef USE_MBSPEX_LIB

						// shizu
						l_stat = mbspex_send_and_receive_tok (fd_pex, l_i, l_tog | l_tok_mode,
								(long) l_dma_target_base, (long unsigned*) &l_dma_trans_size,
								&l_dummy, &l_tok_check, &l_n_slaves);

						// try to check if we have zero suppression.
						if(l_dma_trans_size<=16) // minimum kernel module automatic burst size?
						{
							//printm("mbspex_send_and_receive_tok for sfp %d has l_dma_trans_size=%d\n",l_i,l_dma_trans_size );
							l_dma_trans_size=0; // frontend zero suppression: skip filling up with padding words.
						}
						mbspex_register_wr(fd_pex,0, (long)PEX_REG_OFF + (long) 0x10, 0); // reset dma for zero suppression mode
						mbspex_register_wr(fd_pex,0,(long)PEX_REG_OFF + (long) 0x8,0); // reset previous dma length, for zero supression
#else
						*pl_dma_target_base = l_dma_target_base;
						l_stat = f_pex_send_and_receive_tok (l_i, l_tog | l_tok_mode, &l_dummy, &l_tok_check, &l_n_slaves);
#endif // USE_MBSPEX_LIB

						if (l_stat == -1)
						{
							printm (RON"ERROR>>"RES" PEXOR send token to slave(s) / SFPs failed\n");
							//exit(1);
							l_err_prot_ct++;
							l_check_err = 2; goto bad_event;
						}

						if ((l_tok_check & 0x1) != l_tog)
						{
							printm (RON"ERROR>>"RES" double buffer toggle bit differs from token return toggle bit \n");
							l_err_prot_ct++;
							l_check_err = 2; goto bad_event;
							//printm ("exiting..\n"); exit (0);
						}
						if ((l_tok_check & 0x2) != l_tok_mode)
						{
							printm (RON"ERROR>>"RES" token mode differs from token return token mode bit \n");
							l_err_prot_ct++;
							l_check_err = 2; goto bad_event;
							//printm ("exiting..\n"); exit (0);
						}

#ifdef USE_KINPEX_V5
						if (l_n_slaves != (l_sfp_slaves[l_i] & 0xf))
#else
							if (l_n_slaves != l_sfp_slaves[l_i])
#endif
							{
								printm (RON"ERROR>>"RES" nr. of slaves specified: %d differ from token return: %d \n",
										l_sfp_slaves[l_i], l_n_slaves);
								l_err_prot_ct++;
								l_check_err = 2; goto bad_event;
								//printm ("exiting..\n"); exit (0);
							}

#ifdef DIRECT_DMA

#ifndef USE_MBSPEX_LIB
						// mbspex lib does this internally, dma is finished when call mbspex_send_and_receive_tok returns

						l_ct = 0;
						while (1)    // check if dma transfer finished
						{
							// l_dat1 = l_dat;
							l_dat = *pl_dma_stat;
							//            printm ("status: %d \n", l_dat); sleep (1);
							//printm ("bursts: %d \n", l_burst_size);
							if (l_dat == 0xffffffff)
							{
								printm (RON"ERROR>>"RES" PCIe bus errror, check again\n" );
								l_dat = *pl_dma_stat;
								if (l_dat == 0xffffffff)
								{
									printm (RON"ERROR>>"RES" PCIe bus errror, exiting.. \n" );
									exit (0);
								}
							}
							else if ((l_dat & 0x1)  == 0)
							{
								break; // dma shall be finished
							}
							l_ct++;
							if ( (l_ct % 1000000) == 0)
							{
								printm ("DMA not ready after %d queries on SFP %d: l_dat: %d \n", l_ct, l_i, l_dat);  
								sleep (1);
							}
#ifndef Linux
							yield ();
#else
							sched_yield ();
#endif
						}
						l_dma_trans_size = *pl_dma_trans_size; // in this case true, not BURST_SIZE aligned size

						*pl_dma_stat = 0; // shizu de-activate dma
						*pl_dma_trans_size=0; // shizu debugging.... set data size 0
#endif // not USE_MBSPEX_LIB

						//if(l_dma_trans_size % 8 != 0) {printm ("dma data size  0x%x\n", l_dma_trans_size);}
						// adjust pl_dat, pl_dat comes always 4 byte aligned
						// fill padding space with pattern

						if(l_dma_trans_size == 0) // zero suppression //shizu
						{
							l_padd[l_i]= 0;
						}
						else
						{
							l_padd[l_i] = l_padd[l_i] >> 2;                  // now in 4 bytes (longs)
						}

						for (l_k=0; l_k<l_padd[l_i]; l_k++)
						{
							//*pl_dat++ = 0xadd00000 + (l_i*0x1000) + l_k;
							*pl_dat++ = 0xadd00000 + (l_padd[l_i]<<8) + l_k;
						}
						// increment pl_dat with true transfer size (not dma transfer size)
						// true transfer size expected and must be 4 bytes aligned
						pl_dat += l_dma_trans_size>>2;
#ifndef Linux
						yield ();
#else
						sched_yield ();
#endif // Linux
#endif // DIRECT_DMA
					}
				}
				// end SEQUENTIAL_TOKEN_SEND
#else
				// begin parallel token sending

				// send token to all SFPs used
#ifndef WAIT_FOR_DATA_READY_TOKEN
				//printm ("send token in NOT WAIT_FOR_DATA_READY_TOKEN mode \n");
				//printm ("l_tog | l_tok_mode: 0x%x \n", l_tog | l_tok_mode);
				//sleep (1);

#ifdef USE_MBSPEX_LIB
				l_stat =  mbspex_send_tok (fd_pex, l_sfp_pat,  l_tog | l_tok_mode);
#else
				l_stat = f_pex_send_tok (l_sfp_pat, l_tog | l_tok_mode);
#endif // USE_MBSPEX_LIB
#endif // (ifndef) WAIT_FOR_DATA_READY_TOKEN

				for (l_i=0; l_i<MAX_SFP; l_i++)
				{
					if (l_sfp_slaves[l_i] != 0)
					{
						// wait until token of all used SFPs returned successfully
#ifdef USE_MBSPEX_LIB
						l_dma_target_base = 0; // disable automatic internal dma,
											   // we do it manually with burst adjustment later!
						l_stat = mbspex_receive_tok (fd_pex, l_i, l_dma_target_base, (long unsigned*) &l_dma_trans_size,
								&l_dummy, &l_tok_check, &l_n_slaves);
#else
						l_stat = f_pex_receive_tok (l_i, &l_dummy, &l_tok_check, &l_n_slaves);
#endif // USE_MBSPEX_LIB

						if (l_stat == -1)
						{
							printm (RON"ERROR>>"RES" PEXOR receive token from SFP %d failed\n", l_i);
							l_err_prot_ct++;
							l_check_err = 2; goto bad_event;
						}

						if ((l_tok_check & 0x1) != l_tog)
						{
							printm (RON"ERROR>>"RES" double buffer toggle bit differs from token return toggle bit \n");
							l_err_prot_ct++;
							l_check_err = 2; goto bad_event;
							//printm ("exiting..\n"); exit (0);
						}
						if ((l_tok_check & 0x2) != l_tok_mode)
						{
							printm (RON"ERROR>>"RES" token mode bit differs from token return token mode bit \n");
							l_err_prot_ct++;
							l_check_err = 2; goto bad_event;
							//printm ("exiting..\n"); exit (0);
						}

#ifdef USE_KINPEX_V5
						if (l_n_slaves != (l_sfp_slaves[l_i] & 0xf))
#else
							if (l_n_slaves != l_sfp_slaves[l_i])
#endif
							{
								printm (RON"ERROR>>"RES" nr. of slaves specified: %d differ from token return: %d \n",
										l_sfp_slaves[l_i], l_n_slaves);
								l_err_prot_ct++;
								l_check_err = 2; goto bad_event;
								//printm ("exiting..\n"); exit (0);
							}
					}
				}
#endif // else SEQUENTIAL_TOKEN_SEND := parallel token send

#ifndef DIRECT_DMA
				// read tamex data (sent by token mode to the pexor)
				// from pexor the pexor memory
				for (l_i=0; l_i<MAX_SFP; l_i++)
				{
					if (l_sfp_slaves[l_i] != 0)
					{
#ifdef USE_MBSPEX_LIB
						l_dat_len_sum[l_i] = mbspex_get_tok_memsize(fd_pex, l_i); // in bytes
																				  //if(l_dat_len_sum[l_i]<10)
																				  //printm("l_dat_len_sum mbspex, 0x%x \n", l_dat_len_sum[l_i]); // shizu
																				  //printm("PEXOR_TK_Mem_Size with mbspex, 0x%x \n", PEXOR_TK_Mem_Size (&sPEXOR, l_i));
																				  // in bytes
#else
						l_dat_len_sum[l_i] = PEXOR_TK_Mem_Size (&sPEXOR, l_i);    // in bytes
																				  //if(l_dat_len_sum[l_i]==0) printm("l_dat_len_sum, 0x%x \n", l_dat_len_sum[l_i]); // shizu
																				  //   printm("PEXOR_TK_Mem_Size, 0x%x \n", PEXOR_TK_Mem_Size (&sPEXOR, l_i));    // in bytes
						PEXOR_RX_Clear_Ch (&sPEXOR, l_i);  // shizu
														   //printm("PEXOR_TK_Mem_Size, 0x%x \n", PEXOR_TK_Mem_Size (&sPEXOR, l_i));    // in bytes
														   //printm("l_dat_len_sum, 0x%x \n", l_dat_len_sum[l_i]);    // in bytes
#endif // USE_MBSPEX_LIB
						if(l_dat_len_sum[l_i] > 0x0)// zero suppression by shizu
							l_dat_len_sum[l_i] += 4; // wg. shizu !!??
													 //printm("l_dat_len_sum, 0x%x \n", l_dat_len_sum[l_i]); // shizu
#ifdef PEXOR_PC_DRAM_DMA

						// choose burst size to accept max. 20% padding size
						if      (l_dat_len_sum[l_i] < 0xa0 ) { l_burst_size = 0x10; }
						else if (l_dat_len_sum[l_i] < 0x140) { l_burst_size = 0x20; }
						else if (l_dat_len_sum[l_i] < 0x280) { l_burst_size = 0x40; }
						else                                 { l_burst_size = 0x80; }

						// setup DMA

						// transfer size must be adjusted to burst size
						if ( (l_dat_len_sum[l_i] % l_burst_size) != 0)
						{
							l_dma_trans_size    =  l_dat_len_sum[l_i] + l_burst_size     // in bytes
								- (l_dat_len_sum[l_i] % l_burst_size);
						}
						else
						{
							l_dma_trans_size = l_dat_len_sum[l_i];
						}

						l_padd[l_i] = 0;

						if ( l_dma_trans_size == 0) // Shizu : zero suppression
						{
						}
						else if ( ((long)pl_dat % l_burst_size) != 0)
						{
							l_padd[l_i] = l_burst_size - ((long)pl_dat % l_burst_size);
							l_dma_target_base = (long) pl_dat + l_diff_pipe_phys_virt + l_padd[l_i];
						}
						else
						{
							l_dma_target_base = (long) pl_dat + l_diff_pipe_phys_virt;
						}

#ifdef USE_MBSPEX_LIB
						if(l_dma_trans_size > 0x0) // JAM zero suppression by shizu
							mbspex_dma_rd (fd_pex, l_pex_sfp_phys_mem_base[l_i], l_dma_target_base, l_dma_trans_size, l_burst_size);
						// note: return value is true dma transfer size, we do not use this here

#else // USE_MBSPEX_LIB

						// source address is (must be) adjusted to burst size !

						if (l_dma_trans_size > 0x0) // zero suppression by shizu
						{ 
							*pl_dma_target_base = l_dma_target_base;
							*pl_dma_burst_size  = l_burst_size;                          // in bytes
							*pl_dma_trans_size  = l_dma_trans_size;
							*pl_dma_source_base = l_pex_sfp_phys_mem_base[l_i]; // starts also dma if USE_KINPEX_V5 defined
#ifndef USE_KINPEX_V5
							// do dma transfer pexor memory -> pc dram (sub-event pipe)
							*pl_dma_stat = 1;    // start dma
#endif
						}

						l_ct = 0;
						while (1)    // check if dma transfer finished
						{
							l_dat = *pl_dma_stat;
							if (l_dat == 0xffffffff)
							{
								printm (RON"ERROR>>"RES" PCIe bus errror, exiting.. \n");
								exit (0);
							}
							else if ((l_dat&0x1) == 0)
							{
								break; // dma shall be finished
							}
							l_ct++;
							if ( (l_ct % 1000000) == 0)
							{
								printm ("DMA not ready after %d queries: l_dat: %d \n", l_ct, l_dat);  
								sleep (1);
							}
#ifndef Linux
							yield ();
#else
							sched_yield ();
#endif
						}
#endif // (else) USE_MBSPEX_LIB

						// adjust pl_dat, pl_dat comes always 4 byte aligned
						// fill padding space with pattern
						l_padd[l_i] = l_padd[l_i] >> 2;                  // now in 4 bytes (longs) 
						for (l_k=0; l_k<l_padd[l_i]; l_k++)
						{
							//*pl_dat++ = 0xadd00000 + (l_i*0x1000) + l_k;
							*pl_dat++ = 0xadd00000 + (l_padd[l_i]<<8) + l_k;
						}
						// increment pl_dat with true transfer size (not dma transfer size)
						// true transfer size expected and must be 4 bytes aligned
						l_dat_len_sum_long[l_i] = (l_dat_len_sum[l_i] >> 2);
						pl_dat += l_dat_len_sum_long[l_i];

#else // PEXOR_PC_DRAM_DMA

						//l_dat_len_sum_long[l_i] = (l_dat_len_sum[l_i] >> 2) + 1;  // in 4 bytes
						l_dat_len_sum_long[l_i] = (l_dat_len_sum[l_i] >> 2);  // in 4 bytes

#ifdef USE_MBSPEX_LIB
						for (l_k=0; l_k<l_dat_len_sum_long[l_i]; l_k++)
						{
							l_rd_ct++;
							mbspex_register_rd (fd_pex, 0, PEX_MEM_OFF + (long)(PEX_SFP_OFF * l_i), pl_dat++);
						}
#else // USE_MBSPEX_LIB
						pl_tmp = pl_pex_sfp_mem_base[l_i];
						for (l_k=0; l_k<l_dat_len_sum_long[l_i]; l_k++)
						{
							*pl_dat++ = *pl_tmp++;
						}
#endif // (else) USE_MBSPEX_LIB
#endif // PEXOR_PC_DRAM_DMA
					}
				}
#endif // not DIRECT_DMA
#endif //  (USE_DRIVER_PARALLEL_TOKENREAD) &&  ! (SEQUENTIAL_TOKEN_SEND)
			}

			if ( (l_tr_ct[0] % STATISTIC) == 0)
			{
				printm ("----------------------------------------------------\n");
				printm ("nr of triggers processed: %u \n", l_tr_ct[0]);
				printm ("\n");
				for (l_i=1; l_i<MAX_TRIG_TYPE; l_i++)
				{
					if (l_tr_ct[l_i] != 0)
					{
						printm ("trigger type %2u found %10u times \n", l_i, l_tr_ct[l_i]);
					}
				}

#ifdef CHECK_META_DATA
				printm ("TAMEX - TRIXOR trigger type mismatches: %d \n", l_tam_trixor_trig_type_mism);
				printm ("TAMEX data size errors (trig. type 1):  %d \n", l_tam_tdc_data_size_1_err);
				printm ("TAMEX data size errors (trig. type 3):  %d \n", l_tam_tdc_data_size_3_err);
				printm ("TAMEX header  lec mismatches            %d \n", l_tdc_head_lec_err);
				printm ("TDC trailer lec mismatches              %d \n", l_tdc_trail_lec_err);
				printm ("");
				printm ("re-initialized TAMEX modules %d times \n", l_tam_init_ct);
				printm ("gosip error count: %lld", l_err_prot_ct);
#endif // CHECK_META_DATA
#ifdef WR_TIME_STAMP
				printm ("");
				printm ("reset White Rabbit PEXARIA TLU fifo      %d times \n", l_wr_init_ct);
#endif // WR_TIME_STAMP
				printm ("----------------------------------------------------\n");
			}

#ifdef CHECK_META_DATA
			//printm ("----------- check next event------------\n");
			pl_tmp = pl_dat_save;

#ifdef WR_TIME_STAMP
			// 5 first 32 bits must be WR time stamp
			l_dat = *pl_tmp++;
			if (l_dat != SUB_SYSTEM_ID)
			{
				printm (RON"ERROR>>"RES" 1. data word is not sub-system id: %d \n");
				printm ("should be: 0x%x, but is: 0x%x\n", SUB_SYSTEM_ID, l_dat);
			}
			l_dat = (*pl_tmp++) >> 16;
			if (l_dat != TS__ID_L16)
			{
				printm (RON"ERROR>>"RES" 2. data word does not contain low WR 16bit identifier: %d \n");
				printm ("should be: 0x%x, but is: 0x%x\n", TS__ID_L16, l_dat);
			}
			l_dat = (*pl_tmp++) >> 16;
			if (l_dat != TS__ID_M16)
			{
				printm (RON"ERROR>>"RES" 3. data word does not contain middle WR 16bit identifier: %d \n");
				printm ("should be: 0x%x, but is: 0x%x\n", TS__ID_M16, l_dat);
			}
			l_dat = (*pl_tmp++) >> 16;
			if (l_dat != TS__ID_H16)
			{
				printm (RON"ERROR>>"RES" 4. data word does not contain high WR 16bit identifier: %d \n");
				printm ("should be: 0x%x, but is: 0x%x\n", TS__ID_H16, l_dat);
			}
			l_dat = (*pl_tmp++) >> 16;
			if (l_dat != TS__ID_X16)
			{
				printm (RON"ERROR>>"RES" 5. data word does not contain 48-63 bit WR 16bit identifier: %d \n");
				printm ("should be: 0x%x, but is: 0x%x\n", TS__ID_X16, l_dat);
			}
#endif // WR_TIME_STAMP

			pl_tmp++; // because of trigger window

			while (pl_tmp < pl_dat)
			{
				//printm ("             while start \n"); sleep (1);
				l_dat = *pl_tmp++;   // must be padding word or channel header
									 //printm ("l_dat 0x%x \n", l_dat);
				if ( (l_dat & 0xfff00000) == 0xadd00000 ) // begin of padding 4 byte words
				{
					//printm ("padding found \n");
					l_dat = (l_dat & 0xff00) >> 8;
					//printm ("padding: %d \n", l_dat);
					pl_tmp += l_dat - 1;  // increment pointer with nr. of padding  4byte words 
				}
				else if ( (l_dat & 0xff) == 0x34) //channel header
				{
					l_tam_head = l_dat;
					//printm ("gosip header: 0x%x \n", l_tam_head);

					l_trig_type = (l_tam_head & 0xf00)      >>  8;
					l_sfp_id    = (l_tam_head & 0xf000)     >> 12;
					l_tam_id    = (l_tam_head & 0xff0000)   >> 16;
					l_tdc_id    = (l_tam_head & 0xff000000) >> 24; // must be always 0 in current version

					if (l_tdc_id != 0)
					{
						printm (RON"ERROR>>"RES" wrong TDC id: %d \n", l_tdc_id);
						printm ("        for SFP: %d, TAMEX id: %d, TDC id: %d \n", l_sfp_id, l_tam_id, l_tdc_id);
						l_err_prot_ct++;
						l_check_err = 2; goto bad_event;
					}

					if (l_sfp_id > 3)
					{
						printm (RON"ERROR>>"RES" wrong SFP id: %d \n", l_sfp_id);
						printm ("        for SFP: %d, TAMEX id: %d, TDC id:%d \n", l_sfp_id, l_tam_id, l_tdc_id);
						l_err_prot_ct++;
						l_check_err = 2; goto bad_event;
					}

					if (l_tam_id >= l_sfp_slaves[l_sfp_id])
					{
						printm (RON"ERROR>>"RES" wrong TAMEX id: %d \n", l_tdc_id);
						printm ("        for SFP: %d, TAMEX id: %d, TDC id:%d \n", l_sfp_id, l_tam_id, l_tdc_id);
						l_err_prot_ct++;
						l_check_err = 2; goto bad_event;
					}

					if ( ((l_tam_head & 0xff) >> 0) != 0x34 )
					{
						printm (RON"ERROR>>"RES" TDC header type is not 0x34 \n");
						l_err_prot_ct++;
					}

					if ( l_trig_type != bh_trig_typ )
					{
						printm (RON"ERROR>>"RES" trigger type is not the same as from TRIXOR \n");
						printm ("        trigger types: TRIXOR: %d, TAMEX: %d \n", bh_trig_typ, (l_tam_head & 0xff00) >> 8);
						printm ("        for SFP: %d, TAMEX id: %d, TDC id:%d \n", l_sfp_id, l_tam_id, l_tdc_id);
						l_err_prot_ct++;
						l_tam_trixor_trig_type_mism++;
						l_check_err = 2; goto bad_event;
					}

					// TDC data size
					l_tdc_size = *pl_tmp++;

					if (bh_trig_typ == 3)
					{
						//printm ("synch. trigger, bh_trig_typ: 3 \n");
						if (l_tdc_size != 12)
						{
							printm (RON"ERROR>>"RES" TDC data size: %d is  wrong \n", l_tdc_size);
							printm ("        for trigger type %d \n", bh_trig_typ);
							l_err_prot_ct++;
							l_tam_tdc_data_size_3_err++;
							l_check_err = 2; goto bad_event;
						}
					}

					if ( (bh_trig_typ != 1) && (bh_trig_typ != 3) )
					{
						printm ("TRIXOR trigger type neither 1 nor 3 ?? \n");
					}

					// TDC header
					l_tdc_head = *pl_tmp++;
					if ( (l_tdc_head & 0xffff) != (l_lec_check & 0xffff) )
					{
						printm (RON"ERROR>>"RES" local event counter mismatch in TDC header \n");
						printm ("        SFP: %d, slave id: %d, TDC: %d \n", l_i, l_j, l_k);
						printm ("        lec is: %d, but must be %d \n",
								l_tdc_head & 0xffff, l_lec_check & 0xffff);
						l_err_prot_ct++;
						l_tdc_head_lec_err++;
						l_check_err = 2; goto bad_event;
					}
					if ( ((l_tdc_head & 0xf0000) >> 16) != l_tog )
					{
						printm (RON"ERROR>>"RES" buffer (0,1) mismatch with toggle bit in TDC header\n");
						printm ("                 toggle bit : 0x%x \n", l_tog);
						l_err_prot_ct++;
					}
					if ( ((l_tdc_head & 0xf00000) >> 20) != bh_trig_typ )
					{
						printm (RON"ERROR>>"RES" wrong trigger type in TDC header \n");
						l_err_prot_ct++;
						l_check_err = 2; goto bad_event;
					}
					if ( ((l_tdc_head & 0xff000000) >> 24) != 0xaa)
					{
						printm (RON"ERROR>>"RES" TDC header id is not 0xaa \n");
						l_err_prot_ct++;
					}

					// jump over TDC data

					// shizu 05.02.2021 -- start 
					//          pl_tmp += (l_tdc_size/4) - 2; 
					pl_tmp += (l_tdc_size/4) - 3; 
					//          l_tdc_err_flg = *pl_tmp; 
					l_tdc_err_flg = *pl_tmp++; 
					if ( (l_tdc_err_flg & 0xfff0) != 0 )
					{
						printm (RON"ERROR>>"RES" a trigger was rejected: error flag 0x%x \n", l_tdc_err_flg);
						l_err_prot_ct++;
						l_check_err = 2; goto bad_event;
					}
					// shizu 05.02.2021 -- end

					// TDC trailer
					l_tdc_trail = *pl_tmp++;
					if ( (l_tdc_trail & 0xffff) != (l_lec_check & 0xffff) )
					{
						printm (RON"ERROR>>"RES" local event counter mismatch in TDC trailer\n");
						printm ("        SFP: %d, slave id: %d, TDC: %d \n", l_i, l_j, l_k);
						printm ("        lec is: %d, but must be %d \n\n", l_tdc_trail & 0xffff, l_lec_check & 0xffff);
						l_err_prot_ct++;
						l_tdc_trail_lec_err++;
						l_check_err = 2; goto bad_event;
					}
					if ( ((l_tdc_trail & 0xf0000) >> 16) != l_tog )
					{
						printm (RON"ERROR>>"RES" buffer (0,1) mismatch with toggle bit in TDC trailer\n");
						l_err_prot_ct++;
					}
					if ( ((l_tdc_trail & 0xf00000) >> 20) != bh_trig_typ )
					{
						printm (RON"ERROR>>"RES" wrong trigger type in TDC trailer \n");
						printm ("        TRIXOR: %d, trailer: %d \n", bh_trig_typ, ((l_tdc_trail & 0xf00000) >> 20));
						l_err_prot_ct++;
						l_check_err = 2; goto bad_event;
					}
					if ( ((l_tdc_trail & 0xff000000) >> 24) != 0xbb)
					{
						printm (RON"ERROR>>"RES" TDC trailer id is not 0xbb \n");
						l_err_prot_ct++;
					}
				}
				else
				{
					printm (RON"ERROR>>"RES" evt: %d data word: 0x%x neither channel header nor padding word \n", l_tr_ct[0], l_dat);
					sleep (1);
					goto bad_event;
				}
			}
#endif // CHECK_META_DATA

bad_event:

#ifdef WR_TIME_STAMP
			if (l_check_wr_err == 0)
			{
				*l_se_read_len = (long)pl_dat - (long)pl_dat_save;
			}
			else
			{
				if (l_check_wr_err == 2)
				{
					printm ("white rabbit failure: invalidate current trigger/event (1) (0xbad00bad)\n");
				}
				if (l_check_wr_err == 1)
				{
					printm ("white rabbit failure: invalidate current trigger/event (2) (0xbad00bad)\n");
					printm ("");
				}
				pl_dat = pl_dat_save;
				*pl_dat++ = 0xbad00bad;
				*l_se_read_len = 4;
				l_check_wr_err--;
			}
#endif // WR_TIME_STAMP

			if (l_check_err == 0)
			{
				*l_se_read_len = (long)pl_dat - (long)pl_dat_save;
			}
			else
			{
				printm ("invalidate current trigger/event  (0xbad00bad)\n");
				pl_dat = pl_dat_save;
				*pl_dat++ = 0xbad00bad;
				*l_se_read_len = 4;
				l_check_err--;
				l_fi_hw_trg=0;
			}
			break;

		case 14:
			break;

		case 15:
			l_tog = 1;
			l_lec_check =  0;
			l_fi_hw_trg =  0;
			break;
		default:
			break;
	}
	bh_trig_typ_prev = bh_trig_typ; //shizu
	return (1);
}

/*****************************************************************************/


int f_pex_slave_init (long l_sfp, long l_n_slaves)
{

#ifdef USE_MBSPEX_LIB
	return mbspex_slave_init (fd_pex, l_sfp, l_n_slaves);
#else

	int  l_ret;
	long l_comm;

	printm ("initialize SFP chain %d ", l_sfp);
	l_comm = PEXOR_INI_REQ | (0x1<<16+l_sfp);

	for (l_j=1; l_j<=10; l_j++)
	{
		PEXOR_RX_Clear_Ch (&sPEXOR, l_sfp);
		PEXOR_TX (&sPEXOR, l_comm, 0, l_n_slaves  - 1) ;

		//printm ("SFP %d: try nr. %d \n", l_sfp, l_j);
		l_dat1 = 0; l_dat2 = 0; l_dat3 = 0;
		l_stat = PEXOR_RX (&sPEXOR, l_sfp, &l_dat1 , &l_dat2, &l_dat3);
		if ( (l_stat != -1) && (l_dat2 > 0) && (l_dat2<=32))
		{
			break;
		}
#ifndef Linux
		yield ();
#else
		sched_yield ();
#endif
	}
	l_ret = 0;
	if (l_stat == -1)
	{
		l_ret = -1;
		printm (RON"ERROR>>"RES" initialization of SFP chain %d failed. ", l_sfp);
		printm ("no reply: 0x%x 0x%x 0x%x \n", l_dat1, l_dat2, l_dat3);
		//printm ("exiting.. \n"); exit (0);
	}
	else
	{
		if (l_dat2 != 0)
		{
			printm ("initialization for SFP chain %d done. \n", l_sfp),
				   printm ("No of slaves : %d \n", l_dat2);
		}
		else
		{
			l_ret = -1;
			printm (RON"ERROR>>"RES" initialization of SFP chain %d failed. ", l_sfp);
			printm ("no slaves found \n");
			//printm ("exiting.. \n"); exit (0);
		}
	}
	return (l_ret);
#endif // (else) USE_MBSPEX_LIB
}

/*****************************************************************************/

int f_pex_slave_wr (long l_sfp, long l_slave, long l_slave_off, long l_dat)
{
#ifdef USE_MBSPEX_LIB
	return mbspex_slave_wr (fd_pex, l_sfp, l_slave, l_slave_off, l_dat);
#else

	int  l_ret;
	long l_comm;
	long l_addr;

	l_comm = PEXOR_PT_AD_W_REQ | (0x1<<16+l_sfp);
	l_addr = l_slave_off + (l_slave << 24);
	PEXOR_RX_Clear_Ch (&sPEXOR, l_sfp);
	PEXOR_TX (&sPEXOR, l_comm, l_addr, l_dat);
	l_stat = PEXOR_RX (&sPEXOR, l_sfp, &l_dat1 , &l_dat2, &l_dat3);

	l_ret = 0;
	if (l_stat == -1)
	{
		l_ret = -1;
		l_err_flg++;
		l_i_err_flg[l_sfp][l_slave]++;
#ifdef DEBUG
		printm (RON"ERROR>>"RES" writing to SFP: %d, slave id: %d, addr 0x%d \n",
				l_sfp, l_slave, l_slave_off);
		printm ("  no reply: 0x%x 0x%x 0x%x \n", l_dat1, l_dat2, l_dat3);
#endif // DEBUG
	}
	else
	{
		// printm ("Reply to PEXOR from SFP: 0x%x ", l_sfp);
		if( (l_dat1 & 0xfff) == PEXOR_PT_AD_W_REP)
		{
			//printm ("SFP: %d, slave id: %d addr: 0x%x  \n",
			//                l_sfp, (l_dat2 & 0xf0000) >> 24, l_dat2 & 0xfffff);
			if ( (l_dat1 & 0x4000) != 0)
			{
				l_ret = -1;
				l_err_flg++;
				l_i_err_flg[l_sfp][l_slave]++;
#ifdef DEBUG
				printm (RON"ERROR>>"RES" packet structure: command reply 0x%x \n", l_dat1);
#endif // DEBUG
			}
		}
		else
		{
			l_ret = -1;
			l_err_flg++;
			l_i_err_flg[l_sfp][l_slave]++;
#ifdef DEBUG
			printm (RON"ERROR>>"RES" writing to empty slave or wrong address: \n");
			printm ("  SFP: %d, slave id: %d, 0x%x addr: 0x%x,  command reply:  0x%x \n",
					l_sfp, l_slave, (l_addr & 0xf00000) >> 24 , l_addr & 0xfffff, l_dat1);
#endif // DEBUG
		}
	}
	return (l_ret);
#endif // (else) USE_MBSPEX_LIB
}

/*****************************************************************************/

int f_pex_slave_rd (long l_sfp, long l_slave, long l_slave_off, long *l_dat)
{
#ifdef USE_MBSPEX_LIB
	return mbspex_slave_rd (fd_pex, l_sfp, l_slave, l_slave_off, l_dat);
#else

	int  l_ret;
	long l_comm;
	long l_addr;

	l_comm = PEXOR_PT_AD_R_REQ | (0x1<<16+l_sfp);
	l_addr = l_slave_off + (l_slave << 24);
	PEXOR_RX_Clear_Ch (&sPEXOR, l_sfp);
	PEXOR_TX (&sPEXOR, l_comm, l_addr, 0);
	l_stat = PEXOR_RX (&sPEXOR, l_sfp, &l_dat1 , &l_dat2, l_dat);
	//printm ("f_pex_slave_rd, l_dat: 0x%x, *l_dat: 0x%x \n", l_dat, *l_dat);

	l_ret = 0;
	if (l_stat == -1)
	{
		l_ret = -1;
		l_err_flg++;
		l_i_err_flg[l_sfp][l_slave]++;
#ifdef DEBUG
		printm (RON"ERROR>>"RES" reading from SFP: %d, slave id: %d, addr 0x%d \n",
				l_sfp, l_slave, l_slave_off);
		printm ("  no reply: 0x%x 0x%x 0x%x \n", l_dat1, l_dat2, *l_dat);
#endif // DEBUG
	}
	else
	{
		// printm ("Reply to PEXOR from SFP: 0x%x ", l_sfp);
		if( (l_dat1 & 0xfff) == PEXOR_PT_AD_R_REP)
		{
			//printm ("SFP: %d, slave id: %d addr: 0x%x  \n",
			//     l_sfp, (l_dat2 & 0xf00000) >> 24, l_dat2 & 0xfffff);
			if ( (l_dat1 & 0x4000) != 0)
			{
				l_ret = -1;
				l_err_flg++;
				l_i_err_flg[l_sfp][l_slave]++;
#ifdef DEBUG
				printm (RON"ERROR>>"RES" packet structure: command reply 0x%x \n", l_dat1);
#endif //DEBUG
			}
		}
		else
		{
			l_ret = -1;
			l_err_flg++;
			l_i_err_flg[l_sfp][l_slave]++;
#ifdef DEBUG
			printm (RON"ERROR>>"RES" Reading from empty slave or wrong address: \n");
			printm ("  SFP: %d, slave id: %d, 0x%x addr: 0x%x,  command reply:  0x%x \n",
					l_sfp, l_slave, (l_addr & 0xf0000) >> 24 , l_addr & 0xfffff, l_dat1);
#endif // DEBUG
		}
	}
	return (l_ret);
#endif // (else) USE_MBSPEX_LIB
}

/*****************************************************************************/

void f_tam_init ()

{
	for (l_i=0; l_i<MAX_SFP; l_i++)
	{
		if (l_sfp_slaves[l_i] != 0)
		{
			l_stat = f_pex_slave_init (l_i, l_sfp_slaves[l_i]);
			if (l_stat == -1)
			{
				printm (RON"ERROR>>"RES" slave address initialization failed \n");
				printm ("exiting...\n");
				exit (-1);
			}
		}
		printm ("");
	}

	//sleep (1);

	if (l_first == 0)
	{
		l_first = 1;
		for (l_i=0; l_i<MAX_TRIG_TYPE; l_i++)
		{
			l_tr_ct[l_i] = 0;
		}
	}

	for (l_i=0; l_i<MAX_SFP; l_i++)
	{
		if (l_sfp_slaves[l_i] != 0)
		{
			for (l_j=0; l_j<l_sfp_slaves[l_i]; l_j++)
			{
				// needed for check of meta data, read it in any case
				printm ("SFP: %d, TAMEX: %d \n", l_i, l_j);
				// get address offset of TAMEX buffer 0,1 for each TAMEX
				l_stat = f_pex_slave_rd (l_i, l_j, REG_BUF0, &(l_tam_buf_off[l_i][l_j][0]));
				l_stat = f_pex_slave_rd (l_i, l_j, REG_BUF1, &(l_tam_buf_off[l_i][l_j][1]));
				// get nr. of TDCs per TAMEX
				l_stat = f_pex_slave_rd (l_i, l_j, REG_SUBMEM_NUM, &(l_tam_n_tdc[l_i][l_j]));
				// get buffer per TDC offset
				l_stat = f_pex_slave_rd (l_i, l_j, REG_SUBMEM_OFF, &(l_tam_tdc_off[l_i][l_j]));

				printm ("addr offset: buf0: 0x%x, buf1: 0x%x \n",
						l_tam_buf_off[l_i][l_j][0], l_tam_buf_off[l_i][l_j][1]);
				printm ("No. TDCs: %d \n", l_tam_n_tdc[l_i][l_j]);
				printm ("TDC addr offset: 0x%x \n", l_tam_tdc_off[l_i][l_j]);

				// reset gosip token waiting bits // shizu
				l_stat = f_pex_slave_wr (l_i, l_j, REG_RST, 0x1);
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" reset gosip token waiting bits failed\n");
					l_err_prot_ct++;
				}

				l_stat = f_pex_slave_rd (l_i, l_j, REG_RST, &(l_tam_rst_stat[l_i][l_j]));	
				printm ("STAT_DRDY: %d \n", l_tam_rst_stat[l_i][l_j]);

				// disable test data length
				l_stat = f_pex_slave_wr (l_i, l_j, REG_DATA_LEN, 0x10000000);
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" disabling test data length failed\n");
					l_err_prot_ct++;
				}

				// disable/enable data reduction // shizu
				l_stat = f_pex_slave_wr (l_i, l_j, REG_DATA_REDUCTION, DATA_REDUCTION);
				//        l_stat = f_pex_slave_wr (l_i, l_j, REG_DATA_REDUCTION, (1-l_j) );
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" PEXOR slave write REG_DATA_REDUCTION failed\n");
					l_err_prot_ct++;
				}

				// disable buffers to record // shizu
				l_stat = f_pex_slave_wr (l_i, l_j, REG_MEM_DISABLE, DISABLE_CHANNEL );
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" PEXOR slave write REG_MEM_DISABLE  failed\n");
					l_err_prot_ct++;
				}

				// write SFP id for TDC header
				l_stat = f_pex_slave_wr (l_i, l_j, REG_HEADER, l_i);
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" PEXOR slave write REG_HEADER  failed\n");
					l_err_prot_ct++;
				}

				// Set PADI default thresholds
#ifdef SET_PADI_TH_AT_INIT
				if ( (l_sfp_tam_mode[l_i] == 2) || (l_sfp_tam_mode[l_i] == 3) )  // TAMEX2+PADI & TAMEX-PADI1
				{
					//printm (RON"DEBUG>>"RES" SET_PADI_TH_AT_INIT SPI DATA: 0x%x \n", PADI_DEF_TH);
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_DAT, PADI_DEF_TH);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PADI failed 1\n");
						l_err_prot_ct++;
					}

					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_CTL, 0x1);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PADI failed 2\n");
						l_err_prot_ct++;
					}

					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_CTL, 0x0);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PADI failed 3\n");
						l_err_prot_ct++;
					}
				}
#endif


				// Set PQDC default thresholds
#ifdef SET_PQDC_TH_AT_INIT
				if (l_sfp_tam_mode[l_i] == 41)  // TAMEX4 PQDC1
				{
					int l_pqdc_th = (int) (((1100.0 + l_pqdc_threshold_rel) / 3300.0) * 65535.0);
					// Channel 1 / 4 on all 4 FPGAS
					//printm (RON"DEBUG>>"RES" SET_PQDC_TH_AT_INIT SPI DATA: 0x%x\n", 0x800000 | l_pqdc_th);
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_DAT, 0x800000 | l_pqdc_th);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 1\n");
						l_err_prot_ct++;
					}
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_CTL, 0xf1);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 2\n");
						l_err_prot_ct++;
					}
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_CTL, 0xf0);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 3\n");
						l_err_prot_ct++;
					}
					usleep (100000);
					// Channel 2 / 4 on all 4 FPGAS
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_DAT, 0x810000 | l_pqdc_th);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 1\n");
						l_err_prot_ct++;
					}
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_CTL, 0xf1);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 2\n");
						l_err_prot_ct++;
					}
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_CTL, 0xf0);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 3\n");
						l_err_prot_ct++;
					}
					usleep (100000);
					// Channel 3 / 4 on all 4 FPGAS
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_DAT, 0x820000 | l_pqdc_th);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 1\n");
						l_err_prot_ct++;
					}
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_CTL, 0xf1);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 2\n");
						l_err_prot_ct++;
					}
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_CTL, 0xf0);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 3\n");
						l_err_prot_ct++;
					}
					usleep (100000);
					// Channel 4 / 4 on all 4 FPGAS
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_DAT, 0x830000 | l_pqdc_th);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 1\n");
						l_err_prot_ct++;
					}
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_CTL, 0xf1);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 2\n");
						l_err_prot_ct++;
					}
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_SPI_CTL, 0xf0);
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting PQDC thresholds failed 3\n");
						l_err_prot_ct++;
					}
				}
#endif

				// Set TAMEX registers
				if (l_initialize_tamex_modules)
				{
					if ( (l_sfp_tam_mode[l_i] == 1) || (l_sfp_tam_mode[l_i] == 2) ) // Set TAMEX2 clock source
					{

						if (CLK_SRC_TDC_TAM2 == 0x22) // special case clock distribution via TRBus
						{
							l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CLK_SEL, 0x21);
							if (l_stat == -1)
							{
								printm (RON"ERROR>>"RES" Setting clock source failed\n");
								l_err_prot_ct++;
							}
						}
						else
						{
							l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CLK_SEL, CLK_SRC_TDC_TAM2);
							if (l_stat == -1)
							{
								printm (RON"ERROR>>"RES" Setting clock source failed\n");
								l_err_prot_ct++;
							}
						}

						if ((CLK_SRC_TDC_TAM2 == 0x22) && (l_j == 0)) // If clock from TRBus used
						{
							l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_BUS_EN, 0x80); // Enable feeding clock to TRBus on slave 0
							if (l_stat == -1)
							{
								printm (RON"ERROR>>"RES" Enabling TRBus CLK on slave 0 failed\n");
								l_err_prot_ct++;
							}
						}
					}
					else if ((l_sfp_tam_mode[l_i] == 3) || (l_sfp_tam_mode[l_i] == 41)) // Set TAMEX-PADI1 / TAMEX4 clock source
					{
						if (CLK_SRC_TDC_TAM4_PADI == 0x4) // First module clock master for crate with local oscillator
						{
							if (l_j == 0)
							{
								l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CLK_SEL, 0x1); // Enable local clock oscillator on slave 0
								if (l_stat == -1)
								{
									printm (RON"ERROR>>"RES" Setting clock source failed\n");
									l_err_prot_ct++;
								}
							}
							else
							{
								l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CLK_SEL, 0x0); // Enable clock from backplane on all other modules
								if (l_stat == -1)
								{
									printm (RON"ERROR>>"RES" Setting clock source failed\n");
									l_err_prot_ct++;
								}
							}
						}
						else if (CLK_SRC_TDC_TAM4_PADI == 0x8) // First module clock master for crate with clock from front panel
						{
							if (l_j == 0)
							{
								l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CLK_SEL, 0x2); // Enable clock from front panel on slave 0
								if (l_stat == -1)
								{
									printm (RON"ERROR>>"RES" Setting clock source failed\n");
									l_err_prot_ct++;
								}
							}
							else
							{
								l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CLK_SEL, 0x0); // Enable clock from backplane on all other modules
								if (l_stat == -1)
								{
									printm (RON"ERROR>>"RES" Setting clock source failed\n");
									l_err_prot_ct++;
								}
							}
						}
						else
						{
							l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CLK_SEL, CLK_SRC_TDC_TAM4_PADI); // Set same clock source on all modules
							if (l_stat == -1)
							{
								printm (RON"ERROR>>"RES" Setting clock source failed\n");
								l_err_prot_ct++;
							}
						}
					}
					else if (l_sfp_tam_mode[l_i] == 10) // Set TAMEX3 clock source
					{
						l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CLK_SEL, CLK_SRC_TDC_TAM3);
						if (l_stat == -1)
						{
							printm (RON"ERROR>>"RES" Setting clock source failed\n");
							l_err_prot_ct++;
						}
						if ((CLK_SRC_TDC_TAM3 == 0x22) && (l_j == 0)) // If clock from TRBus used
						{
							l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_BUS_EN, 0x80); // Enable feeding clock to TRBus on slave 0
							if (l_stat == -1)
							{
								printm (RON"ERROR>>"RES" Enabling TRBus CLK on slave 0 failed\n");
								l_err_prot_ct++;
							}
						}
					}
#ifdef DONT_RE_INITIALIZE_MODULES_AFTER_ERROR  // Henning
					l_initialize_tamex_modules = 0
#endif
				}

				printm ("trigger window: 0x%x \n", l_trig_wind); // set trigger window
				l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_TRG_WIN, l_trig_wind);
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" Setting TDC trigger window failed\n");
					l_err_prot_ct++;
				}

#ifdef EN_REF_CH
				//        l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x7c20d0); // set reset bit
				l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x20d0| l_tam_fifo_almost_full_sh ); // set reset bit	
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" TDC reset failed\n");
					l_err_prot_ct++;
				}

				if ( (l_sfp_tam_mode[l_i] == 1) || (l_sfp_tam_mode[l_i] == 2) || (l_sfp_tam_mode[l_i] == 3) || (l_sfp_tam_mode[l_i] == 41)) // TAMEX2 & TAMEX-PADI1 & TAMEX4
				{
					// clear reset & set CNTRL_REG (CH0 enabled)
					//          l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x7c20c0 | l_enable_or | l_combine_trig);// | l_en_async_trig);
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x20c0 | l_enable_or | l_combine_trig| l_tam_fifo_almost_full_sh);// | l_en_async_trig);	  
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting TDC control register failed\n");
						l_err_prot_ct++;
					}
				}
				else if (l_sfp_tam_mode[l_i] == 10)                             // TAMEX3
				{
					// clear reset & set CNTRL_REG (CH0 enabled)
					//          l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x7c20c0);
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x20c0| l_tam_fifo_almost_full_sh);	  
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting TDC control register failed\n");
						l_err_prot_ct++;
					}
				}

#else // !EN_REF_CH

				//        l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x7c2050); // set reset bit
				l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x2050| l_tam_fifo_almost_full_sh); // set reset bit	
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" TDC reset failed\n");
					l_err_prot_ct++;
				}

				if ((l_sfp_tam_mode[l_i] == 1) || (l_sfp_tam_mode[l_i] == 2) || (l_sfp_tam_mode[l_i] == 3) || (l_sfp_tam_mode[l_i] == 41)) // TAMEX2 & TAMEX-PADI1 & TAMEX4
				{
					// clear reset & set CNTRL_REG
					//          l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x7c2040 | l_enable_or | l_combine_trig);
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x2040 | l_enable_or | l_combine_trig| l_tam_fifo_almost_full_sh);	  
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting TDC control register failed\n");
						l_err_prot_ct++;
					}
				}
				else if (l_sfp_tam_mode[l_i] == 10)                             // TAMEX3
				{
					// clear reset & set CNTRL_REG
					//          l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x7c2040);
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_CTRL, 0x2040| l_tam_fifo_almost_full_sh);	  
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Setting TDC control register failed\n");
						l_err_prot_ct++;
					}
				}

#endif

				l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_EN_CH, EN_TDC_CH); // set tdc channel enable register
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" Enabling TDC channels failed\n");
					l_err_prot_ct++;
				}


				//////////// SELF_TRIG ///////////////
#ifdef IDATEN
				l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_EN_TR, l_selftrig[l_j]); // set trigger enable register
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" Writing TRIG enable register failed2\n");
					l_err_prot_ct++;
				}
#else //IDATEN	
				if (l_i==1 && (l_j==0 || l_j==1))
				{
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_EN_TR, 0xffffffff); // set trigger enable register
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Writing TRIG enable register failed2\n");
						l_err_prot_ct++;
					}
				}
				else if (l_i==1 && (l_j ==2))
				{
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_EN_TR, 0xffffffff); // set trigger enable register
																				   //l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_EN_TR, 0x00ff00ff); // set trigger enable register
																				   //l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_EN_TR, 0x00ff0000); // set trigger enable register
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Writing TRIG enable register failed2\n");
						l_err_prot_ct++;
					}
				}
				else 
				{
					//l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_EN_TR, EN_TRIG_CH); // set trigger enable register
					l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_EN_TR, DIS_TRIG_CH); // set trigger enable register
					if (l_stat == -1)
					{
						printm (RON"ERROR>>"RES" Writing TRIG enable register failed\n");
						l_err_prot_ct++;
					}
				}
#endif // IDATEN
	   //////////// SELF_TRIG ///////////////

				l_stat = f_pex_slave_wr (l_i, l_j, REG_TAM_POLARITY, TDC_CH_POL); // set tdc channel polarity register
				if (l_stat == -1)
				{
					printm (RON"ERROR>>"RES" Writing CH polarity register failed\n");
					l_err_prot_ct++;
				}

				// shizu
				l_stat = f_pex_slave_rd (l_i, l_j, REG_RST, &(l_tam_rst_stat[l_i][l_j]));	
				printm ("STAT_DRDY: %d \n", l_tam_rst_stat[l_i][l_j]);

			}
		}
	}
}

/*****************************************************************************/

#ifndef USE_MBSPEX_LIB

int f_pex_send_and_receive_tok (long l_sfp, long l_toggle,
		long *pl_check1, long *pl_check2, long *pl_check3)
{
	int  l_ret;
	long l_comm;

	l_comm = PEXOR_PT_TK_R_REQ | (0x1<<16+l_sfp);
	PEXOR_RX_Clear_Ch(&sPEXOR, l_sfp);
	PEXOR_TX (&sPEXOR, l_comm, l_toggle, 0);
	l_stat = PEXOR_RX (&sPEXOR, l_sfp, pl_check1, pl_check2, pl_check3);
	// return values:
	// l_check1: l_comm
	// l_check2: toggle bit
	// l_check3: nr. of slaves connected to token chain

	l_ret = 0;
	if (l_stat == -1)
	{
		l_ret = -1;
#ifdef DEBUG
		printm (RON"ERROR>>"RES" sending token to SFP: %d \n", l_sfp);
		printm ("  no reply: 0x%x 0x%x 0x%x \n", *pl_check1, *pl_check2, *pl_check3);
#endif // DEBUG
	}

	return (l_ret);
}

#endif // (ifndef) USE_MBSPEX_LIB

/*****************************************************************************/

#ifndef USE_MBSPEX_LIB

int f_pex_send_tok (long l_sfp_p, long l_toggle)
{
	// sends token to all SFPs marked in l_sfp_p pattern: 1: sfp 0, 2: sfp 1,
	//                                                    4: sfp 2, 8: sfp 3,
	//                                                  0xf: all four SFPs

	long l_comm;

	l_comm = PEXOR_PT_TK_R_REQ | (l_sfp_p << 16);
	PEXOR_RX_Clear_Pattern(&sPEXOR, l_sfp_p);
	PEXOR_TX (&sPEXOR, l_comm, l_toggle, 0);

	return (0);
}

#endif // (ifndef) USE_MBSPEX_LIB

/*****************************************************************************/

#ifndef USE_MBSPEX_LIB

int f_pex_receive_tok (long l_sfp, long *pl_check1, long *pl_check2, long *pl_check3)
{
	// checks token return for a single, individual SFPS
	int  l_ret;

	l_stat = PEXOR_RX (&sPEXOR, l_sfp, pl_check1, pl_check2, pl_check3);
	// return values:
	// l_check1: l_comm
	// l_check2: toggle bit
	// l_check3: nr. of slaves connected to token chain

	l_ret = 0;
	if (l_stat == -1)
	{
		l_ret = -1;
#ifdef DEBUG
		printm (RON"ERROR>>"RES" receiving token from SFP: %d \n", l_sfp);
		printm ("  no reply: 0x%x 0x%x 0x%x \n", *pl_check1, *pl_check2, *pl_check3);
#endif // DEBUG
	}

	return (l_ret);
}

#endif // (ifndef) USE_MBSPEX_LIB

/*****************************************************************************/

#ifdef WR_TIME_STAMP
void f_wr_init ()
{
#ifdef WR_USE_TLU_DIRECT

	//sleep(1);
	*ch_select = WR_TLU_FIFO_NR;
	printm ("directly selected White Rabbit TLU FIFO channel number: %3d \n", *ch_select);
	//sleep(1);
	eb_fifo_size = *ch_fifosize;
	printm ("size of  White Rabbit TLU FIFO:                %3d \n", eb_fifo_size);
	printm ("");
	*fifoclear =0xFFFFFFFF;

	*armset =0xFFFFFFFF;

#else // WR_USE_TLU_DIRECT

	// select FIFO channel
	if ((eb_stat = eb_device_write (eb_device, wrTLU + GSI_TM_LATCH_CH_SELECT, EB_BIG_ENDIAN|EB_DATA32, WR_TLU_FIFO_NR, 0, eb_block)) != EB_OK)
	{
		printm (RON"ERROR>>"RES" when selecting TLU FIFO channel \n");
	}

	// read back selected FIFO channel
	if ((eb_stat = eb_device_read (eb_device, wrTLU + GSI_TM_LATCH_CH_SELECT, EB_BIG_ENDIAN|EB_DATA32, &eb_fifo_cha, 0, eb_block)) != EB_OK)
	{
		printm (RON"ERROR>>"RES" when reading selected FIFO channel number \n");
	}
	else
	{
		printm ("");
		printm ("selected White Rabbit TLU FIFO channel number: %d \n", eb_fifo_cha);
	}

	l_eb_first2 = 1;
	// size of TLU FIFO
	if ((eb_stat = eb_device_read (eb_device, wrTLU + GSI_TM_LATCH_CHNS_FIFOSIZE, EB_BIG_ENDIAN|EB_DATA32, &eb_fifo_size, 0, eb_block)) != EB_OK)
	{
		printm (RON"ERROR>>"RES" when reading TLU FIFO size \n");
	}
	else
	{
		printm ("size of  White Rabbit TLU FIFO:                %d \n", eb_fifo_size);
		printm ("");
	}

	/* prepare the TLU for latching of timestamps */
	/* clear all FIFOs */
	if ((eb_stat = eb_device_write(eb_device, wrTLU + GSI_TM_LATCH_FIFO_CLEAR,
					EB_BIG_ENDIAN|EB_DATA32, 0xFFFFFFFF, 0, eb_block)) != EB_OK)
	{
		printm (RON"ERROR>>"RES" etherbone TLU eb_device_write (CLEAR TLU FIFOS), status: %s \n", eb_status(eb_stat));
	}

	/* arm triggers for latching */
	if ((eb_stat = eb_device_write(eb_device, wrTLU + GSI_TM_LATCH_TRIG_ARMSET,
					EB_BIG_ENDIAN|EB_DATA32, 0xFFFFFFFF, 0, eb_block)) != EB_OK)
	{
		printm (RON"ERROR>>"RES" etherbone TLU eb_device_write (ARM LATCHING), status: %s \n", eb_status(eb_stat));
	}

#endif // WR_USE_TLU_DIRECT
}

#endif // WR_TIME_STAMP

/*****************************************************************************/

#ifdef WR_TIME_STAMP
void f_wr_reset_tlu_fifo ()
{
	/* clear all FIFOs */
#ifdef WR_USE_TLU_DIRECT
	*fifoclear =0xFFFFFFFF;
#else
	if ((eb_stat = eb_device_write(eb_device, wrTLU + GSI_TM_LATCH_FIFO_CLEAR,
					EB_BIG_ENDIAN|EB_DATA32, 0xFFFFFFFF, 0, eb_block)) != EB_OK)
	{
		printm (RON"ERROR>>"RES" etherbone TLU eb_device_write (CLEAR TLU FIFOS), status: %s \n", eb_status(eb_stat));
	}
#endif
}
#endif // WR_TIME_STAMP

/*****************************************************************************/
