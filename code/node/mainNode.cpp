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
#define MyOwnAddress 0x09
#define MaxHopsPossible 10
#define TTL 5
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
#define Gateway 0x01
#define szMax 32
#define queMax 10
#define RSSIThresh 180

uint8_t pLen[queMax]={0},ipLen[queMax]={0},pDes[queMax]={0},ipDes[queMax]={0},pPtr=0,ipPtr=0;
uint8_t pDat[queMax][szMax]={0},ipDat[queMax][szMax]={0};
uint8_t pQue=0,txPtr=0,ipQue=0,itxPtr=0,iFlag=0;
uint8_t uniqueIDsRREQ[MaxuIDTrack] = {0};
uint8_t cache[MaxHopsPossible] = {0}; //store first as own address

void RouteRequestRx(uint8_t *rreq, uint8_t len);
void RouteRequestTx(uint8_t *rreq,uint8_t len);
void TxQueueAdd(uint8_t px,uint8_t len,uint8_t dest);
void InterTxQueueAdd(uint8_t *px,uint8_t len,uint8_t dest);
void RxPacketProcess(uint8_t *pack,uint8_t len);
void RouteReplyTx(uint8_t *rrep,uint8_t len, uint8_t timeOff);
void RouteReplyRx(uint8_t *rrep, uint8_t len);
void RouteDiscovery();
void DataInitiate();
void DataTx(uint8_t *data,uint8_t len, uint8_t flag);
void DataRx(uint8_t *data, uint8_t len);

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task(void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

nrk_task_type INTER_TX_TASK;
NRK_STK inter_tx_task_stack[NRK_APP_STACKSIZE];
void inter_tx_task (void);

void nrk_create_taskset();

uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];

int main(void){
	cache[0]=0x09;
	cache[1]=0x07;
	cache[2]=0x08;
	cache[3]=0x06;
	cache[4]=0x01;
	
	nrk_setup_ports();
	nrk_init();
	bmac_task_config();
	bmac_init(1);
	nrk_create_taskset();
	nrk_start();
	return 0;
}
void RxPacketProcess(uint8_t *pack,uint8_t len){
	uint8_t i, j, len2;
	len2 = pack[1]-1;
	uint8_t temp[szMax];
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
			RouteReplyRx(temp,len2);
			break;
		case DATA:
			DataRx(temp,len2);	
			break;
	  default:
			break;
	}
	return;
}

void InterTxQueueAdd(uint8_t *px,uint8_t len,uint8_t dest){
if(ipPtr>9){
ipPtr=0;
}
for(int i=0;i<len;i++){
ipDat[ipPtr][i]=px[i];
}
ipDes[ipPtr]=dest;
ipLen[ipPtr]=len;
ipPtr++;
ipQue++;
return;
}
void TxQueueAdd(uint8_t *px,uint8_t len,uint8_t dest,uint8_t iFlag){
if(iFlag==1){
	InterTxQueueAdd(px,len,dest);
	return;
}
if(pPtr>9){pPtr=0;}
for(int i=0;i<len;i++){
pDat[pPtr][i]=px[i];
}
pDes[pPtr]=dest;
pLen[pPtr]=len;
pPtr++;
pQue++;
return;
}
void DataInitiate(){
	uint8_t i,j;
	for(i=0;i<MaxHopsPossible;i++){if(cache[i] == Gateway){break;}}
	uint8_t temp[szMax];
	temp[0]=TTL+1;
	for(j=1;j<i+2;j++){temp[j]=cache[j-1];}
	temp[j++]=0xFF;
	temp[j]=0xFE;
	DataTx(temp,i+4,0);
}

void DataTx(uint8_t *data,uint8_t len,uint8_t flag){
	if(data[0]<2){return;}
	uint8_t temp[szMax],i,j;
	temp[0]=0x7E;
	temp[1]=len+1;
	temp[2]=data[0]-1;
	temp[3]=DATA;
	j=1;
	for(i=4;i<len+3;i++){temp[i]=data[j];j++;}
	temp[i]=CRC;
	i=4;
	while(temp[i] != MyOwnAddress){i++;}
	TxQueueAdd(temp,len+4,temp[i+1],flag);
	return;
}

