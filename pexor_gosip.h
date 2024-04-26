//// 10/09/2020 for gosip master version 5.0 "USE_KINPEX_V5" is defined in f_user.c.

#ifndef PEXOR_NAME
#define PEXOR_NAME "PEXOR"
//#define DEBUG

// byte address offset for PEXOR
#define PEXOR_BASE 0x0
#define PEXOR_REQ_COMM 0x21000
#define PEXOR_REQ_ADDR 0x21004
#define PEXOR_REQ_DATA 0x21008
#define PEXOR_REP_STAT 0x2100c
#define PEXOR_REP_CLR  0x2100c
#define PEXOR_REP_STAT_0 0x21010
#define PEXOR_REP_STAT_1 0x21014
#define PEXOR_REP_STAT_2 0x21018
#define PEXOR_REP_STAT_3 0x2101c

#define PEXOR_REP_ADDR_0 0x21020
#define PEXOR_REP_ADDR_1 0x21024
#define PEXOR_REP_ADDR_2 0x21028
#define PEXOR_REP_ADDR_3 0x2102c

#define PEXOR_REP_DATA_0 0x21030
#define PEXOR_REP_DATA_1 0x21034
#define PEXOR_REP_DATA_2 0x21038
#define PEXOR_REP_DATA_3 0x2103c

#define PEXOR_RX_MONI 0x21040
#define PEXOR_RX_RST  0x21044
#define PEXOR_SFP_DISA 0x21048
#define PEXOR_SFP_FAULT 0x2104c

#define PEXOR_SFP_FIFO 0x21050

#define PEXOR_REP_TK_STAT 0x21060
#define PEXOR_REP_TK_HEAD 0x21070
#define PEXOR_REP_TK_FOOT 0x21080
#define PEXOR_REP_TK_DSIZE 0x21090
#define PEXOR_TK_DSIZE_SEL 0x210a0

#define PEXOR_TK_MEM_SIZE 0x210b0

#define PEXOR_PROG_VERSION 0x211fc

#define PEXOR_TK_MEM 0x28000

#define PEXOR_TK_MEM_0 0x100000
#define PEXOR_TK_MEM_1 0x140000
#define PEXOR_TK_MEM_2 0x180000
#define PEXOR_TK_MEM_3 0x1c0000

// Packet types
#define PEXOR_PT_AD_R_REQ 0x240
#define PEXOR_PT_AD_W_REQ 0x644

//#define PEXOR_PT_TK_R_REQ 0xA00   //#define PEXOR_PT_TK_R_REQ 0xA44
#define PEXOR_PT_TK_R_REQ 0xA11
#define PEXOR_INI_REQ 0x344
#define PEXOR_RST_REQ 0x744

#define PEXOR_PT_AD_R_REP 0x044
#define PEXOR_PT_AD_W_REP 0x440
#define PEXOR_PT_TK_R_REP 0x844
#define PEXOR_PT_INI_REP 0x244
#define PEXOR_PT_ERR_REP 0x544


// address map for slave

/* #define REG_MODID     0x3FE0 */
/* #define REG_HEADER    0x3FE4 */
/* #define REG_FOOTER    0x3FE8 */
/* #define REG_DATA_LEN  0x3FEC */

/* #define REG_VERSION 0x3FFC */

#define REG_BUF0_DATA_LEN     0xFFFD00  // buffer 0 submemory data length
#define REG_BUF1_DATA_LEN     0xFFFE00  // buffer 1 submemory data length


#define REG_DATA_REDUCTION  0xFFFFB0  // Nth bit = 1 enable data reduction of  Nth channel from block transfer readout. (bit0:time, bit1-8:adc)
#define REG_MEM_DISABLE     0xFFFFB4  // Nth bit =1  disable Nth channel from block transfer readout.(bit0:time, bit1-8:adc)
#define REG_MEM_FLAG_0      0xFFFFB8  // read only:
#define REG_MEM_FLAG_1      0xFFFFBc  // read only:


#define REG_BUF0     0xFFFFD0 // base address for buffer 0 : 0x0000
#define REG_BUF1     0xFFFFD4  // base address for buffer 1 : 0x20000
#define REG_SUBMEM_NUM   0xFFFFD8 //num of channels 8
#define REG_SUBMEM_OFF   0xFFFFDC // offset of channels 0x4000

