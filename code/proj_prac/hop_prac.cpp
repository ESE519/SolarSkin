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
#define MyOwnAddress 02
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
#define max_storage 10
uint8_t local_rx_buf,pkt_length[max_storage];
uint8_t length1,trans_flag=0,pkt_pointer=0,packet_in_que=0;
uint8_t pkt_store_data[max_storage][128];
uint8_t uniqueIDsRREQ[MaxuIDTrack] = {0};
uint8_t pkt_destination[max_storage];
nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);
void RouteDiscovery(void);
void RouteRequestRx(uint8_t *rreq, uint8_t len);
void RouteRequestTx(uint8_t *rreq,uint8_t len);
void TxQueueAdd(uint8_t px,uint8_t len,uint_8 dest);
void RxPacketProcess(uint8_t *pack,uint8_t len);
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
		    bmac_init (10);
			nrk_create_taskset();
		    nrk_start();
			return 0;

	}
void RxPacketProcess(uint8_t *pack,uint8_t len){
	uint8_t i, j, len2;
	len2 = pack[1]-1;
	uint8_t temp[len2];
	if(*pack != 0x7E){return;}
	j=5;
	temp[0]=pack[2];
	for(i=1;i<len2;i++){temp[i]=pack[j];j++;}
	switch(pack[3])
	{
		case RREQ:
			RouteRequestRx(temp,len2);
			break;
	    case default:
			break;
	}
	return();
}

void TxQueueAdd(uint8_t px,uint_8 len,uint8_t dest){
if(pkt_pointer>9){
pkt_pointer=0;
}
for(int i=0;i<len;i++){
pkt_store_data[pkt_pointer][i]=px[i];
}
pkt_destination[pkt_pointer]=dest;
pkt_length[pkt_pointer]=len;
pkt_pointer++;
packet_in_que++;

}
 void RouteRequestTx(uint8_t *rreq,uint8_t len){
	if(rreq[0]<2){return;}
	uint8_t temp[len+5],i,j;
	temp[0]=0x7E;
	temp[1]=len+2;
	temp[2]=rreq[0]-1;
	temp[3]=RREQ;
	j=1;
	for(i=4;i<len+3;i++){temp[i]=rreq[j];j++;}
	temp[j]=MyOwnAddress;
	j++;
	temp[j]=CRC;
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
		if(*uID == uniqueIDsRREQ[i]){return();}
	}
	//Add it to unique ID
	for(i=0;i<MaxuIDTrack;i++)
	{
		if(uniqueIDsRREQ[i]==0){uniqueIDsRREQ[i]=*uID;break;}
	}
	//Check if the packect is destined for ownself
	if(*rrDest == MyOwnAddress)
	{
		uint8_t temp[len+1];
		for(j=0;j<len;j++){temp[j]=rreq[j];}
		temp[j]=MyOwnAddress;
		RouteReplyTx(temp,len+1,0);
		return();
	}
	//Check for the cache to the destination existence 
	for(i=0;i<MaxHopsPossible;i++)
	{
		if(*rrDest == cache[i])
		{
			uint8_t temp[len+i+1];
			for(j=0;j<len;j++){temp[j]=rreq[j];}
			for(k=0;k<i+1;k++){temp[j]=cache[k];j++;}
			RouteReplyTx(temp,len+i+1,i);
			return();
		}
	}
	//broadcast the packet forward 
	RouteRequestTx(rreq,len);
	return;
}

