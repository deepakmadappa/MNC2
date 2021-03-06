#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <getopt.h>
#include <ctype.h>
#include <queue>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
 **********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
	char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
	int seqnum;
	int acknum;
	int checksum;
	char payload[20];
};

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Statistics 
 * Do NOT change the name/declaration of these variables
 * You need to set the value of these variables appropriately within your code.
 * */
int A_application = 0;
int A_transport = 0;
int B_application = 0;
int B_transport = 0;

/* Globals 
 * Do NOT change the name/declaration of these variables
 * They are set to zero here. You will need to set them (except WINSIZE) to some proper values.
 * */
float TIMEOUT = 5;
int WINSIZE;         //This is supplied as cmd-line parameter; You will need to read this value but do NOT modify it; 
int SND_BUFSIZE = 0; //Sender's Buffer size
int RCV_BUFSIZE = 0; //Receiver's Buffer size

/*My code from here */

float time_local = 0;	//moved the variable

using namespace std;
void tolayer5(int AorB,char *datasent);
void tolayer3(int AorB,struct pkt packet);
void starttimer(int AorB,float increment);
void stoptimer(int);

int GetCheckSum(pkt *packet, bool isAck) {
	int checksum = 0;
	checksum = packet->acknum;
	checksum += packet->seqnum;
	for(int i=0; i < 20; i++) {
		checksum += packet->payload[i];
	}
	return checksum;
}

#define PACKET_SIZE 20;
int SEQ_END;
using namespace std;
const int A = 0;
const int B = 1;
float ALPHA = 0.125;
float BETA = 0.25;
int DEV_MULTI = 4;

class Packet {
public:
	pkt *mContent;
	float mStartTime;
	bool mIsAcked;
	float mTimeout;
	bool mIsResentPacket;

	Packet(pkt* pk) {
		mContent = pk;
		mStartTime = time_local;
		mIsAcked = false;
		mTimeout = 0;
		mIsResentPacket = false;
	}
};

class SenderGlobals {
public:
	static int nextSequenceNumber;
	static queue<msg> senderBuffer;
	static int currentWindowSize;
	static int windowBase;
	static Packet** windowPackets;	//array of packets of current window
	static bool isTimerRunning;
	static int seqNoOnTimer;
	static float EstRTT;
	static float devRTT;

}A_globals;

int SenderGlobals::nextSequenceNumber;
queue<msg> SenderGlobals::senderBuffer;
int SenderGlobals::currentWindowSize;
Packet** SenderGlobals::windowPackets;
int SenderGlobals::windowBase;
bool SenderGlobals::isTimerRunning;
int SenderGlobals::seqNoOnTimer;
float SenderGlobals::EstRTT = TIMEOUT;
float SenderGlobals::devRTT = 0;

float GAMMA = 0.7;
float gIncrement = 0;
float STEP_SIZE = 0.2;

class ReceiverGlobals {
public:
	static pkt** windowPackets;
	static int windowBase;
}B_globals;
int ReceiverGlobals::windowBase;
pkt** ReceiverGlobals::windowPackets;

float absolute(float value) {
	return (value < 0)? value * -1: value;
}

void SetNewTimeOut(float sampleRTT) {
	A_globals.EstRTT = (1 - ALPHA ) * A_globals.EstRTT + ALPHA * sampleRTT;
	A_globals.devRTT = (1 - BETA) * A_globals.devRTT + BETA * absolute(sampleRTT - A_globals.EstRTT);
	TIMEOUT = A_globals.EstRTT + DEV_MULTI * A_globals.devRTT;
	//printf("RTT:%f, TIMEOUT:%f", sampleRTT, TIMEOUT);
}


