#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

/* Multithread setup.
 *
 * When MULTITHREADED_OPERATION = 0:
 *   Use one signle thread for reading the UDP packets from the WiFi SoC and sending the data via TCP.
 *   Here, no buffering is done, but the performance might be worse and data might get lost in the worst-case. Timestamps might
 *   get messed.
 * When MULTITHREADED_OPERATION = 1:
 *   Use a separate thread for reading data from the WiFi SoC and for transmitting them via TCP.
 *   One thread always listens to the UDP socket and writes the received data to a circular buffer immediately after reception.
 *   The other thread tries to send out what is in the buffer via TCP as soon as the buffer is not empty
 *   This mode of operation is recommended.
 *
 */
#define MULTITHREADED_OPERATION 1

#if  MULTITHREADED_OPERATION
#include <pthread.h>
#include <semaphore.h>
#endif

#define DEBUG(...)                                      //if you'd like to see the debug messages, replace this by #define DEBUG(...) printf(__VA_ARGS)
#define NEXMON_PORT 5500                                //the port on which the packets from Nexmon are received
#define TCP_SERVER_PORT 5501                            //the port on which our TCP server listens for incomming connections. Recommended: 5501
#define BUFLEN_SINGLEPKG (4*512 + 34)                   //A buffer large enough to buffer a single packet from nexmon
#define NPKGS_TO_BUFFER 1000                            //The maximum number of packets to buffer. The reception of UDP packets from Nexmon might temporarily be faster then what is polled from the TCP socket. Hence, buffering is needed.
#define BUFLEN (BUFLEN_SINGLEPKG*NPKGS_TO_BUFFER)       //The number of bytes for the buffer. Keep this as it is and change NPKGS_TO_BUFFER if needed



/* A timespec with 64 bytes length. On a Raspi, timespec will be typicalls 8 bytes and hence incompatible to CSI GUI.
 * We hence here define a larger one, and send it over the network.
 */

struct timespec_16bytes{
  uint64_t tv_sec;
  uint64_t tv_nsec;
};

//are we yet connected?
static uint32_t connected = 0;

int s_TCP_remote;                   //socket to communicate with a station that has requested a conenction

/* If the remote side closes the connection, we get a SIGPIPE on our socket.
 * The CSI sever should not close, but start listening for a new incoming connection.*/
void sigpipe_handler(){
    printf("sigPipe received.\n");
    connected = 0;
}



/* **Ringbuffer stuff**
 * We use a circular buffer to store our data.
 */

#if  MULTITHREADED_OPERATION                    //we only need buffering in the multithread operation mode
typedef struct _ringBuf{ 
 uint32_t posWr;               //position to write
 uint32_t posRd;               //position to read
 uint32_t nBytes;              //number of bytes currently stored
 char* buf;                    //the actual buffer in memory
 uint32_t size;                //the number of bytes the ringbuffer could store
} ringBuf;


ringBuf rb_global;              //our global ringbuffer structure
char buf[BUFLEN];               //the actual buffer


sem_t sem_protect;              //A semaphore to protect the access to the ringbuffer by two concurrent threads
sem_t sem_wakeup;               //a semaphore to wake up the TCP thread if there is new data in the circular ringBuf that could be transmitted


/* Init the ringBuf data structure*/
void ringBuf_init(ringBuf* rb, char* buf, uint32_t size){
    rb->buf = buf;
    rb->size = size;
    rb->posWr = 0;
    rb->posRd = 0;
    rb->nBytes =0 ;    
}

/*  Copy the data contained in the buffer buf into the ringBuf. The size of buf is given by nByte.
 *  The return value is the number of bytes written. This could be less than requested if there is insufficient space.
 */