#define REG_MODID     0xFFFFE0
#define REG_HEADER    0xFFFFE4
#define REG_FOOTER    0xFFFFE8
#define REG_DATA_LEN  0xFFFFEC

#define REG_RST 0xFFFFF4
#define REG_LED 0xFFFFF8
#define REG_VERSION 0xFFFFFC



// registers should be 32 bit size also on 64 bit architecture: JAM64
typedef struct
{
  char *name;

  int volatile *pexor_base;
  int volatile *req_comm;
  int volatile *req_addr;
  int volatile *req_data;
  int volatile *rep_stat;
  int volatile *rep_clr;
  int volatile *rep_stat_0;
  int volatile *rep_stat_1;
  int volatile *rep_stat_2;
  int volatile *rep_stat_3;

  int volatile *rep_addr_0;
  int volatile *rep_addr_1;
  int volatile *rep_addr_2;
  int volatile *rep_addr_3;

  int volatile *rep_data_0;
  int volatile *rep_data_1;
  int volatile *rep_data_2;
  int volatile *rep_data_3;

  int volatile *rx_moni;
  int volatile *rx_rst;
  int volatile *sfp_disa;
  int volatile *sfp_fault;
  int volatile *sfp_fifo;
  int volatile *sfp_tk_stat, *sfp_tk_head, *sfp_tk_foot;
  int volatile *sfp_tk_dsize, *sfp_tk_sel;
  int volatile *tk_mem_size;
  int volatile *tk_mem_0,*tk_mem_1,*tk_mem_2,*tk_mem_3;
  int volatile *tk_mem;

  int volatile *pexor_version;
} s_pexor ;


int PEXOR_GetPointer( unsigned long PEXOR_BASE_OFF, volatile int *pl_virt_sram, s_pexor *ps_pexor )
{
  unsigned long ph_pexor_seq= PEXOR_BASE_OFF + (unsigned long) pl_virt_sram;

  ps_pexor->pexor_base   = ( volatile int *) (ph_pexor_seq + (unsigned long)PEXOR_BASE );
  ps_pexor->req_comm     = ( volatile int *) (ph_pexor_seq + (unsigned long)PEXOR_REQ_COMM );
  ps_pexor->req_addr     = ( volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REQ_ADDR);
  ps_pexor->req_data     = ( volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REQ_DATA);
  ps_pexor->rep_stat     = ( volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_STAT);
  ps_pexor->rep_clr      = ( volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_CLR);
  ps_pexor->rep_stat_0   = ( volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_STAT_0);
  ps_pexor->rep_stat_1   = ( volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_STAT_1);
  ps_pexor->rep_stat_2   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_STAT_2);
  ps_pexor->rep_stat_3   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_STAT_3);
                          
  ps_pexor->rep_addr_0   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_ADDR_0);
  ps_pexor->rep_addr_1   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_ADDR_1);
  ps_pexor->rep_addr_2   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_ADDR_2);
  ps_pexor->rep_addr_3   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_ADDR_3);
                          
  ps_pexor->rep_data_0   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_DATA_0);
  ps_pexor->rep_data_1   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_DATA_1);
  ps_pexor->rep_data_2   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_DATA_2);
  ps_pexor->rep_data_3   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_DATA_3);
                          
  ps_pexor->rx_moni      = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_RX_MONI);
  ps_pexor->rx_rst       = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_RX_RST);
  ps_pexor->sfp_disa     = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_SFP_DISA);
  ps_pexor->sfp_fault    = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_SFP_FAULT);
  ps_pexor->sfp_fifo     = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_SFP_FIFO);
  ps_pexor->sfp_tk_stat  = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_TK_STAT);
  ps_pexor->sfp_tk_head  = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_TK_HEAD);
  ps_pexor->sfp_tk_foot  = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_TK_FOOT);
  ps_pexor->sfp_tk_dsize = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_REP_TK_DSIZE);
  ps_pexor->sfp_tk_sel   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_TK_DSIZE_SEL);
  ps_pexor->sfp_tk_sel   = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_TK_DSIZE_SEL);

  ps_pexor->tk_mem_size  = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_TK_MEM_SIZE);

  ps_pexor->tk_mem_0     = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_TK_MEM_0);
  ps_pexor->tk_mem_1     = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_TK_MEM_1);
  ps_pexor->tk_mem_2     = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_TK_MEM_2);
  ps_pexor->tk_mem_3     = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_TK_MEM_3);

  ps_pexor->pexor_version    = (volatile int *) (ph_pexor_seq +  (unsigned long) PEXOR_PROG_VERSION);
  return(1);
}