void SendTillWindowEnd() {
	while(!A_globals.senderBuffer.empty() && A_globals.currentWindowSize < WINSIZE) {
		msg message = A_globals.senderBuffer.front();
		A_globals.senderBuffer.pop();
		pkt *packet = new pkt();
		packet->seqnum = A_globals.nextSequenceNumber;
		cout<<"S:";
		for(int i=0; i < 20; i++) {
			packet->payload[i] = message.data[i];
			cout<<message.data[i];
		}
		cout<<endl;
		packet->checksum = GetCheckSum(packet, false);
		Packet *winPacket = new Packet(packet);	//LEAKY!!!!!!
		A_globals.windowPackets[A_globals.nextSequenceNumber] = winPacket;
		A_globals.windowPackets[A_globals.nextSequenceNumber]->mTimeout = TIMEOUT;
		tolayer3(A, *packet);
		A_globals.currentWindowSize++;
		A_transport++;
		if(!A_globals.isTimerRunning) {
			//this is the first packet that was sent, start the timer
			starttimer(A, TIMEOUT);
			A_globals.seqNoOnTimer = A_globals.nextSequenceNumber;
			A_globals.isTimerRunning = true;
		}
		A_globals.nextSequenceNumber = (A_globals.nextSequenceNumber + 1) % SEQ_END;

	}
}

bool IsInInterval(int value,int start, int end) {
	for(int i = start; i!= end; i=(i+1)%SEQ_END ) {
		if(value == i)
			return true;
	}
	return false;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) //ram's comment - students can change the return type of the function from struct to pointers if necessary
{
	A_application++;
	A_globals.senderBuffer.push(message);
	SendTillWindowEnd();
}

void B_output(struct msg message)  /* need be completed only for extra credit */
// ram's comment - students can change the return type of this function from struct to pointers if needed  
{

}

int findMinimumTimeLeftPacket() {
	float min = 10000000000000000000.0;
	int minPacket = -1;
	for(int i = A_globals.windowBase; i!= A_globals.nextSequenceNumber; i = (i+1)%SEQ_END) {
		float timeleft = A_globals.windowPackets[i]->mTimeout - (time_local - A_globals.windowPackets[i]->mStartTime);
		if(timeleft < min && A_globals.windowPackets[i]->mIsAcked == false) {
			min = timeleft;
			minPacket = i;
		}
	}
	return minPacket;
}

void MoveWindow() {
	int i;
	for(i = A_globals.windowBase; i!= A_globals.nextSequenceNumber; i = (i+1)%SEQ_END) {
		if(A_globals.windowPackets[i]->mIsAcked == false) {
			break;
		}
		A_globals.currentWindowSize--;
		delete A_globals.windowPackets[i]->mContent;
		delete A_globals.windowPackets[i];
	}
	A_globals.windowBase = i;
}

