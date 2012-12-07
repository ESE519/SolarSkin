#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_stack_check.h>
#include <nrk_stats.h>
#include <string.h>
#include "mbed.h"
#include "basic_rf.h"
#include "bmac.h"

// Only require MAC address for address decode 
#define MyOwnAddress 01
#define MaxHopsPossible 64
#define TTL 10
#define MaxuIDTrack 10
#define CRC 0
#define RREQ 0x01
#define RREP 0x02
#define RERR 0x03
#define DATA 0x04
#define DATR 0x05
#define RCUP 0x06
#define RSAL 0x07
#define DACK 0x08
#define Gateway 01
#define arraysize 128
#define max_storage 10

uint8_t pTxQ[max_storage][arraysize], pTxQlen[max_storage]={0}, pTxQdes[max_storage]={0}, pTxQsen[max_storage]={0};

uint8_t *local_rx_buf,pkt_length[max_storage];
uint8_t length1,trans_flag=0,pkt_pointer=0,packet_in_que=0,trans_ptr=0;
uint8_t pkt_store_data[max_storage][128];
uint8_t uniqueIDsRREQ[MaxuIDTrack] = {0};
uint8_t pkt_destination[max_storage];
uint8_t cache[MaxHopsPossible] = {0}; //store first as own address
nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);
void RouteRequestRx(uint8_t *rreq, uint8_t len);
void RouteRequestTx(uint8_t *rreq,uint8_t len);
void TxQueueAdd(uint8_t px,uint8_t len,uint8_t dest);
void RxPacketProcess(uint8_t *pack,uint8_t len);
void RouteReplyTx(uint8_t *rrep,uint8_t len, uint8_t timeOff);
void RouteReplyRx(uint8_t *rrep, uint8_t len);
void RouteDiscovery(void);
nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

nrk_task_type INTER_TX_TASK;
NRK_STK inter_tx_task_stack[NRK_APP_STACKSIZE];
void inter_tx_task (void);

void nrk_create_taskset ();

uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];

int main(void)

  {
	    nrk_setup_ports();
			nrk_init();
			bmac_task_config();
		  bmac_init (1);
			nrk_create_taskset();
		  nrk_start();
			return 0;

	}
void RxPacketProcess(uint8_t *pack,uint8_t len){
	uint8_t i, j, len2;
	len2 = pack[1]-1;
	uint8_t temp[arraysize];
	if(*pack != 0x7E){return;}
	j=4;
	temp[0]=pack[2];
	for(i=1;i<len2;i++){temp[i]=pack[j];j++;}
	switch(pack[3])
	{
		case RREQ:
			RouteRequestRx(temp,len2);
			break;
		case RREP:
			RouteReplyRx(temp,len2);	break;
	    default:
			break;
	}
	return;
}

void TxQueueAdd(uint8_t *px,uint8_t len,uint8_t dest){
	uint8_t i,j;
	for(i=0;i<max_storage;i++){pTxQsen[i]==0;break;}
	if(i==10){putchar(0x42);return;}
	for(j=0;j<len;j++){pTxQ[i][j]=px[j];}
	pTxQdes[i]=dest;
	pTxQlen[i]=len;
	pTxQsen[i]=1;
	return;
}
 void RouteRequestTx(uint8_t *rreq,uint8_t len){
	if(rreq[0]<2){return;}
	uint8_t temp[arraysize],i,j;
	temp[0]=0x7E;
	temp[1]=len+2;
	temp[2]=rreq[0]-1;
	temp[3]=RREQ;
	j=1;
	for(i=4;i<len+3;i++){temp[i]=rreq[j];j++;}
	temp[i]=MyOwnAddress;
	i++;
	temp[i]=CRC;
	TxQueueAdd(temp,len+5,0xFF);
	return;
}
void RouteRequestRx(uint8_t *rreq, uint8_t len){
	uint8_t i,j,k;
	uint8_t *uID, *rrDest;

	uID     = rreq + 2 ;
	rrDest  = rreq + 1 ;

	//Check if the unique ID packet has already been seen recently.
	for(i=0;i<MaxuIDTrack;i++)
	{
		if(*uID == uniqueIDsRREQ[i]){}
   //{return;}
	}
	//Add it to unique ID
	for(i=0;i<MaxuIDTrack;i++)
	{
		if(uniqueIDsRREQ[i]==0){uniqueIDsRREQ[i]=*uID;break;}
	}
	//Check if the packect is destined for ownself
	if(*rrDest == MyOwnAddress)
	{
		uint8_t temp[arraysize];
		temp[0]=TTL+1;
		for(j=1;j<len;j++){temp[j]=rreq[j];}
		temp[j]=MyOwnAddress;
		RouteReplyTx(temp,len+1,0);
		return;
	}
	//Check for the cache to the destination existence 
	for(i=0;i<MaxHopsPossible;i++)
	{
		if(*rrDest == cache[i])
		{
			uint8_t temp[arraysize];
			
			for(j=0;j<len;j++){temp[j]=rreq[j];}
			for(k=0;k<i+1;k++){temp[j]=cache[k];j++;}
			RouteReplyTx(temp,len+i+1,i);
			return;
		}
	}
	//broadcast the packet forward 
	RouteRequestTx(rreq,len);
	return;
}
void RouteReplyRx(uint8_t *rrep, uint8_t len){
	uint8_t i,j,k ;
	i=1;
	while(rrep[i] != MyOwnAddress){i++;}
	if(i>1){RouteReplyTx(rrep,len,0);}
	for(j=0;j<len-i;j++){cache[j]=rrep[i];i++;}
	return;
}