int PEXOR_Read (s_pexor *ps_pexor, long *addr, long *data)
{
  *data =  * ( ps_pexor->pexor_base + (*addr >>2) );
  //  printf ("PEXOR_Read: 0x%x  0x%x \n",(ps_pexor->pexor_base),*(ps_pexor->pexor_base) );
  //  printf ("PEXOR_Read: 0x%x  0x%x \n",(ps_pexor->pexor_base+(addr>>2)),*(ps_pexor->pexor_base+(addr>>2) ) );
}

int PEXOR_Slave_Read (s_pexor *ps_pexor, long l_sfp, long l_slave, long l_addr, long *data)
{
  long l_comm, l_address;
  long a,b,c;
  int status;

  status=1;
  l_comm = PEXOR_PT_AD_R_REQ | (0x1<<16+l_sfp);
  l_address = l_addr+ (l_slave << 24);

  PEXOR_RX_Clear_Ch( *ps_pexor, l_sfp); 

  PEXOR_TX( ps_pexor, l_comm, l_address, 0x0) ; 
  if (PEXOR_RX( ps_pexor, l_sfp, &a , &b, &c)==1)
  {
    #ifdef DEBUG
    printf ("PEXOR_Slave_Read: Reply to PEXOR from SFP: 0x%x ", l_sfp);
    printf (" 0x%x 0x%x 0x%x \n", a,b,c);
    #endif
    *data = c;
    if( (a&0xfff) == PEXOR_PT_AD_R_REP)
    {
      if(a&0x4000!=0)
      {
        printf ("PEXOR_Slave_Read: ERROR: Packet Structure : Command Reply 0x%x \n", a);
        status=-1;
      }
    }
    else
    {
      printf ("PEXOR_Slave_Read: ERROR : Access to empty slave or address: Module  0x%x Address 0x%x  Command Reply  0x%x \n",
                                                                      ( l_address&0xff000000) >> 24 , l_address&0xffffff, a );
      status=-1;
    }
  }
  else
  {
    status=-1;
    printf ("PEXOR_Slave_Read: no reply: 0x%x 0x%x 0x%x \n", a,b,c);
  }
  return status;
}

int PEXOR_Slave_Write (s_pexor *ps_pexor, long l_sfp, long l_slave, long l_addr, long l_data)
{
  long l_comm, l_address;
  long a,b,c;
  int status;

  status=1;
  l_comm = PEXOR_PT_AD_W_REQ | (0x1<<16+l_sfp);
  l_address = l_addr+ (l_slave << 24);

  PEXOR_RX_Clear( *ps_pexor); 

  PEXOR_TX( ps_pexor, l_comm, l_address, l_data) ; 
  if(PEXOR_RX( ps_pexor, l_sfp, &a , &b, &c)==1)
  {
    #ifdef DEBUG
    printf ("PEXOR_Slave_Write: Reply to PEXOR from SFP: 0x%x ", l_sfp);
    printf (" 0x%x 0x%x 0x%x \n", a,b,c);
    #endif
    if( (a&0xfff) == PEXOR_PT_AD_W_REP)
    {
      if(a&0x4000!=0)
      {
        printf ("PEXOR_Slave_Write: ERROR: Packet Structure : Command Reply 0x%x \n", a);
        status=-1;
      }
    }
    else
    {
      printf ("PEXOR_Slave_Write: ERROR : Access to empty slave or address: Module  0x%x Address 0x%x  Command Reply  0x%x \n",
                                                                       ( l_address&0xff000000) >> 24 , l_address&0xffffff, a );
      status=-1;
    }
  }
  else
  {
    status=-1;
    printf ("PEXOR_Slave_Write: no reply: 0x%x 0x%x 0x%x \n", a,b,c);
  }
  return status;
}

