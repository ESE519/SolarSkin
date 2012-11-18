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


uint8_t uniqueIDs[MaxuIDTrack] = {0};
uint8_t cache[MaxHopsPossible] = {0}; //store first as own address

void RouteRequestRx(uint8_t *rreq, uint8_t len){
	uint8_t i,j,k;
	uint8_t *uID, *rrStart, *rrEnd, *rrDest;

	uID     = rreq + 2 ;
	rrStart = rreq + 3 ;
	rrEnd   = rreq + len ;
	rrDest  = rreq + 1 ;

	//Check if the unique ID packet has already been seen recently.
	for(i=0;i<MaxuIDTrack;i++)
	{
		if(*uID == uniqueIds[i]){return();}
	}
	//Check if the packect is destined for ownself
	if(*rrDest == MyOwnAddress)
	{
		uint8_t temp[len+1];
		for(j=0;j<len;j++){temp[j]=rreq[j];}
		temp[j]=MyOwnAddress;
		RouteReplyFormatTx(temp,len+1,0);
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
			RouteReplyFormatTx(temp,len+i+1,i);
			return();
		}
	}
	//broadcast the packet forward 
	RouteRequestFormatTx(rreq,len);
	return;
}

void RouteRequestFormatTx(uint8_t *rreq,uint8_t len){
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

void RouteReplyFormatTx(uint8_t *rrep,uint8_t len, uint8_t timeOff){
	if(rrep[0]<2){return;}
	uint8_t temp[len+3],i,j;
	temp[0]=0x7E;
	temp[1]=len;
	temp[2]=rrep[0]-1;
	temp[3]=RREP;
	j=2;
	for(i=4;i<len+2;i++){temp[i]=rreq[j];j++;}
	temp[j]=CRC;
	i=4;
	while(temp[i] != MyOwnAddress){i++;}
	TxQueueAdd(temp,len+3,temp[i-1]);
	return;
}

void RouteReplyRx(uint8_t *rrep, uint8_t len){
	uint8_t i,j,k ;
	i=1;
	while(rrep[i] != MyOwnAddress){i++;}
	if(i>1){RouteReplyFormatTx(rrep,len,0);}
	for(j=0;j<len-i;j++){cache[j]=rrep[i];i++;}
	return;
}

void RxPacketProcess(){}
void TxQueueAdd(){}
void RouteDiscovery(){}
/*
DATATx
DATARx
ErrTx
ErrRx
DACKRx
RSALTx
RSALRx
RCUPRx

gateway RCUPTx DACKTx 
*/