void SetNextTimer() {
	int nextPacketToTime = -1;
	A_globals.isTimerRunning = false;
	if((nextPacketToTime = findMinimumTimeLeftPacket()) != -1) {
		//no packet to time
		A_globals.isTimerRunning = true;
		float timePassed = time_local - A_globals.windowPackets[nextPacketToTime]->mStartTime;
		float newTime = A_globals.windowPackets[nextPacketToTime]->mTimeout - timePassed;
		if(newTime < 0) {
			newTime = 0;
		}
		starttimer(A, newTime);

		A_globals.seqNoOnTimer = nextPacketToTime;

	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	//printf("WINDOWBASE:%d, WINDOWEND:%d, ACK:%d\n",A_globals.windowBase, A_globals.nextSequenceNumber, packet.acknum);
	if(packet.checksum != GetCheckSum(&packet, true)) {
		//corrupted ACK
		return;
	}
	bool isValidAck = IsInInterval(packet.acknum, A_globals.windowBase, A_globals.nextSequenceNumber);	//Ack is within our sent window
	if(!isValidAck) {
		gIncrement = 1;
		TIMEOUT+= gIncrement;
		//printf("STEP_INV_INCREASE:%f, TIMEOUT: %f\n", gIncrement, TIMEOUT);
		//Ack is not within window, discard it
		return;
	}

	if(A_globals.windowPackets[packet.acknum]->mIsAcked) {
		gIncrement = GAMMA * 0.2;
		TIMEOUT+= gIncrement;
		//printf("STEP_INCREASE:%f, TIMEOUT: %f\n", gIncrement, TIMEOUT);

		return;

	} else {
		gIncrement = GAMMA * (-0.2);
		TIMEOUT+= gIncrement;
		//printf("STEP_DECREASE:%f, TIMEOUT: %f\n", gIncrement, TIMEOUT);

	}

	if(!A_globals.windowPackets[packet.acknum]->mIsResentPacket) {
		float rtt = time_local - A_globals.windowPackets[packet.acknum]->mStartTime;
		//	SetNewTimeOut(rtt);
	}

	if(packet.acknum != A_globals.seqNoOnTimer) {
		//timer was not on this packet
		A_globals.windowPackets[packet.acknum]->mIsAcked = true;
		MoveWindow();
		SendTillWindowEnd();
		return;
	}
	A_globals.windowPackets[packet.acknum]->mIsAcked = true;
	stoptimer(A);
	MoveWindow();
	SetNextTimer();
	SendTillWindowEnd();
}

/* called when A's timer goes off */
void A_timerinterrupt() //ram's comment - changed the return type to void.
{
	//stoptimer(A);
	int seqOfTimeOutPacket = A_globals.seqNoOnTimer;
	//gIncrement = (1-GAMMA) * gIncrement + GAMMA * (0.05);
	//TIMEOUT+= gIncrement;
	A_globals.windowPackets[seqOfTimeOutPacket]->mStartTime = time_local;
	A_globals.windowPackets[seqOfTimeOutPacket]->mTimeout = TIMEOUT;
	A_globals.windowPackets[seqOfTimeOutPacket]->mIsResentPacket = true;
	//we need to set timer on new packet find the oldest packet
	//resend the timedout packet
	tolayer3(A, *A_globals.windowPackets[seqOfTimeOutPacket]->mContent);
	cout<<"R:";
	for(int i; i<20; i++) {
		cout<<A_globals.windowPackets[seqOfTimeOutPacket]->mContent->payload[i];
	}
	cout<<endl;
	A_transport++;
	SetNextTimer();
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() //ram's comment - changed the return type to void.
{
	SEQ_END = WINSIZE * 3;
	SenderGlobals::nextSequenceNumber = 0;
	SenderGlobals::currentWindowSize = 0;
	SenderGlobals::windowPackets = new Packet*[SEQ_END];
	SenderGlobals::windowBase = 0;
	SenderGlobals::isTimerRunning = false;
}

void DeliverPackets() {
	int i;
	for(i = B_globals.windowBase; B_globals.windowPackets[i]!=NULL; i=(i+1)%SEQ_END) {
		char message[20];
		cout<<"D:";
		for(int j=0; j<20; j++) {
			message[j] = B_globals.windowPackets[i]->payload[j];
			printf("%c", message[j]);
		}
		cout<<endl;
		B_application++;
		tolayer5(B, message);
		delete B_globals.windowPackets[i];
		B_globals.windowPackets[i] = NULL;
	}
	B_globals.windowBase = i;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	B_transport++;
	if(packet.checksum != GetCheckSum(&packet, false)) {
		//corrupt packet, drop it
		return;
	}
	int windowEnd = (B_globals.windowBase + WINSIZE) % SEQ_END;
	int ackNo = packet.seqnum;	//if the very first packet is junk, it'll respond with max sequence number which is not really correct behavior but doesn't harm
	pkt ack;
	ack.acknum =ackNo;
	ack.checksum = GetCheckSum(&ack, true);
	tolayer3(B, ack);
	if(!IsInInterval(packet.seqnum, B_globals.windowBase, windowEnd)) {
		//unexpected packet, ack and ignore
		DeliverPackets();
		return;
	}
	pkt *newPacket = new pkt();
	newPacket->seqnum = packet.seqnum;
	for(int i=0; i< 20; i++) {
		newPacket->payload[i] = packet.payload[i];
	}
	B_globals.windowPackets[packet.seqnum] = newPacket;
	DeliverPackets();
}

/* called when B's timer goes off */
void B_timerinterrupt() //ram's comment - changed the return type to void.
{

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() //ram's comment - changed the return type to void.
{
	ReceiverGlobals::windowPackets = new pkt*[SEQ_END];
	for(int i=0; i < SEQ_END; i++) {
		ReceiverGlobals::windowPackets[i] = NULL;
	}
	ReceiverGlobals::windowBase = 0;
}

int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
	double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
	float x;                   /* individual students may need to change mmm */
	x = rand()/mmm;            /* x should be uniform in [0,1] */
	return(x);
}  


/*****************************************************************
 ***************** NETWORK EMULATION CODE IS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
 ******************************************************************/



/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1


struct event {
	float evtime;           /* event time */
	int evtype;             /* event type code */
	int eventity;           /* entity where event occurs */
	struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
	struct event *prev;
	struct event *next;
};
struct event *evlist = NULL;   /* the event list */


void insertevent(struct event *p)
{
	struct event *q,*qold;

	if (TRACE>2) {
		printf("            INSERTEVENT: time is %lf\n",time_local);
		printf("            INSERTEVENT: future time will be %lf\n",p->evtime);
	}
	q = evlist;     /* q points to header of list in which p struct inserted */
	if (q==NULL) {   /* list is empty */
		evlist=p;
		p->next=NULL;
		p->prev=NULL;
	}
	else {
		for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
			qold=q;
		if (q==NULL) {   /* end of list */
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		}
		else if (q==evlist) { /* front of list */
			p->next=evlist;
			p->prev=NULL;
			p->next->prev=p;
			evlist = p;
		}
		else {     /* middle of list */
			p->next=q;
			p->prev=q->prev;
			q->prev->next=p;
			q->prev=p;
		}
	}
}





/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
	double x,log(),ceil();
	struct event *evptr;
	//    //char *malloc();
	float ttime;
	int tempint;

	if (TRACE>2)
		printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

	x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
	/* having mean of lambda        */

	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime =  time_local + x;
	evptr->evtype =  FROM_LAYER5;
	if (BIDIRECTIONAL && (jimsrand()>0.5) )
		evptr->eventity = B;
	else
		evptr->eventity = A;
	insertevent(evptr);
}





void init(int seed)                         /* initialize the simulator */
{
	int i;
	float sum, avg;
	float jimsrand();


	printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
	printf("Enter the number of messages to simulate: ");
	scanf("%d",&nsimmax);
	printf("Enter  packet loss probability [enter 0.0 for no loss]:");
	scanf("%f",&lossprob);
	printf("Enter packet corruption probability [0.0 for no corruption]:");
	scanf("%f",&corruptprob);
	printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
	scanf("%f",&lambda);
	printf("Enter TRACE:");
	scanf("%d",&TRACE);

	srand(seed);              /* init random number generator */
	sum = 0.0;                /* test random number generator for students */
	for (i=0; i<1000; i++)
		sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
	avg = sum/1000.0;
	if (avg < 0.25 || avg > 0.75) {
		printf("It is likely that random number generation on your machine\n" );
		printf("is different from what this emulator expects.  Please take\n");
		printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
		exit(0);
	}

	ntolayer3 = 0;
	nlost = 0;
	ncorrupt = 0;

	time_local=0;                    /* initialize time to 0.0 */
	generate_next_arrival();     /* initialize event list */
}






//int TRACE = 1;             /* for my debugging */
//int nsim = 0;              /* number of messages from 5 to 4 so far */ 
//int nsimmax = 0;           /* number of msgs to generate, then stop */
//float time = 0.000;
//float lossprob;            /* probability that a packet is dropped  */
//float corruptprob;         /* probability that one bit is packet is flipped */
//float lambda;              /* arrival rate of messages from layer 5 */   
//int   ntolayer3;           /* number sent into layer 3 */
//int   nlost;               /* number lost in media */
//int ncorrupt;              /* number corrupted by media*/

/**
 * Checks if the array pointed to by input holds a valid number.
 *
 * @param  input char* to the array holding the value.
 * @return TRUE or FALSE
 */
int isNumber(char *input)
{
	while (*input){
		if (!isdigit(*input))
			return 0;
		else
			input += 1;
	}

	return 1;
}

int main(int argc, char **argv)
{
	struct event *eventptr;
	struct msg  msg2give;
	struct pkt  pkt2give;

	int i,j;
	char c;

	int opt;
	int seed;

	//Check for number of arguments
	if(argc != 5){
		fprintf(stderr, "Missing arguments\n");
		printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
		return -1;
	}

	/*
	 * Parse the arguments
	 * http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
	 */
	while((opt = getopt(argc, argv,"s:w:")) != -1){
		switch (opt){
		case 's':   if(!isNumber(optarg)){
			fprintf(stderr, "Invalid value for -s\n");
			return -1;
		}
		seed = atoi(optarg);
		break;

		case 'w':   if(!isNumber(optarg)){
			fprintf(stderr, "Invalid value for -w\n");
			return -1;
		}
		WINSIZE = atoi(optarg);
		break;

		case '?':   break;

		default:    printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
		return -1;
		}
	}

	init(seed);
	A_init();
	B_init();

	while (1) {
		eventptr = evlist;            /* get next event to simulate */
		if (eventptr==NULL)
			goto terminate;
		evlist = evlist->next;        /* remove this event from event list */
		if (evlist!=NULL)
			evlist->prev=NULL;
		if (TRACE>=2) {
			printf("\nEVENT time: %f,",eventptr->evtime);
			printf("  type: %d",eventptr->evtype);
			if (eventptr->evtype==0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype==1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n",eventptr->eventity);
		}
		time_local = eventptr->evtime;        /* update time to next event time */
		if (nsim==nsimmax)
			break;                        /* all done with simulation */
		if (eventptr->evtype == FROM_LAYER5 ) {
			generate_next_arrival();   /* set up future arrival */
			/* fill in msg to give with string of same letter */
			j = nsim % 26;
			for (i=0; i<20; i++)
				msg2give.data[i] = 97 + j;
			if (TRACE>2) {
				printf("          MAINLOOP: data given to student: ");
				for (i=0; i<20; i++)
					printf("%c", msg2give.data[i]);
				printf("\n");
			}
			nsim++;
			if (eventptr->eventity == A)
				A_output(msg2give);
			else
				B_output(msg2give);
		}
		else if (eventptr->evtype ==  FROM_LAYER3) {
			pkt2give.seqnum = eventptr->pktptr->seqnum;
			pkt2give.acknum = eventptr->pktptr->acknum;
			pkt2give.checksum = eventptr->pktptr->checksum;
			for (i=0; i<20; i++)
				pkt2give.payload[i] = eventptr->pktptr->payload[i];
			if (eventptr->eventity ==A)      /* deliver packet by calling */
				A_input(pkt2give);            /* appropriate entity */
			else
				B_input(pkt2give);
			free(eventptr->pktptr);          /* free the memory for packet */
		}
		else if (eventptr->evtype ==  TIMER_INTERRUPT) {
			if (eventptr->eventity == A)
				A_timerinterrupt();
			else
				B_timerinterrupt();
		}
		else  {
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}

	terminate:
	//Do NOT change any of the following printfs
	printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time_local,nsim);

	printf("\n");
	printf("Protocol: ABT\n");
	printf("[PA2]%d packets sent from the Application Layer of Sender A[/PA2]\n", A_application);
	printf("[PA2]%d packets sent from the Transport Layer of Sender A[/PA2]\n", A_transport);
	printf("[PA2]%d packets received at the Transport layer of Receiver B[/PA2]\n", B_transport);
	printf("[PA2]%d packets received at the Application layer of Receiver B[/PA2]\n", B_application);
	printf("[PA2]Total time: %f time units[/PA2]\n", time_local);
	printf("[PA2]Throughput: %f packets/time units[/PA2]\n", B_application/time_local);
	return 0;
}


/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

/*void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

   x = lambda*jimsrand()*2;  // x is uniform on [0,2*lambda] 
                             // having mean of lambda       
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} */




void printevlist()
{
	struct event *q;
	int i;
	printf("--------------\nEvent List Follows:\n");
	for(q = evlist; q!=NULL; q=q->next) {
		printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
	}
	printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB)
//AorB;  /* A or B is trying to stop timer */
{
	struct event *q,*qold;

	if (TRACE>2)
		printf("          STOP TIMER: stopping timer at %f\n",time_local);
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q=evlist; q!=NULL ; q = q->next)
		if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
			/* remove this event */
			if (q->next==NULL && q->prev==NULL)
				evlist=NULL;         /* remove first and only event on list */
			else if (q->next==NULL) /* end of list - there is one in front */
				q->prev->next = NULL;
			else if (q==evlist) { /* front of list - there must be event after */
				q->next->prev=NULL;
				evlist = q->next;
			}
			else {     /* middle of list */
				q->next->prev = q->prev;
				q->prev->next =  q->next;
			}
			free(q);
			return;
		}
	printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB,float increment)