int PEXOR_TK_TX_Mem_Read (s_pexor *ps_pexor, long *addr, long *data)
{
  *data =  * ( ps_pexor->tk_mem + (*addr >>2) );
  //  printf ("PEXOR_Read: 0x%x  0x%x \n",(ps_pexor->tk_mem),*(ps_pexor->tk_mem) );
  //  printf ("PEXOR_Read: 0x%x  0x%x \n",(ps_pexor->tk_mem+(addr>>2)),*(ps_pexor->tk_mem+(addr>>2) ) );
}

int PEXOR_TK_Mem_Write (s_pexor *ps_pexor, long *addr, long *data)
{
   *( ps_pexor->tk_mem + (*addr >>2) )= *data;
  //  printf ("PEXOR_Read: 0x%x  0x%x \n",(ps_pexor->pexor_base),*(ps_pexor->pexor_base) );
  //  printf ("PEXOR_Read: 0x%x  0x%x \n",(ps_pexor->pexor_base+(addr>>2)),*(ps_pexor->pexor_base+(addr>>2) ) );
}

#ifdef USE_KINPEX_V5
int PEXOR_RX_Clear (s_pexor *ps_pexor)
{
  *ps_pexor->rep_clr=0xf;
  return(1);
}
#else //USE_KINPEX_V5
int PEXOR_RX_Clear (s_pexor *ps_pexor)
{
  //  while( (*ps_pexor->rep_stat&0xcccc)!=0x0 ){
  while( (*ps_pexor->rep_stat)!=0x0 )
  {
    *ps_pexor->rep_clr=0xf;
    //    sleep(1);
    #ifdef DEBUG
    //  printf ("PEXOR_RX_Clear: rep_stat: 0x%x 0x%x \n",ps_pexor->rep_stat, *ps_pexor->rep_stat );
    #endif
  }
  return(1);
}
#endif//USE_KINPEX_V5

#ifdef USE_KINPEX_V5
int PEXOR_RX_Clear_Ch (s_pexor *ps_pexor, long ch)
{
  long val;
  val = 0x1<<ch;
  *ps_pexor->rep_clr=val;
  return(1);
}
#else //USE_KINPEX_V5
int PEXOR_RX_Clear_Ch (s_pexor *ps_pexor, long ch)
{
  long val;
  val = 0x1<<ch;
  while ((*(ps_pexor->rep_stat_0+ch)&0xf000)!=0x0 || (*(ps_pexor->sfp_tk_stat+ch)&0xf000)!=0x0 )
  {
    // while( (*ps_pexor->rep_stat)!=0x0 ){
    // *ps_pexor->rep_clr=0xf;
    *ps_pexor->rep_clr=val;
    // sleep(1);
    #ifdef DEBUG
    // printf ("PEXOR_RX_Clear_Ch: rep_stat: ch 0x%x 0x%x  0x%x , registers 0x%p and 0x%p\n",ch,
    // *(ps_pexor->sfp_tk_stat+ch), *(ps_pexor->rep_stat_0+ch), ps_pexor->sfp_tk_stat+ch, ps_pexor->rep_stat_0+ch);
    #endif
  }
  return(1);
}
#endif//USE_KINPEX_V5



#ifdef USE_KINPEX_V5
int PEXOR_RX_Clear_Pattern (s_pexor *ps_pexor, long l_ptn)
{
  long mask;
  long i=0;
    *ps_pexor->rep_clr=l_ptn;
    //#ifdef DEBUG
    //        printf ("PEXOR_RX_Clear: rep_stat: 0x%x 0x%x \n",ps_pexor->rep_stat, *ps_pexor->rep_stat );
    //#endif
  return(1);
}
#else //USE_KINPEX_V5
int PEXOR_RX_Clear_Pattern (s_pexor *ps_pexor, long l_ptn)
{
  long mask;
  long i=0;
  //  mask=(l_ptn<<8)|(l_ptn<<4)|l_ptn;
  //    printf ("PEXOR_RX_Clear: rep_stat: 0x%x 0x%x \n",ps_pexor->rep_stat, *ps_pexor->rep_stat );
  //  while( (*ps_pexor->rep_stat&0xcccc)!=0x0 ){
  //  i=0;
  //  while( ((*ps_pexor->rep_stat)&mask)!=0x0 )
  //  {
    *ps_pexor->rep_clr=l_ptn;
    // sleep(1);
    //#ifdef DEBUG
    //    printf ("PEXOR_RX_Clear: rep_stat: 0x%x 0x%x \n",ps_pexor->rep_stat, *ps_pexor->rep_stat );
    //#endif
    //    i++;
    //  }
    if  ( (*ps_pexor->rep_stat&0xcccc)!=0x0 ){
      printf ("PEXOR_RX_Clear: rep_stat: 0x%x 0x%x \n",ps_pexor->rep_stat, *ps_pexor->rep_stat );
    }
  return(1);
}
#endif//USE_KINPEX_V5



