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


uint8_t uniqueIDsRREQ[MaxuIDTrack] = {0};
uint8_t uniqueIDsRSAL[MaxuIDTrack] = {0};
uint8_t cache[MaxHopsPossible] = {0}; //store first as own address

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
		case RREP:
			RouteReplyRx(temp,len2);
			break;
		case RERR:
			break;
		case DATA:
			DataRx(temp,len2);
			break;
		case DATR:
			break;
		case RCUP:
			RcupRx(temp,len2);
			break;
		case RSAL:
			RsalRx(temp,len2);
			break;
		case DACK:
			DackRx(temp,len2);
			break;
		case default:
			break;
	}
	return();
}

void RouteDiscovery(void){
	uint8_t temp[2];
	temp[0]=TTL+1;
	temp[1]=Gateway;
	RouteRequestTx(temp,2);
	return();
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
	uint8_t temp[len+3],i,j;
	temp[0]=0x7E;
	temp[1]=len;
	temp[2]=rrep[0]-1;
	temp[3]=RREP;
	j=2;
	for(i=4;i<len+2;i++){temp[i]=rrep[j];j++;}
	temp[j]=CRC;
	i=4;
	while(temp[i] != MyOwnAddress){i++;}
	TxQueueAdd(temp,len+3,temp[i-1]);
	return;
}

void DataTx(uint8_t *data,uint8_t len){
	if(data[0]<2){return;}
	uint8_t temp[len+4],i,j;
	temp[0]=0x7E;
	temp[1]=len;
	temp[2]=data[0]-1;
	temp[3]=DATA;
	j=1;
	for(i=4;i<len+3;i++){temp[i]=data[j];j++;}
	temp[j]=CRC;
	i=4;
	while(temp[i] != MyOwnAddress){i++;}
	TxQueueAdd(temp,len+4,temp[i+1]);
	return;
}

void DataRx(uint8_t *data, uint8_t len){
	uint8_t i,j,k ;
	i=1;j=1;
	while(data[i] != MyOwnAddress){i++;}
	while(data[j] != Gateway){j++;}	
	if(i>1 && i<j){DataTx(data,len);}
	return();
}

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

void RsalTx(uint8_t *rsal, uint8_t len){
	if(rsal[0]<2){return;}
	uint8_t temp[len+4],i,j;
	temp[0]=0x7E;
	temp[1]=len;
	temp[2]=rsal[0]-1;
	temp[3]=RSAL;
	j=1;
	for(i=4;i<len+3;i++){temp[i]=rsal[j];j++;}
	temp[j]=CRC;
	TxQueueAdd(temp,len+4,0xFF);
	return;
}

void TxQueueAdd(){}

/*
Data Initiate
RSAL Initiate

ErrTx
ErrRx
DACKRx
RCUPRx

gateway RCUPTx DACKTx 
*/