void DataRx(uint8_t *data, uint8_t len){
	uint8_t i,j ;
	i=1;j=1;
	while(data[i] != MyOwnAddress){i++;}
	while(data[j] != Gateway){j++;}	
	if(i>1 && i<j){DataTx(data,len,1);}
	return;
}
void RouteRequestTx(uint8_t *rreq,uint8_t len){
	if(rreq[0]<2){return;}
	uint8_t temp[szMax],i,j;
	temp[0]=0x7E;
	temp[1]=len+2;
	temp[2]=rreq[0]-1;
	temp[3]=RREQ;
	j=1;
	for(i=4;i<len+3;i++){temp[i]=rreq[j];j++;}
	temp[i]=MyOwnAddress;
	i++;
	temp[i]=CRC;
	if(temp[1]==4)// own packet
		TxQueueAdd(temp,len+5,0xFF,0);
	else
		TxQueueAdd(temp,len+5,0xFF,1);
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
		if(*uID == uniqueIDsRREQ[i]){return;}
	}
	//Add it to unique ID
	for(i=0;i<MaxuIDTrack;i++)
	{
		if(uniqueIDsRREQ[i]==0){uniqueIDsRREQ[i]=*uID;break;}
	}
	//Check if the packect is destined for ownself
	if(*rrDest == MyOwnAddress)
	{
		uint8_t temp[szMax];
		for(j=0;j<len;j++){temp[j]=rreq[j];}
		temp[j]=MyOwnAddress;
		RouteReplyTx(temp,len+1,0);
		return;
	}
	//Check for the cache to the destination existence 
	
	for(i=0;i<MaxHopsPossible;i++)
	{
		if(*rrDest == cache[i])
		{
			uint8_t temp[szMax];
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
	uint8_t i,j,k,m,z,zz ;
	i=1;
	while(rrep[i] != MyOwnAddress){i++;}
	if(i>1){RouteReplyTx(rrep,len,0);}
	
	if(cache[0]==0)
	{	
		for(j=0;j<len-1;j++){cache[j]=rrep[i];i++;}
		check:
		for(k=0;k<j;k++){
			for(m=k+1;m<j;m++){
				if(cache[k]==cache[m])
					{
						zz=k+1;
						for(z=m+1;z<j;z++){cache[zz++]=cache[z];}
						j = zz;
						goto check;
					}
			}
		}
	}
	return;
}

void RouteReplyTx(uint8_t *rrep,uint8_t len, uint8_t timeOff){
	if(rrep[0]<2){return;}
	uint8_t temp[szMax],i,j;
	temp[0]=0x7E;
	temp[1]=len;
	temp[2]=rrep[0]-1;
	temp[3]=RREP;
	j=2;
	for(i=4;i<len+2;i++){temp[i]=rrep[j];j++;}
	temp[i]=CRC;
	i=4;
	while(temp[i] != MyOwnAddress){i++;}
	TxQueueAdd(temp,len+3,temp[i-1],0);
	return;
}

void RouteDiscovery(void){
	uint8_t temp[2];
	temp[0]=TTL+1;
	temp[1]=Gateway;
	RouteRequestTx(temp,2);
	return;
}

void rx_task (){
  uint8_t i, len, rssi, *local_rx_buf;
  bmac_set_cca_thresh(DEFAULT_BMAC_CCA); 
	bmac_rx_pkt_set_buffer ((char*)rx_buf, RF_MAX_PAYLOAD_SIZE);
	while (1) {
    bmac_wait_until_rx_pkt();
    nrk_led_set(ORANGE_LED);
    (char*)local_rx_buf = bmac_rx_pkt_get (&len, &rssi);
		if(rssi>RSSIThresh){
			for(i=0;i<len;i++){putchar(local_rx_buf[i]);}
			RxPacketProcess(local_rx_buf,len);
		}
		nrk_led_clr (ORANGE_LED);
		bmac_rx_pkt_release ();
		nrk_wait_until_next_period ();
  }
}
void inter_tx_task (){
	uint8_t i;
  nrk_sig_t tx_done_signal;
  while (!bmac_started ())
  nrk_wait_until_next_period ();
  tx_done_signal = bmac_get_tx_done_signal ();
  nrk_signal_register (tx_done_signal);
	bmac_addr_decode_enable();
  bmac_addr_decode_set_my_mac(MyOwnAddress);
  while (1) {
		if(ipQue>0){
			if(itxPtr>queMax-1){itxPtr=0;}
			bmac_addr_decode_enable();
			bmac_addr_decode_set_my_mac(MyOwnAddress);
			nrk_led_set (BLUE_LED);
			//bmac_auto_ack_enable();
			for(i=0;i<ipLen[itxPtr];i++){tx_buf[i]=ipDat[itxPtr][i];}
			if(ipDes[itxPtr]==0xFF){bmac_addr_decode_dest_mac(0xFFFF);}
			else{bmac_addr_decode_dest_mac(ipDes[itxPtr]);}
			bmac_tx_pkt((char*)tx_buf,ipLen[itxPtr]);
			ipQue--;
			itxPtr++;
			nrk_led_clr (BLUE_LED);
		}  
		nrk_wait_until_next_period ();
  }
}

void tx_task (){
  uint8_t i;
  nrk_sig_t tx_done_signal;
  while (!bmac_started ())
  nrk_wait_until_next_period ();
  tx_done_signal = bmac_get_tx_done_signal ();
  nrk_signal_register (tx_done_signal);
	bmac_addr_decode_enable();
  bmac_addr_decode_set_my_mac(MyOwnAddress);
	while (1) {
		for(i=0;i<MaxuIDTrack;i++)
		{
			uniqueIDsRREQ[i]=0;
		}
		if(cache[0]==0){RouteDiscovery();}
		else{DataInitiate();}
		if(pQue>0){
			if(txPtr>queMax-1){txPtr=0;}
			bmac_addr_decode_enable();
			bmac_addr_decode_set_my_mac(MyOwnAddress);
			nrk_led_set (GREEN_LED);
			//bmac_auto_ack_enable();
			for(i=0;i<pLen[txPtr];i++){tx_buf[i]=pDat[txPtr][i];}
			if(pDes[txPtr]==0xFF){bmac_addr_decode_dest_mac(0xFFFF);}
			else{bmac_addr_decode_dest_mac(pDes[txPtr]);}
			bmac_tx_pkt((char*)tx_buf,pLen[txPtr]);
			for(i=0;i<pLen[txPtr];i++){putchar(tx_buf[i]);}
			putchar(pDes[txPtr]);
			pQue--;
			txPtr++;
			nrk_led_clr (GREEN_LED);
		}  
		nrk_wait_until_next_period ();
  }
}

/*
void RsalRx(uint8_t *rsal,uint8_t len){
	uint8_t i;
	//Check if the unique ID packet has already been seen recently.
	for(i=0;i<MaxuIDTrack;i++)
	{
		if(data[1] == uniqueIDsRSAL[i]){return();}
	}
	//Add it to unique ID
	for(i=0;i<MaxuIDTrack;i++)
	{
		if(uniqueIDsRSAL[i]==0){uniqueIDsRSAL[i]=data[1];break;}
	}

	for(i=1;i<MaxHopsPossible;i++)
	{
		if(cache[i]==data[1])
		{
			if(i==MaxHopsPossible-1) 
			{
				if(cache[i-1]==data[2])
				{
					for(j=0;j<MaxHopsPossible;j++){cache[j]=0;}
					break;
				}
			}
			else
			{
				if(cache[i-1]==data[2] || cache[i+1]==data[2])
				{
					for(j=0;j<MaxHopsPossible;j++){cache[j]=0;}
					break;
				}
			}
		}
	}
	RsalTx(rsal,len);
	return();
}

void RsalTx(uint8_t *rsal, uint8_t len, uint8_t flag){
	if(rsal[0]<2){return;}
	uint8_t temp[len+4],i,j;
	temp[0]=0x7E;
	temp[1]=len;
	temp[2]=rsal[0]-1;
	temp[3]=RSAL;
	j=1;
	for(i=4;i<len+3;i++){temp[i]=rsal[j];j++;}
	temp[j]=CRC;
	TxQueueAdd(temp,len+4,0xFF, flag);
	return;
}

void Rsal_Initiate(dest){
	uint8_t i,j;
	for(i=0;i<MaxHopsPossible;i++){if(cache[i] == Gateway){break;}}
	uint8_t temp[szMax];
	temp[0]=TTL+1;
	for(j=1;j<i+2;j++){temp[j]=cache[j-1];}
	temp[j++]=0xFF;
	temp[j]=0xFE;
}
*/

void nrk_create_taskset (){
  RX_TASK.task = rx_task;
  nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
  RX_TASK.prio = 3;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 0;
  RX_TASK.period.nano_secs = 760 * NANOS_PER_MS;
  RX_TASK.cpu_reserve.secs = 1;
  RX_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
  RX_TASK.offset.secs = 0;
  RX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RX_TASK);

  TX_TASK.task = tx_task;
  nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
  TX_TASK.prio = 1;
  TX_TASK.FirstActivation = TRUE;
  TX_TASK.Type = BASIC_TASK;
  TX_TASK.SchType = PREEMPTIVE;
  TX_TASK.period.secs = 5;
  TX_TASK.period.nano_secs = 1 * NANOS_PER_MS;
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
  INTER_TX_TASK.period.nano_secs = 450 * NANOS_PER_MS;
  INTER_TX_TASK.cpu_reserve.secs = 1;
  INTER_TX_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
  INTER_TX_TASK.offset.secs = 0;
  INTER_TX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&INTER_TX_TASK);
}