int PEXOR_TX_Reset_Ch (s_pexor *ps_pexor, long ch)
{
  long val;
  val = 0x1<<(ch+4);
  *ps_pexor->rx_rst= val;
  return(1);
}

int PEXOR_SERDES_Reset( s_pexor *ps_pexor)
{
  long val;
  *ps_pexor->rx_rst= 0x100;
  *ps_pexor->rx_rst= 0x0;
  sleep(1);
  return(1);
}


#ifdef USE_KINPEX_V5
int PEXOR_TX( s_pexor *ps_pexor,  long comm, long addr, long data )
{
  if( (comm&0xfff) == PEXOR_PT_TK_R_REQ ) 
    {
      *ps_pexor->req_comm = comm | (data&0xf) << 20 | (addr&0x3) << 24 | 1<<28 ;
      //                  printf ("PEXOR_TX: req_comm  %x \n", *ps_pexor->req_comm);
    }
  else
    {
      *ps_pexor->req_addr = addr;
      *ps_pexor->req_data = data;
      *ps_pexor->req_comm = comm;
    }
  return(1);
}

#else //USE_KINPEX_V5
int PEXOR_TX( s_pexor *ps_pexor,  long comm, long addr, long data )
{
  *ps_pexor->req_addr = addr;
  *ps_pexor->req_data = data;
  *ps_pexor->req_comm = comm;
  return(1);
}
#endif //USE_KINPEX_V5


#ifdef USE_KINPEX_V5
int PEXOR_RX( s_pexor *ps_pexor, int sfp_id,  long *comm, long *addr, long *data )
{
  int stat;
  int loop=0;
  int loop_max=1000000;
  //  INTU4 rep_stat;

  stat=-1;
  if (sfp_id==0)
    {
    while( (((*comm = *ps_pexor->rep_stat_0) & 0x3000)>>12)!=2 && loop < loop_max)
    {
      loop++;
    }
    if( (*comm&0xfff) == PEXOR_PT_TK_R_REQ )
      {
	*addr =  (*comm & 0xf000000)>>24;
	*data =  (*comm & 0xf0000)>>16;
      }
    else
      {
      *addr =  *ps_pexor->rep_addr_0;
      *data =  *ps_pexor->rep_data_0;
      }
    }
    else if (sfp_id==1)
    {
    while( (((*comm = *ps_pexor->rep_stat_1) & 0x3000)>>12)!=2 && loop < loop_max)
    {
      loop++;
    }
    if( (*comm&0xfff) == PEXOR_PT_TK_R_REQ )
      {
	*addr =  (*comm & 0xf000000)>>24;
	*data =  (*comm & 0xf0000)>>16;
      }
    else
      {
      *addr =  *ps_pexor->rep_addr_1;
      *data =  *ps_pexor->rep_data_1;
      }
    }

    else if (sfp_id==2)
    {
    while( (((*comm = *ps_pexor->rep_stat_2) & 0x3000)>>12)!=2 && loop < loop_max)
    {
      loop++;
    }
    if( (*comm&0xfff) == PEXOR_PT_TK_R_REQ )
      {
	*addr =  (*comm & 0xf000000)>>24;
	*data =  (*comm & 0xf0000)>>16;
      }
    else
      {
      *addr =  *ps_pexor->rep_addr_2;
      *data =  *ps_pexor->rep_data_2;
      }
    }
    else if (sfp_id==3)
    {
    while( (((*comm = *ps_pexor->rep_stat_3) & 0x3000)>>12)!=2 && loop < loop_max)
    {
      loop++;
    }
    if( (*comm&0xfff) == PEXOR_PT_TK_R_REQ )
      {
	*addr =  (*comm & 0xf000000)>>24;
	*data =  (*comm & 0xf0000)>>16;
      }
    else
      {
      *addr =  *ps_pexor->rep_addr_3;
      *data =  *ps_pexor->rep_data_3;
      }
    }

  if(loop!=loop_max) stat =1;
  return(stat);
}

