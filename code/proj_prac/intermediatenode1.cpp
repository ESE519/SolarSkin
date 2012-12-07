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
//#define MAC_ADDR	0x0001
const int MY_ID=2;
const int DEST_ID=1;
char *local_rx_buf;
uint8_t length1,trans_flag=0,priority=1;
char store_buf[2][10];
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
char rx_buf[RF_MAX_PAYLOAD_SIZE];

int main(void)

  {
	        //Initialize buffer to all zeros
			for (int i;i<2;i++){
			for(int j;j<10;j++){
			sprintf(&store_buf[i][j],"0");
			}
			}
			nrk_setup_ports();
			nrk_init();
			bmac_task_config();
			nrk_create_taskset();
		  nrk_start();
			return 0;

	}

void rx_task ()
{
  uint8_t i, j,len, rssi;
  int8_t val;
	//char *local_rx_buf;
  nrk_time_t check_period;
  printf ("rx_task PID=%d\r\n", nrk_get_pid ());

  // init bmac on channel 24 
  bmac_init (24);
	
  // Enable AES 128 bit encryption
  // When encryption is active, messages from plaintext
  // source will still be received. 
	
	// Commented out by MB
  //bmac_encryption_set_key(aes_key,16);
  //bmac_encryption_enable();
	// bmac_encryption_disable();

	
  // By default the RX check rate is 200ms
  // below shows how to change that
  //check_period.secs=0;
  //check_period.nano_secs=200*NANOS_PER_MS;
  //val=bmac_set_rx_check_rate(check_period);

  // The default Clear Channel Assement RSSI threshold.
  // Setting this value higher means that you will only trigger
  // receive with a very strong signal.  Setting this lower means
  // bmac will try to receive fainter packets.  If the value is set
  // too high or too low performance will suffer greatly.
   bmac_set_cca_thresh(DEFAULT_BMAC_CCA); 


  // This sets the next RX buffer.
  // This can be called at anytime before releaseing the packet
  // if you wish to do a zero-copy buffer switch
  bmac_rx_pkt_set_buffer (rx_buf, RF_MAX_PAYLOAD_SIZE);

  while (1) {
    // Wait until an RX packet is received
    val = bmac_wait_until_rx_pkt ();
	 // Get the RX packet 
    nrk_led_set (ORANGE_LED);
    local_rx_buf = bmac_rx_pkt_get (&len, &rssi);
	if(val==1){
	//check  which buffer position is empty
	trans_flag=1;
	for (i=0;i<2;i++){
	if(store_buf[i][0]-48==0){
	sprintf(&store_buf[i][0],"%d",priority);
	for (j=1;j<len;j++){
	store_buf[i][j]=local_rx_buf[j-1];
	}
	priority=priority+1;
	}
	}
	}
	else{
	trans_flag=0;
	}
	
		
   
	
	
	length1=len;
    nrk_led_clr (ORANGE_LED);
   
		// Release the RX buffer so future packets can arrive 
    bmac_rx_pkt_release ();
		
		// this is necessary
    nrk_wait_until_next_period ();

  }

}