uint32_t ringBuf_write(ringBuf *rb, char* buf,uint32_t nBytes){
  sem_wait(&sem_protect);
  if(nBytes > rb->size - rb->nBytes){
    nBytes = rb->size - rb->nBytes;
  }

  uint32_t nBytesNoWrap = rb->size - rb->posWr - 1;
  if(nBytes < nBytesNoWrap){
    nBytesNoWrap = nBytes;
  }
  uint32_t nBytesWrap = nBytes - nBytesNoWrap;

  memcpy(rb->buf + rb->posWr, buf, nBytesNoWrap);               //no wrap
  if(nBytesWrap > 0){
  memcpy(rb->buf, buf + nBytesNoWrap, nBytesWrap);               //wrap
  }

  //update pointer
  if(nBytesWrap == 0){
    rb->posWr += nBytesNoWrap;
  }else{
    rb->posWr = nBytesWrap;
  }
  rb->nBytes += nBytes;
  sem_post(&sem_protect);               //free the protection sema
  sem_post(&sem_wakeup);                //wake-up an eventually blocked ringBuf_read() function
  return nBytes;
}


/*
 *  Copy nBytes from the ringBuf into buf.
 *  The return value is the number of bytes read fromt he ringbuf. This could be less than requested if nBytes exceeds what is in the ringBuf.
 */
uint32_t ringBuf_read(ringBuf* rb, char* buf, uint32_t nBytes){
  sem_wait(&sem_protect);
  if(nBytes > rb->nBytes){
    nBytes =  rb->nBytes;
  }
    while(rb->nBytes == 0){                     //if the ringBuf is empty, free the protection semaphore and wait for the wakeup-sema. The wakeup-sema is alread acquired and hence, the call will return until any other thread gives this sema.
      sem_post(&sem_protect);
      sem_wait(&sem_wakeup);                    //This sema will already been taken when we call sem_wait(). Hence, this call will return once another thread calls sem_post(). This is done in ringBuf_write().
      sem_wait(&sem_protect);
    }

  uint32_t nBytesNoWrap = rb->size - rb->posRd - 1;             //Number of bytes without wraping around.
  if(nBytes < nBytesNoWrap){
    nBytesNoWrap = nBytes;
  }
  uint32_t nBytesWrap = nBytes - nBytesNoWrap;
  memcpy(buf,rb->buf + rb->posRd, nBytesNoWrap);               //no wrap
  if(nBytesWrap > 0){
  memcpy(buf + nBytesNoWrap,rb->buf, nBytesWrap);               //wrap
  }

  //update pointer
  if(nBytesWrap == 0){
    rb->posRd += nBytesNoWrap;
  }else{
    rb->posRd = nBytesWrap;
  }

  rb->nBytes -= nBytes;
  sem_post(&sem_protect);
  return nBytes;
}


// A thread to write the data from the ringBuf to the TCP socket
void* write_thread(void* ign){
    char txBuf[BUFLEN_SINGLEPKG];   
    int32_t nBytesRead;

    while(connected){
        nBytesRead = ringBuf_read(&rb_global,txBuf,BUFLEN_SINGLEPKG);
        if((nBytesRead >= 0)&&(connected)){
          write(s_TCP_remote,txBuf,nBytesRead);
        }
    }
        printf("WR thread terminating.\n");
return NULL;
}
#endif