#else //USE_KINPEX_V5
int PEXOR_RX( s_pexor *ps_pexor, int sfp_id,  long *comm, long *addr, long *data )
{
  int stat;
  int loop=0;
  int loop_max=1000000;

  stat=-1;
  if (sfp_id==0)
  {
    while( ((*ps_pexor->rep_stat_0 & 0x3000)>>12)!=2 && loop < loop_max)
    {
      // sleep(1);
      loop++;
      //      printf ("PEXOR_RX: rep_stat: sfp0:0x%x  loop %d \n", *ps_pexor->rep_stat_0, loop);
    }
    //    usleep(1);
    *comm =  *ps_pexor->rep_stat_0;
    *addr =  *ps_pexor->rep_addr_0;
    *data =  *ps_pexor->rep_data_0;

    }
    else if (sfp_id==1)
    {
      while( ((*ps_pexor->rep_stat_1 & 0x3000)>>12)!=2  && loop < loop_max )
      {
        // sleep(1);
        loop++;
        // printf ("PEXOR_RX: rep_stat: sfp1:0x%x \n", *ps_pexor->rep_stat_1);
      }
      *comm =  *ps_pexor->rep_stat_1;
      *addr =  *ps_pexor->rep_addr_1;
      *data =  *ps_pexor->rep_data_1;
    }
  else if (sfp_id==2)
  {
    while( ((*ps_pexor->rep_stat_2 & 0x3000)>>12)!=2   && loop < loop_max)
    {
      // sleep(1);
      loop++;
      //  printf ("PEXOR_RX: rep_stat: sfp2:0x%x \n", *ps_pexor->rep_stat_2);
    }
    *comm =  *ps_pexor->rep_stat_2;
    *addr =  *ps_pexor->rep_addr_2;
    *data =  *ps_pexor->rep_data_2;
    }
    else if (sfp_id==3){
    while( ((*ps_pexor->rep_stat_3 & 0x3000)>>12)!=2   && loop < loop_max)
    {
      // sleep(1);
      loop++;
      // printf ("PEXOR_RX: rep_stat: sfp3:0x%x \n", *ps_pexor->rep_stat_3);
    }
    *comm =  *ps_pexor->rep_stat_3;
    *addr =  *ps_pexor->rep_addr_3;
    *data =  *ps_pexor->rep_data_3;
  }
  if(loop!=loop_max) stat =1;
  return(stat);
}
#endif


long PEXOR_TK_Data_Size (s_pexor *ps_pexor, long l_sfp,  long slave_id)
{
  long comm, addr, data;

  *(ps_pexor->sfp_tk_sel+l_sfp) = slave_id;
  return(  *(ps_pexor->sfp_tk_dsize + l_sfp) );
}

long PEXOR_TK_Mem_Size (s_pexor *ps_pexor, long l_sfp)
{
  long comm, addr, data;
  return ( *(ps_pexor->tk_mem_size+l_sfp)) ;
}

long PEXOR_TK_Mem_Read (s_pexor *ps_pexor, long l_sfp, long **pl_dat)
{
  long mem_size, mem_size_32b;
  long MEM_SIZE_MAX=0x10000;
  long *pl_dat_start;
  int i;

  pl_dat_start = *pl_dat;
  mem_size=*(ps_pexor->tk_mem_size+l_sfp);
  printf ("PEXOR_TK_Mem_Read:  %x \n", mem_size);

  mem_size_32b = (mem_size>>2);
  if(mem_size > MEM_SIZE_MAX)
  {
    printf ("PEXOR_TK_Mem_Read: sfp x%x  memory overflow!! (size = %x ) \n", l_sfp, mem_size);
  }
  else
  {
    if (l_sfp==0)
    {
      for (i=0; i < mem_size_32b ; i++)  *((*pl_dat)++)=*(ps_pexor->tk_mem_0 +i);
    }
    else if (l_sfp==1)
    {
      for (i=0; i < mem_size_32b ; i++)  *((*pl_dat)++)=*(ps_pexor->tk_mem_1 +i);
    }
    else if (l_sfp==2)
    {
      for (i=0; i < mem_size_32b ; i++)  *((*pl_dat)++)=*(ps_pexor->tk_mem_2 +i);
    }
    else if (l_sfp==3)
    {
      for (i=0; i < mem_size_32b ; i++) *((*pl_dat)++)=*(ps_pexor->tk_mem_3 +i);
    }
  }
  *pl_dat=pl_dat_start;
  return (mem_size ) ;
}