// AorB;  /* A or B is trying to stop timer */

{

	struct event *q;
	struct event *evptr;
	////char *malloc();

	if (TRACE>2)
		printf("          START TIMER: starting timer at %f\n",time_local);
	/* be nice: check to see if timer is already started, if so, then  warn */
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q=evlist; q!=NULL ; q = q->next)
		if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
			printf("Warning: attempt to start a timer that is already started\n");
			return;
		}

	/* create future event for when timer goes off */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime =  time_local + increment;
	evptr->evtype =  TIMER_INTERRUPT;
	evptr->eventity = AorB;
	insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
void tolayer3(int AorB,struct pkt packet)
{
	struct pkt *mypktptr;
	struct event *evptr,*q;
	////char *malloc();
	float lastime, x, jimsrand();
	int i;


	ntolayer3++;

	/* simulate losses: */
	if (jimsrand() < lossprob)  {
		nlost++;
		if (TRACE>0)
			printf("          TOLAYER3: packet being lost\n");
		return;
	}

	/* make a copy of the packet student just gave me since he/she may decide */
	/* to do something with the packet after we return back to him/her */
	mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
	mypktptr->seqnum = packet.seqnum;
	mypktptr->acknum = packet.acknum;
	mypktptr->checksum = packet.checksum;
	for (i=0; i<20; i++)
		mypktptr->payload[i] = packet.payload[i];
	if (TRACE>2)  {
		printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
				mypktptr->acknum,  mypktptr->checksum);
		for (i=0; i<20; i++)
			printf("%c",mypktptr->payload[i]);
		printf("\n");
	}

	/* create future event for arrival of packet at the other side */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
	evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
	evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
	/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
	lastime = time_local;
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
	for (q=evlist; q!=NULL ; q = q->next)
		if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) )
			lastime = q->evtime;
	evptr->evtime =  lastime + 1 + 9*jimsrand();



	/* simulate corruption: */
	if (jimsrand() < corruptprob)  {
		ncorrupt++;
		if ( (x = jimsrand()) < .75)
			mypktptr->payload[0]='Z';   /* corrupt payload */
		else if (x < .875)
			mypktptr->seqnum = 999999;
		else
			mypktptr->acknum = 999999;
		if (TRACE>0)
			printf("          TOLAYER3: packet being corrupted\n");
	}

	if (TRACE>2)
		printf("          TOLAYER3: scheduling arrival on other side\n");
	insertevent(evptr);
} 

void tolayer5(int AorB,char *datasent)
{

	int i;
	if (TRACE>2) {
		printf("          TOLAYER5: data received: ");
		for (i=0; i<20; i++)
			printf("%c",datasent[i]);
		printf("\n");
	}

}
