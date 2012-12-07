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

// Change this to your group channel
#define MY_CHANNEL 13 

#define MAX_MOLES  5 

// Gateway has ID = 1, the slaves/moles have ID 2,3 onwards...
#define MY_ID 1

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

void nrk_create_taskset ();

char tx_buf[RF_MAX_PAYLOAD_SIZE];
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

  uint8_t j, i,last_mole,m,len;
  uint8_t cnt,cmd,length;
  uint8_t rssi,slot,oldMole,newMole=0,Rounds =0 ;
  uint8_t number_timeouts =0;
  uint8_t *local_rx_buf;
  uint16_t counter;
  uint32_t Score = 0;
  volatile nrk_time_t t;
  char c;
  nrk_sig_t uart_rx_signal;

  printf( "Task1 PID=%d\r\n",nrk_get_pid());
  counter=0;
  cnt=0;
  
  nrk_led_set(RED_LED);
	
  int8_t val;
  nrk_time_t check_period;
  printf ("rx_task PID=%d\r\n", nrk_get_pid ());

  // init bmac on channel 
  bmac_init (MY_CHANNEL);
	
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

  while (Rounds<=10) {
    // Wait until an RX packet is received
    val = bmac_wait_until_rx_pkt ();
		
		
	 uint8_t mole_index,state;	
    // Get the RX packet 
    local_rx_buf = bmac_rx_pkt_get (&len, &rssi);
    //if( bmac_rx_pkt_is_encrypted()==1 ) nrk_kprintf( PSTR( "Packet Encrypted\r\n" ));
    printf( "RX slot %d %d: ",slot,length );
		   // print the packet out
		   for(i=PKT_DATA_START; i<length; i++ )
			printf( "%c",local_rx_buf[i] );
 		  
                   //buffer position 8 stores the value of the moleid from the slaves
                   //buffer position 16 stores the value of light 
                   // '1' indicates mole whacked (light closed)
                   // '0' indicates mole not whacked yet (light open)
                   if( ((local_rx_buf[8]-48) == newMole) && (local_rx_buf[16]=='1') && Rounds <=9)
                   {
		     printf("NEWMOLE:%d",newMole);
                     oldMole = newMole;
                     while(oldMole==newMole)
		     newMole = rand()%MAX_MOLES;
                     
                     Rounds++;
		     
                     printf("\n rounds %d",Rounds);
                    nrk_time_get(&timeend);
		    nrk_time_get(&timeout);
                    Score = timeend.secs-timestart.secs + number_timeouts * 10;
		    
                    printf("\nScore : %d",Score);

                   }
		   printf( "\r\n" ); 
   
		// Release the RX buffer so future packets can arrive 
    bmac_rx_pkt_release ();
		
	nrk_time_get(&timeend);
    Score = timeend.secs-timestart.secs + number_timeouts*10;
    printf("DONE and Score = %d",Score);
	
		// this is necessary
    nrk_wait_until_next_period ();

  }

}


uint8_t ctr_cnt[4];

void tx_task ()
{
  uint8_t j, i,last_mole,m,len;
  uint8_t cnt,cmd,length;
  uint8_t rssi,slot,oldMole,newMole=0,Rounds =0 ;
  uint8_t number_timeouts =0;
  uint8_t *local_rx_buf;
  uint16_t counter;
  uint32_t Score = 0;
  volatile nrk_time_t t;
  char c;
  nrk_sig_t uart_rx_signal;
  
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

  ctr_cnt[0]=0; ctr_cnt[1]=0; ctr_cnt[2]=0; ctr_cnt[3]=0;
  cnt = 0;
	
  while (Rounds<=10) {
  
   nrk_time_get(&timeend);
		if (Rounds<=9){
		  if(timeend.secs-timeout.secs > (8-Rounds/2))
		  {
		    oldMole= newMole;
		    while(oldMole==newMole)
		      newMole = rand()%MAX_MOLES;
		    nrk_time_get(&timeout);
		    Rounds++;
		    number_timeouts++;
		    printf("\n\n\n\n\n\nRounds = %d \nnumber_timeouts = %d \npresent time = %d\n", Rounds, number_timeouts, timeout.secs-timestart.secs);
		  }
		}
		 // added the next mole to light up into the buffer
		sprintf( &tx_buf[PKT_DATA_START],"Master count is %d and new mole is %d and Round = %d",counter,newMole,Rounds);
		// PKT_DATA_START + length of string + 1 for null at end of string
		if(Rounds>=10){
		  
		  Rounds++;
		sprintf( &tx_buf[PKT_DATA_START],"Master count is %d and new mole is %d and Round = %d",counter,6,Rounds); 
		}

		length=strlen(&tx_buf[PKT_DATA_START])+PKT_DATA_START+1;
		val=bmac_tx_pkt(tx_buf, length);
		 if(val==NRK_OK) cnt++;
 		 else printf("NO ack or Reserve Violated! \r\n");
		 
         for(i=PKT_DATA_START;i<length;i++)
                   printf("%c",local_rx_buf[i]);
		nrk_led_toggle(BLUE_LED);
		printf("\n\n");

		
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

  printf ("Create done\r\n");
}