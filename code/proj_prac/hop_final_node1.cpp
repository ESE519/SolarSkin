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
#define MY_MAC_ADDR	0x0002
uint16_t dst_addr;
char *local_rx_buf,*inter_tx_buf;
uint8_t length1,inter_flag=0,trans_flag=0; 
nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

nrk_task_type INTER_TX_TASK;
NRK_STK inter_tx_task_stack[NRK_APP_STACKSIZE];
void inter_tx_task (void);

void nrk_create_taskset ();

char tx_buf[RF_MAX_PAYLOAD_SIZE];
//char inter_tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];

int main(void)

  {
	    nrk_setup_ports();
			nrk_init();
			bmac_task_config();
			nrk_create_taskset();
		  nrk_start();
			return 0;
}

void rx_task ()
{
  uint8_t i, len, rssi;
  int8_t val;
  
	//char *local_rx_buf;
  nrk_time_t check_period;
  printf ("rx_task PID=%d\r\n", nrk_get_pid ());

  // init bmac on channel 24 
  bmac_init (5);
  bmac_set_cca_thresh(DEFAULT_BMAC_CCA); 
  bmac_rx_pkt_set_buffer (rx_buf, RF_MAX_PAYLOAD_SIZE);

  while (1) {
    // Wait until an RX packet is received
    val = bmac_wait_until_rx_pkt ();
		//printf("Hi..\n");
    // Get the RX packet 
    nrk_led_set (ORANGE_LED);
    local_rx_buf = bmac_rx_pkt_get (&len, &rssi);
	  inter_flag=0;
		if(rssi<5){
	    dst_addr=0x0003;
	//inter_tx_buf=local_rx_buf;
		}
			else
			{
				dst_addr=0x0001;
			}
	if(local_rx_buf[0]=='P'){
   dst_addr=0x0001;
	 inter_tx_buf=local_rx_buf;
   inter_flag=1;		
}
	
	printf("inter_flag: %d",inter_flag);
	printf("\r\n");
	printf("rx_buf:");
	for(i=0;i<len;i++){
	printf("%c",local_rx_buf[i]);
	}
	
    nrk_led_clr (ORANGE_LED);
   
		// Release the RX buffer so future packets can arrive 
    bmac_rx_pkt_release ();
		
		// this is necessary
    nrk_wait_until_next_period ();

  }

}
/*
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
  
  while (1) {
    if(inter_flag==1){
		printf("Hi..\n");
	  bmac_addr_decode_disable();
    bmac_addr_decode_set_my_mac(MY_MAC_ADDR);
	  sprintf(&tx_buf[0],"%d",MY_MAC_ADDR);
	  for(i=0;i<strlen(inter_tx_buf);i++){
	  tx_buf[i+1]=inter_tx_buf[i];
	}
	  nrk_led_set (BLUE_LED);
    bmac_auto_ack_disable();
    bmac_addr_decode_dest_mac(dst_addr);  // 0xFFFF is broadcast
	  val=bmac_tx_pkt(tx_buf, strlen(tx_buf));
	  inter_flag=0;
    printf("Tx task sent data!\r\n");
    nrk_led_clr (BLUE_LED);
	  printf("tx_task PID=%d\r\n", nrk_get_pid ());
}  
  nrk_wait_until_next_period ();
  }

}
*/
void tx_task ()
{
  uint8_t j, i, val, len, cnt,turn=0;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  nrk_time_t r_period;
   while (!bmac_started ())
     nrk_wait_until_next_period ();
  tx_done_signal = bmac_get_tx_done_signal ();
  nrk_signal_register (tx_done_signal);
  cnt = 0;
  
  while (1) {
    if(inter_flag==0 ){
		printf("Hi..\n");
	  bmac_addr_decode_disable();
    bmac_addr_decode_set_my_mac(MY_MAC_ADDR);
	  sprintf(tx_buf,"Path: %d-1 node_id:%d count:%d",MY_MAC_ADDR,MY_MAC_ADDR,cnt);
    nrk_led_set (BLUE_LED);
		
   
    bmac_auto_ack_disable();
    bmac_addr_decode_dest_mac(dst_addr);  // 0xFFFF is broadcast
	  val=bmac_tx_pkt(tx_buf, strlen(tx_buf));
    printf("Tx task sent data!\r\n");
    nrk_led_clr (BLUE_LED);
		printf("tx_task PID=%d\r\n", nrk_get_pid ());
    }
	  
			if(inter_flag==1){
				if(turn==0){
					printf("Hi..\n");
	  bmac_addr_decode_disable();
    bmac_addr_decode_set_my_mac(MY_MAC_ADDR);
	  sprintf(tx_buf,"Path: %d-1 node_id:%d count:%d",MY_MAC_ADDR,MY_MAC_ADDR,cnt);
    nrk_led_set (BLUE_LED);
		bmac_auto_ack_disable();
    bmac_addr_decode_dest_mac(dst_addr);  // 0xFFFF is broadcast
	  val=bmac_tx_pkt(tx_buf, strlen(tx_buf));
    printf("Tx task sent data!\r\n");
    nrk_led_clr (BLUE_LED);
		printf("tx_task PID=%d\r\n", nrk_get_pid ());
					turn=1;
				}
				else{
	  bmac_addr_decode_disable();
    bmac_addr_decode_set_my_mac(MY_MAC_ADDR);
	  sprintf(tx_buf,"Path: %d-%d-1 node_id:%d count:%d",inter_tx_buf[6]-48,MY_MAC_ADDR,inter_tx_buf[18]-48,inter_tx_buf[26]-48);
				/*	sprintf(&tx_buf[strlen(tx_buf)],"%d-",inter_tx_buf[6]);
					for(i=0;i<strlen(inter_tx_buf);i++){
	  tx_buf[i+]=inter_tx_buf[i];
	}*/
	  nrk_led_set (BLUE_LED);
    bmac_auto_ack_disable();
    bmac_addr_decode_dest_mac(dst_addr);  // 0xFFFF is broadcast
	  val=bmac_tx_pkt(tx_buf, strlen(tx_buf));
	  printf("Tx task sent data!\r\n");
    nrk_led_clr (BLUE_LED);
	  printf("tx_task PID=%d\r\n", nrk_get_pid ());
	  turn=0;
}
}

  cnt++;
nrk_wait_until_next_period ();
   }

}

void nrk_create_taskset ()
{


  RX_TASK.task = rx_task;
  nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
  RX_TASK.prio = 3;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 0;
  RX_TASK.period.nano_secs = 500 *NANOS_PER_MS;
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
  printf ("Create done\r\n");
}