int PEXOR_RX_Status (s_pexor *ps_pexor)
{
  //  printf ("0x%x 0x%x \n", ps_pexor->rep_stat,*ps_pexor->rep_stat);
  printf ("PEXOR_RX_Status: rep_stat: sfp0:0x%x sfp1:0x%x sfp2:0x%x sfp3:0x%x \n",
  *ps_pexor->rep_stat_0,*ps_pexor->rep_stat_1 ,
  *ps_pexor->rep_stat_2,*ps_pexor->rep_stat_3);
  return(1);
}

int PEXOR_Port_Monitor (s_pexor *ps_pexor)
{
  long tmp, fault, moni;
  int flag=-1;
  int loop=0;
  int loopmax=10;
  //  printf ("0x%x 0x%x \n", ps_pexor->rep_stat,*ps_pexor->rep_stat);
  while((loop < loopmax)&&(flag==-1))
  {
    tmp=~(*ps_pexor->sfp_fault);
    fault=(tmp&0x1)*0xff+(tmp&0x2)*0xff00/2+(tmp&0x4)*0xff0000/4+(tmp&0x8)*0xff000000/8;
    moni=(*ps_pexor->rx_moni)&fault;

    if( (  ( (moni&0xff)!=0) && ((moni&0xff)!=0xbc))||( ((moni&0xff00)!=0) && ((moni&0xff00)!=0xbc00) ) ||
    ( ((moni&0xff0000)!=0) && ((moni&0xff0000)!=0xbc0000) ) ||( ((moni&0xff000000)!=0) &&( (moni&0xff000000)!=0xbc000000)) )
    {
      PEXOR_SERDES_Reset(ps_pexor);
    }
    else
    {
      flag=1;
    }
  }
     
  if(flag==-1)
  {
    printf ("PEXOR_RX_Monitor Error: loop %d 0x%8x 0x%8x 0x%8x  \n",loop, fault, moni ,moni&fault);
  }
  return(flag);
}


int PEXOR_Show_Version (s_pexor *ps_pexor)
{
  long tmp, year,month,day,version[2];

  tmp=(*ps_pexor-> pexor_version);
  year=((tmp&0xff000000)>>24)+0x2000;
  month=(tmp&0xff0000)>>16;
  day=(tmp&0xff00)>>8;
  version[0]=(tmp&0xf0)>>4;
  version[1]=(tmp&0xf);

  printf ("PEXOR program compiled at Year=%x Month=%x Date=%x Version=%x.%x \n", year,month,day,version[0],version[1]);
  return(1);
}

int PEXOR_SFP_Active (s_pexor *ps_pexor, long l_sfp)
{
  long l_comm;
  long l_slave=100;
  long a,b,c;
  long l_num;
  long active;
  long loop;
  long loopmax=10;

  active=((~*ps_pexor->sfp_fault)>>l_sfp)&0x1;
  l_num=-1;
  loop=0;
  if(active==1&&loop<loopmax)
  {
    l_comm = PEXOR_INI_REQ| (0x1<<16+l_sfp);
    PEXOR_RX_Clear_Ch(ps_pexor, l_sfp); 
    while(l_num==-1)
    {
      PEXOR_TX(ps_pexor, l_comm, 0, l_slave) ;
      //  printf (" SFP%d: ",l_sfp);
      if(PEXOR_RX(ps_pexor, l_sfp, &a , &b, &c)==-1)
      {
        // printf ("no reply: 0x%x 0x%x 0x%x \n", a,b,c);
        l_num=-1;
      }
      else
      {
        // printf ("Reply: 0x%x 0x%x 0x%x \n", a,b,c);
        // printf ("     : No of slaves  : 0x%x\n", b);
        l_num=b;
      }
      loop++;
    }
  }
  //  printf (" SFP0 %x    : No of slaves  : %i \n", l_sfp, l_num);
  return(l_num);
}