int main(int argc, char* argv){
    char rcvBuf[BUFLEN_SINGLEPKG];
    struct timespec timeNow;
    struct timespec_16bytes timeNow16;

    /*************************** UDP stuff**********************/
    struct sockaddr_in sin_UDP_CSIServer, sin_UDP_Nexmon;
    int s_UDP;
    int32_t nBytesReceived;

    //create an UDP socket
    s_UDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(s_UDP < 0){
        perror("socket()");
        exit(1);
    }
    memset((void*) &sin_UDP_CSIServer,0,sizeof(struct sockaddr_in));

    sin_UDP_CSIServer.sin_family = AF_INET;
    sin_UDP_CSIServer.sin_addr.s_addr = htonl(INADDR_ANY);
    sin_UDP_CSIServer.sin_port = htons(NEXMON_PORT);

    //If this port is already used, reclaim it :)
    int option = 1;  
    setsockopt(s_UDP, SOL_SOCKET,SO_REUSEADDR,&option, sizeof(option));

    if(bind(s_UDP, (struct sockaddr*) &sin_UDP_CSIServer, sizeof(struct sockaddr_in)) < 0){
        perror("bind");
        exit(1);
    }

    /*************************** Establish TCP connection**********************/
    int s_TCP_server_listen;            //socket for listening to incoming connections

    struct sockaddr_in sin_TCP_server, sin_TCP_remote;        //sin_TCP_server: Our address. sin_TCP_remote: address of remote device.
    socklen_t addrlen = sizeof(struct sockaddr_in);

    memset((void*) &sin_TCP_server, 0, sizeof(struct sockaddr_in));
    memset((void*) &sin_TCP_remote, 0, sizeof(struct sockaddr_in));

    sin_TCP_server.sin_family = AF_INET;
    sin_TCP_server.sin_addr.s_addr = htonl(INADDR_ANY);
    sin_TCP_server.sin_port = htons(TCP_SERVER_PORT);

    //create listening socket
    s_TCP_server_listen = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
    option = 1;     
    setsockopt(s_TCP_server_listen, SOL_SOCKET,SO_REUSEADDR,&option, sizeof(option));

    if(s_TCP_server_listen < 0){
        perror("socket()"); 
        exit(1);      
    }

    //bind listening socket to address
    if(bind(s_TCP_server_listen, (struct sockaddr*) &sin_TCP_server, sizeof(struct sockaddr_in)) < 0){
        perror("bind()"); 
        exit(1);  
    }

    //start listening for incoming connections
    if(listen(s_TCP_server_listen,1) < 0){
        perror("listen()"); 
        exit(1);  
    }
    /*************************** Catch SIGPIPE if the remote device hangs up or dies **********************/

    static struct sigaction sa;
    memset(&sa,0,sizeof(struct sigaction));
    sa.sa_sigaction = sigpipe_handler;
    sigaction(SIGPIPE, &sa,NULL);

    /*************************** Threading **********************/
    #if MULTITHREADED_OPERATION
    pthread_t thread;
      sem_init(&sem_protect,0,1);
      sem_init(&sem_wakeup,0,0);
    #endif
    while(1){
         printf("Waiting for incoming connections\n");
         s_TCP_remote = accept(s_TCP_server_listen, (struct sockaddr*) &sin_TCP_remote, &addrlen);
         printf("Connected to %s\n",inet_ntoa(sin_TCP_remote.sin_addr));
#if MULTITHREADED_OPERATION

         ringBuf_init(&rb_global, buf, BUFLEN);
         if(pthread_create(&thread, NULL, write_thread, NULL) < 0){
             printf("Failed to create thread\n");
             exit(1);
         }
#endif

         connected = 1;
          while(connected){
             //read from UDP socket
             nBytesReceived = recvfrom(s_UDP,(rcvBuf+sizeof(struct timespec_16bytes)),BUFLEN_SINGLEPKG-sizeof(struct timespec_16bytes),0,NULL,0);
            
             //create timestamp for the freshly received CSI data
             clock_gettime(CLOCK_REALTIME,&timeNow);
            
            //convert to 16 byte timespec structure (see beginning of this file)
             timeNow16.tv_sec = htobe64((uint64_t) timeNow.tv_sec);
             timeNow16.tv_nsec = htobe64((uint64_t) timeNow.tv_nsec);     
             *((struct timespec_16bytes*) rcvBuf) = timeNow16;         


             if((nBytesReceived<=0)&&(!connected)){
                   printf("disconnecting.\n");
                   break;
             }
#if MULTITHREADED_OPERATION
                //put this on the ringBuf and go ahead with receiving the next CSI data from the UDP socket. write_thread() will do the job of transmitting it over the TCP socket concurrently.
                ringBuf_write(&rb_global, rcvBuf, nBytesReceived+sizeof(struct timespec_16bytes));
#else
                //Immediately write this to the TCP socket
                write(s_TCP_remote,rcvBuf,nBytesReceived+sizeof(struct timespec_16bytes));
#endif
           }
           connected = 0;
           close(s_TCP_remote);
           //listen again for incoming connections
    }
    //The control flow should never arrive here.

#if MULTITHREADED_OPERATION
    pthread_join(thread, NULL);
#endif
    close(s_UDP);
    close(s_TCP_remote);
    close(s_TCP_server_listen);

    return 1;
}