void tx_task ()
{
  uint8_t j, i, val, len, cnt;
  int8_t v;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  nrk_time_t r_period;
	
	// printf("tx_task PID=%d\r\n", nrk_get_pid ());

  // Wait until the tx_task starts up bmac
  // This should be called by all tasks using bmac that
  // do not call bmac_init()...
  while (!bmac_started ())
    nrk_wait_until_next_period ();


  // Sample of using Reservations on TX packets
  // This example allows 2 packets to be sent every 5 seconds
  // r_period.secs=5;
  // r_period.nano_secs=0;
  // v=bmac_tx_reserve_set( &r_period, 2 );
  // if(v==NRK_ERROR) nrk_kprintf( PSTR("Error setting b-mac tx reservation (is NRK_MAX_RESERVES defined?)\r\n" ));


  // Get and register the tx_done_signal if you want to
  // do non-blocking transmits
  tx_done_signal = bmac_get_tx_done_signal ();
  nrk_signal_register (tx_done_signal);

  
  cnt = 0;
	
  while (1) {
    // Build a TX packet
    //printf("Id:%d \n",local_rx_buf[0]-48);
	/*if(local_rx_buf[2]-48!=MY_ID && cnt==0){
	for(i=0;i<length1;i++){
	tx_buf[i]=local_rx_buf[i];
	}
	cnt=1;
	}
	else{
	sprintf (tx_buf, "%d %d", MY_ID,DEST_ID);
		cnt=0;
	}*/
	
	sprintf(tx_buf,"%d %d node2 count:%d",cnt);
	
	
	//sprintf(tx_buf,"%d %d",MY_ID,DEST_ID);
    nrk_led_set (BLUE_LED);
		
    // Auto ACK is an energy efficient link layer ACK on packets
    // If Auto ACK is enabled, then bmac_tx_pkt() will return failure
    // if no ACK was received. In a broadcast domain, the ACK's will
    // typically collide.  To avoid this, one can use address decoding. 
    // The functions are as follows:
    // bmac_auto_ack_enable();
			 bmac_auto_ack_disable();

    // Address decoding is a way of preventing the radio from receiving
    // packets that are not address to a particular node.  This will 
    // supress ACK packets from nodes that should not automatically ACK.
    // The functions are as follows:
    // bmac_addr_decode_set_my_mac(uint16_t MAC_ADDR); 
    // bmac_addr_decode_dest_mac(uint16_t DST_ADDR);  // 0xFFFF is broadcast
    // bmac_addr_decode_enable();
    // bmac_addr_decode_disable();
/*
     ctr_cnt[0]=cnt; 
     if(ctr_cnt[0]==255) ctr_cnt[1]++; 
     if(ctr_cnt[1]==255) ctr_cnt[2]++; 
     if(ctr_cnt[2]==255) ctr_cnt[3]++; 
     // You need to increase the ctr on each packet to make the 
     // stream cipher not repeat.
     bmac_encryption_set_ctr_counter(&ctr_cnt,4);

*/  // For blocking transmits, use the following function call.
    // For this there is no need to register  
     val=bmac_tx_pkt(tx_buf, strlen(tx_buf));
		 if(val==NRK_OK) cnt++;
 		 else printf("NO ack or Reserve Violated! \r\n");

    // This function shows how to transmit packets in a
    // non-blocking manner  
    // val = bmac_tx_pkt_nonblocking(tx_buf, strlen (tx_buf));
    // printf ("Tx packet enqueued\r\n");
    // This functions waits on the tx_done_signal
    //ret = nrk_event_wait (SIG(tx_done_signal));

    // Just check to be sure signal is okay
    //if(ret & SIG(tx_done_signal) == 0 ) 
    //printf ("TX done signal error\r\n");
   
    // If you want to see your remaining reservation
    // printf( "reserve=%d ",bmac_tx_reserve_get() );
    
    // Task gets control again after TX complete
    printf("Tx task sent data!\r\n");
    nrk_led_clr (BLUE_LED);
		printf("tx_task PID=%d\r\n", nrk_get_pid ());
    nrk_wait_until_next_period ();
  }

}
void inter_tx_task ()
{
  uint8_t j, i, cnt,val;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  nrk_time_t r_period;
	
	// printf("tx_task PID=%d\r\n", nrk_get_pid ());

  // Wait until the tx_task starts up bmac
  // This should be called by all tasks using bmac that
  // do not call bmac_init()...
  while (!bmac_started ())
    nrk_wait_until_next_period ();


  // Sample of using Reservations on TX packets
  // This example allows 2 packets to be sent every 5 seconds
  // r_period.secs=5;
  // r_period.nano_secs=0;
  // v=bmac_tx_reserve_set( &r_period, 2 );
  // if(v==NRK_ERROR) nrk_kprintf( PSTR("Error setting b-mac tx reservation (is NRK_MAX_RESERVES defined?)\r\n" ));


  // Get and register the tx_done_signal if you want to
  // do non-blocking transmits
  tx_done_signal = bmac_get_tx_done_signal ();
  nrk_signal_register (tx_done_signal);

  
  cnt = 0;
	
  while (1) {
    // Build a TX packet
    //printf("Id:%d \n",local_rx_buf[0]-48);
	/*if(local_rx_buf[2]-48!=MY_ID && cnt==0){
	for(i=0;i<length1;i++){
	tx_buf[i]=local_rx_buf[i];
	}
	cnt=1;
	}
	else{
	sprintf (tx_buf, "%d %d", MY_ID,DEST_ID);
		cnt=0;
	}*/
	if(trans_flag==1){
	for(i=0;i<2;i++){
	if(store_buf[i][0]-48!=0){
	sprintf(&tx_buf[0],"%d",MY_ID);
	j=1;
	while(store_buf[i][j]-48==0){
	tx_buf[j]=store_buf[i][j];
    j++;
	}
	sprintf(&store_buf[i][0],"0");
	}
	}
	}
	
	//sprintf(tx_buf,"%d %d",MY_ID,DEST_ID);
    nrk_led_set (BLUE_LED);
		
    // Auto ACK is an energy efficient link layer ACK on packets
    // If Auto ACK is enabled, then bmac_tx_pkt() will return failure
    // if no ACK was received. In a broadcast domain, the ACK's will
    // typically collide.  To avoid this, one can use address decoding. 
    // The functions are as follows:
    // bmac_auto_ack_enable();
			 bmac_auto_ack_disable();

    // Address decoding is a way of preventing the radio from receiving
    // packets that are not address to a particular node.  This will 
    // supress ACK packets from nodes that should not automatically ACK.
    // The functions are as follows:
    // bmac_addr_decode_set_my_mac(uint16_t MAC_ADDR); 
    // bmac_addr_decode_dest_mac(uint16_t DST_ADDR);  // 0xFFFF is broadcast
    // bmac_addr_decode_enable();
    // bmac_addr_decode_disable();
/*
     ctr_cnt[0]=cnt; 
     if(ctr_cnt[0]==255) ctr_cnt[1]++; 
     if(ctr_cnt[1]==255) ctr_cnt[2]++; 
     if(ctr_cnt[2]==255) ctr_cnt[3]++; 
     // You need to increase the ctr on each packet to make the 
     // stream cipher not repeat.
     bmac_encryption_set_ctr_counter(&ctr_cnt,4);

*/  // For blocking transmits, use the following function call.
    // For this there is no need to register  
     val=bmac_tx_pkt(tx_buf, strlen(tx_buf));
		 if(val==NRK_OK) cnt++;
 		 else printf("NO ack or Reserve Violated! \r\n");

    // This function shows how to transmit packets in a
    // non-blocking manner  
    // val = bmac_tx_pkt_nonblocking(tx_buf, strlen (tx_buf));
    // printf ("Tx packet enqueued\r\n");
    // This functions waits on the tx_done_signal
    //ret = nrk_event_wait (SIG(tx_done_signal));

    // Just check to be sure signal is okay
    //if(ret & SIG(tx_done_signal) == 0 ) 
    //printf ("TX done signal error\r\n");
   
    // If you want to see your remaining reservation
    // printf( "reserve=%d ",bmac_tx_reserve_get() );
    
    // Task gets control again after TX complete
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
  INTER_TX_TASK.prio = 1;
  INTER_TX_TASK.FirstActivation = TRUE;
  INTER_TX_TASK.Type = BASIC_TASK;
  INTER_TX_TASK.SchType = PREEMPTIVE;
  INTER_TX_TASK.period.secs = 200 * NANOS_PER_MS;
  INTER_TX_TASK.period.nano_secs = 0;
  INTER_TX_TASK.cpu_reserve.secs = 1;
  INTER_TX_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
  INTER_TX_TASK.offset.secs = 0;
  INTER_TX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&INTER_TX_TASK);

  printf ("Create done\r\n");
}