int PEXOR_SFP_Disable (s_pexor *ps_pexor,  long disa)
{
  //  *ps_pexor->sfp_disa = disa;

  printf ("PEXOR_SFP_Disable: x%x  x%x  \n", ps_pexor->sfp_disa, *ps_pexor->sfp_disa);
  /*   printf ("PEXOR_SFP_-4 x%x  x%x  \n", (ps_pexor->sfp_fifo-4), *(ps_pexor->sfp_fault-4)); */
  /*   printf ("PEXOR_SFP_-3: x%x  x%x  \n", (ps_pexor->sfp_fifo-3), *(ps_pexor->sfp_fault-3)); */
  /*   printf ("PEXOR_SFP_-2: x%x  x%x  \n", (ps_pexor->sfp_fifo-2), *(ps_pexor->sfp_fault-2)); */
  /*   printf ("PEXOR_SFP_-1: x%x  x%x  \n", (ps_pexor->sfp_fifo-1), *(ps_pexor->sfp_fault-1)); */
  /*   printf ("PEXOR_SFP_FIFO: x%x  x%x  \n", ps_pexor->sfp_fifo, *ps_pexor->sfp_fifo); */
  return(1);
}

int PEXOR_SFP_Show_FIFO (s_pexor *ps_pexor, long ch)
{
  int i;
  long tmp;
  i=0;
  printf ("PEXOR_SFP_Show_FIFO: \n");
  tmp=*(ps_pexor->sfp_fifo+ch);
  while((tmp&0xf00)!=0x500)
  {
    printf ("x%x  x%2x \n", i, 0xfff&tmp);
    tmp=*(ps_pexor->sfp_fifo+ch);
    i++;
  }
  printf ("x%x  x%2x \n", i, 0xfff&tmp);
  return(1);
}

int PEXOR_SFP_Clear_FIFO (s_pexor *ps_pexor)
{
  *ps_pexor->sfp_fifo = 0xf;
  return(1);
}
  
int PEXOR_Set_LED (s_pexor *ps_pexor, long l_led)
{
  int i,j;
  long a;
  long l_slave[4];

  for(i=0;i<4;i++)
   {
    l_slave[i]=PEXOR_SFP_Active(ps_pexor, i);
    if(l_slave[i]>0)
    {
      printf ("SFP port: %i\n", i );
      for(j=0;j<l_slave[i];j++){
      PEXOR_Slave_Read( ps_pexor, i, j, REG_VERSION, &a );
      PEXOR_Slave_Write( ps_pexor, i, j, REG_LED, l_led );
      printf ("  Module ID: 0x%x Program Version: 0x%x LED off\n",j , a );
      }
    }
  }
}


int PEXOR_LED_On (s_pexor *ps_pexor)
{
  PEXOR_Set_LED( ps_pexor, 0x0 );
}

int PEXOR_LED_Off (s_pexor *ps_pexor)
{
  PEXOR_Set_LED (ps_pexor, 0x100);
}


int PEXOR_Set_Data_Reduction (s_pexor *ps_pexor, long l_flag)
{
  int i,j;
  long a;
  long l_slave[4];

  for(i=0;i<4;i++)
  {
    l_slave[i]=PEXOR_SFP_Active(ps_pexor, i);
    if(l_slave[i]>0)
    {
      #ifdef DEBUG
      printf ("SFP port: %i\n", i );
      #endif
      for(j=0;j<l_slave[i];j++)
      {
        PEXOR_Slave_Read( ps_pexor, i, j, REG_VERSION, &a );
        PEXOR_Slave_Write( ps_pexor, i, j, REG_DATA_REDUCTION, l_flag );
        #ifdef DEBUG
        printf ("  Module ID: 0x%x Program Version: 0x%x Data Reduction 0x%\n",j , l_flag );
        #endif
      }
    }
  }
}
#endif