void RouteReplyTx(uint8_t *rrep,uint8_t len, uint8_t timeOff){
	if(rrep[0]<2){return;}
	uint8_t temp[arraysize],i,j;
	temp[0]=0x7E;
	temp[1]=len;
	temp[2]=rrep[0]-1;
	temp[3]=RREP;
	j=2;
	for(i=4;i<len+2;i++){temp[i]=rrep[j];j++;}
	temp[i]=CRC;
	i=4;
	while(temp[i] != MyOwnAddress){i++;}
  TxQueueAdd(temp,len+3,temp[i-1]);
	return;
}

void RouteDiscovery(void){
	uint8_t temp[2];
	temp[0]=TTL+1;
	temp[1]=Gateway;
	RouteRequestTx(temp,2);
	return;
}

void rx_task ()
{
  uint8_t i, len, rssi;
  int8_t val;
  nrk_time_t check_period;
	bmac_set_cca_thresh(DEFAULT_BMAC_CCA); 
  bmac_rx_pkt_set_buffer ((char*)rx_buf, RF_MAX_PAYLOAD_SIZE);
  while (1) {
		val = bmac_wait_until_rx_pkt ();
		nrk_led_set (ORANGE_LED);
    (char*)local_rx_buf = bmac_rx_pkt_get (&len, &rssi);
		for(i=0;i<len;i++){putchar(local_rx_buf[i]);}
		RxPacketProcess(local_rx_buf,len);
		nrk_led_clr(ORANGE_LED);
    bmac_rx_pkt_release ();
    nrk_wait_until_next_period ();
  }
}
void tx_task (){
	uint8_t j, i, val, len, cnt;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  nrk_time_t r_period;
  while (!bmac_started ())
  nrk_wait_until_next_period ();
  tx_done_signal = bmac_get_tx_done_signal ();
  nrk_signal_register (tx_done_signal);
  cnt = 0;
  bmac_addr_decode_enable();
  bmac_addr_decode_set_my_mac(MyOwnAddress);
	
	while (1) {
		//DataInitiate();
		nrk_led_set (GREEN_LED);
		for(i=0;i<max_storage;i++){
			bmac_addr_decode_enable();
			bmac_addr_decode_set_my_mac(MyOwnAddress);
			bmac_auto_ack_enable();
			if(pTxQsen[i]!=0){
				bmac_addr_decode_enable();
				bmac_addr_decode_set_my_mac(MyOwnAddress);
				bmac_auto_ack_disable();
				for(j=0;j<pTxQlen[i];j++){tx_buf[j]=pTxQ[i][j];}
				if(pTxQdes[i]==0xFF){bmac_addr_decode_dest_mac(0xFFFF);}
				else{bmac_addr_decode_dest_mac(pTxQdes[i]);}
				val=bmac_tx_pkt((char*)tx_buf,pTxQlen[i]);
				pTxQsen[i] = 0;
				pTxQdes[i] = 0;
				pTxQlen[i] = 0; 
			}
		}
		nrk_led_clr (GREEN_LED);
		nrk_wait_until_next_period ();
	}
}
void nrk_create_taskset (){


  RX_TASK.task = rx_task;
  nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
  RX_TASK.prio = 3;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 2;
  RX_TASK.period.nano_secs = 0;
  RX_TASK.cpu_reserve.secs = 1;
  RX_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
  RX_TASK.offset.secs = 0;
  RX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RX_TASK);

  TX_TASK.task = tx_task;
  nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
  TX_TASK.prio = 2;
  TX_TASK.FirstActivation = TRUE;
  TX_TASK.Type = BASIC_TASK;
  TX_TASK.SchType = PREEMPTIVE;
  TX_TASK.period.secs = 5;
  TX_TASK.period.nano_secs = 0;
  TX_TASK.cpu_reserve.secs = 1;
  TX_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
  TX_TASK.offset.secs = 0;
  TX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&TX_TASK);
	/*
	INTER_TX_TASK.task = inter_tx_task;
  nrk_task_set_stk( &INTER_TX_TASK, inter_tx_task_stack, NRK_APP_STACKSIZE);
  INTER_TX_TASK.prio = 2;
  INTER_TX_TASK.FirstActivation = TRUE;
  INTER_TX_TASK.Type = BASIC_TASK;
  INTER_TX_TASK.SchType = PREEMPTIVE;
  INTER_TX_TASK.period.secs = 1;
  INTER_TX_TASK.period.nano_secs = 0;
  INTER_TX_TASK.cpu_reserve.secs = 1;
  INTER_TX_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
  INTER_TX_TASK.offset.secs = 0;
  INTER_TX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&INTER_TX_TASK);
*/
  //printf ("Create done\r\n");
}