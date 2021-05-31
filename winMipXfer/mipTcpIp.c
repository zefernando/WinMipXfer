#include "mipTcpIp.h"
#include "mipXfer.h"

extern int Log_This(char *,char);
extern char Log_Msg[256];
extern void DumpMsg(unsigned char *, int , char *);
int mipSend(int , char *, size_t);
// int stripBlanks(char * , int );

int mipCONNECT(int , char * );
void mipDISCONNECT(int );

int err;
WSADATA info;

/* Converts ascii text to in_addr struct.  NULL is returned if the address
   can not be found. */
struct in_addr *atoaddr(char * address)
{
        struct hostent *host;
//        static struct in_addr saddr;

     
        host = gethostbyname(address);
        if (host != NULL) {
                return (struct in_addr *) *host->h_addr_list;
        }
        return (struct in_addr  *) NULL;
}


/* This is a generic function to make a connection to a given server/port.
   service is the port name/number,
   type is either SOCK_STREAM or SOCK_DGRAM, and
   netaddress is the host name to connect to.
   The function returns the socket, ready for action.*/
int mipCONNECT(int port, char * netaddress)
{
	
         struct sockaddr_in address;
 	 struct hostent     *hp;

        int sock, connected;

  	
	if (WSAStartup(MAKEWORD(2,0), &info)) {
			printf("Could not start WSA");
		}
        if (port == -1) {
                fprintf(stderr,"make_connection:  Invalid socket type.\n");
                return -1;
        }
  	hp = gethostbyname(netaddress);
  	if (hp == NULL) /* we don't know who this host is */
    	return -1;

 	 memset(&address,0,sizeof(address));
  	memcpy((char *)&address.sin_addr, hp->h_addr, hp->h_length);   /* set address */
  	address.sin_family = hp->h_addrtype;
  	address.sin_port = htons((u_short)port);

        
		
        

        sock = socket(AF_INET, SOCK_STREAM, 0);



        printf("Connecting to %s on port %d.\n",netaddress,port);

        connected = connect(sock, (struct sockaddr *) &address, sizeof(address));
        if (connected < 0) {
                perror("connect");
                return -1;
        }
        return sock;
}

void mipDISCONNECT(int sockfd)
{
        closesocket(sockfd);
}

int mipRecv(int sockfd,char * buf,int * count)
{
        unsigned char auxBuf[2];
	char *ptr;
        int this_read, bytes_read;
        int tam;
        int ret;
        memset(auxBuf,0, sizeof(auxBuf));
        this_read = recv(sockfd, (char *) auxBuf, 2, 0);
        
	DumpMsg(auxBuf, 2, "Tamanho"); 
        
        tam = auxBuf[0];
        tam =   (tam << 8 ) | auxBuf[1]; 
  	if(tam == 0 ) return -1;
	bytes_read = 0;
	this_read = 0;
	ptr = buf;
        while (bytes_read < tam) {
		
                do {
                        this_read = recv(sockfd, ptr, tam - bytes_read,0);
		     				err = WSAGetLastError();
                } while ( (this_read <0 ) && (err == WSAEINTR) );
                if (this_read <= 0)
                        break;
		// DumpMsg(buf, this_read, "MSG");
                bytes_read += this_read;
                ptr += this_read;
		this_read = 0;
        }
        if (tam !=  (bytes_read)) {
                sprintf(Log_Msg,"Tamanho errado na recepcao do MIP: esperado(%d) - recebido (%d)", tam, this_read);
                Log_This(Log_Msg, mpLOG_NORMAL);
                *count = bytes_read;
                ret = -1;
        }
        else {
		
	        //  *count = stripBlanks(buf, bytes_read);
		*count = bytes_read;
		sprintf(Log_Msg,"Tamanho  (%d)",  *count);
                Log_This(Log_Msg, mpLOG_NORMAL);
	      
                ret = 0;
        }
    
        return  ret;

}
/*
int stripBlanks(char * bp, int len)
{
	char *ptr;
	int newLen;
	int count;
	ptr = bp  + len - 1;
	newLen = len;
	count = 0;
	do {
		if(*ptr == 0x40) { 
		 --newLen;
		// sprintf(Log_Msg, "BRANCO %d  ", newLen);
		// Log_This(Log_Msg, mpLOG_NORMAL);
			ptr--;
			count++;
		}

	} while (*ptr == 0x40 && count < 2);

	return newLen;
}
*/

/* This is just like the write() system call, accept that it will
   make sure that all data is transmitted. */
int mipSend(int sockfd, char * buf, size_t count)
{
        unsigned char auxBuf[2048];
        size_t bytes_sent = 0;
        int this_write;
		

	printf("Count: %02x\n", count );
        
        memset(auxBuf,0, sizeof(auxBuf));
	auxBuf[0] = (char ) ((count & 0xff00) >> 8);
	auxBuf[1] = (char ) (count & 0x00ff);
	 
        memcpy(&auxBuf[2], buf, count);
		DumpMsg(auxBuf,21,"SendHeader");
        count += 2;
        while (bytes_sent < count) {
			do {
                this_write = send(sockfd, (const char *) auxBuf, count - bytes_sent, 0);
				err = WSAGetLastError();

			}while ( (this_write < 0) && (err == WSAEINTR) );
		sprintf(Log_Msg, "Bytes sent  %d  err %d ", this_write, err);
		 Log_This(Log_Msg, mpLOG_NORMAL);
            if (this_write <= 0)
                break;
            bytes_sent += this_write;
            buf += this_write;
        }
        return bytes_sent; 
}