void rx_task ()
{
  uint8_t i, len, rssi;
  int8_t val;
	//char *local_rx_buf;
  nrk_time_t check_period;
  printf ("rx_task PID=%d\r\n", nrk_get_pid ());
  bmac_set_cca_thresh(DEFAULT_BMAC_CCA); 
  bmac_rx_pkt_set_buffer ((char*)rx_buf, RF_MAX_PAYLOAD_SIZE);
  while (1) {
    // Wait until an RX packet is received
    val = bmac_wait_until_rx_pkt ();
		printf("Hi..\n");
    // Get the RX packet 
    nrk_led_set (ORANGE_LED);
    (char*)local_rx_buf = bmac_rx_pkt_get (&len, &rssi);
	for(i=0;i<len;i++){
	printf("%c",local_rx_buf[i]);
	}
	RxPacketProcess(local_rx_buf,len);
	nrk_led_clr (ORANGE_LED);
    bmac_rx_pkt_release ();
    nrk_wait_until_next_period ();
  }
}
uint8_t ctr_cnt[4];
void RouteDiscovery(void){
	uint8_t temp[2];
	temp[0]=TTL+1;
	temp[1]=Gateway;
	RouteRequestTx(temp,2);
	return();
}
void inter_tx_task ()
{
  uint8_t j, i, val, len, cnt;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  nrk_time_t r_period;
  while (!bmac_started ())
  nrk_wait_until_next_period ();
  tx_done_signal = bmac_get_tx_done_signal ();
  nrk_signal_register (tx_done_signal);
  cnt = 0;
  RouteDiscovery();
  while (1) {
  if(packet_in_que>0){
    bmac_addr_decode_disable();
    bmac_addr_decode_set_my_mac(MyOwnAddress);
	for(i=0;i<pkt_length[pkt_pointer-1];i++){
    tx_buf[i]=pkt_store_data[pkt_pointer-1][i];
	}
    nrk_led_set (GREEN_LED);
	bmac_auto_ack_enable();
	if(pkt_destination[pkt_pointer-1]==0xFF){
    bmac_addr_decode_dest_mac(0xFFFF);  // 0xFFFF is broadcast
	}
	else{
	bmac_addr_decode_dest_mac(pkt_destination[pkt_pointer-1]);
	}
    val=bmac_tx_pkt((char*)tx_buf,pkt_length[pkt_pointer-1] );
	packet_in_que--;
	cnt++;
    printf("Tx task sent data!\r\n");
    nrk_led_clr (GREEN_LED);
	printf("tx_task PID=%d\r\n", nrk_get_pid ());
    nrk_wait_until_next_period ();
  }
  }
}
void tx_task ()
{
  uint8_t j, i, val, len, cnt;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  nrk_time_t r_period;
  while (!bmac_started ())
    nrk_wait_until_next_period ();
  tx_done_signal = bmac_get_tx_done_signal ();
  nrk_signal_register (tx_done_signal);
  cnt = 0;
  
  while (1) {
   
	bmac_addr_decode_disable();
    bmac_addr_decode_set_my_mac(MyOwnAddress);
	tx_buf[0]=0x7E;
	tx_buf[1]=len;
	tx_buf[2]=MaxHopsPossible;//
	tx_buf[3]=RREQ;
	tx_buf[4]=	
    nrk_led_set (BLUE_LED);
	bmac_auto_ack_enable();
    bmac_addr_decode_dest_mac(DST_ADDR);  // 0xFFFF is broadcast
    val=bmac_tx_pkt(tx_buf, strlen(tx_buf));
		 printf("%c",val);
		 cnt++;
		 printf("TXSTAT: %d",mrf_read_short(TXSTAT));
		 printf("\r\n");
		 if(val!=NRK_OK) 
 		  printf("NO ack or Reserve Violated! \r\n");
   printf("Tx task sent data!\r\n");
    nrk_led_clr (BLUE_LED);
		printf("tx_task PID=%d\r\n", nrk_get_pid ());
    nrk_wait_until_next_period ();
  }

}

void nrk_create_taskset ()
{


  RX_TASK.task = rx_task;
  nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
  RX_TASK.prio = 2;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 1;
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
  TX_TASK.period.secs = 1;
  TX_TASK.period.nano_secs = 0;
  TX_TASK.cpu_reserve.secs = 1;
  TX_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
  TX_TASK.offset.secs = 0;
  TX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&TX_TASK);
	
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

  printf ("Create done\r\n");
}