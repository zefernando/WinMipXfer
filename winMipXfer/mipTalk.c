/*
*       @(#)
*       @(#) MIP file transfer interface
*       @(#) Modulo mipTalk.c 1.0 09 Oct 2004
*       @(#) JFAB 2004 Jose Fernando Alvim Borges
*       @(#) ultima alteracao:
*       @(#)
*
*/

#include <fcntl.h>
#include <io.h>
#include <errno.h>

/* #include "sccsid.h" */
#include "mipXfer.h"
/* #include "mipUtil.ext" */

#define HEADER_LEN 62
/*
 * prototypes
 */
int mipParm_Build(char *,_mipParm_List **);
int mipParm_Scan(_mipParm_List **);
char * mipParm_Value(char *,_mipParm_List *);
int mipFile_OK(char *);
int Log_This(char *,char);
int checkMipResponse( unsigned char *Buffer, int *Type);
int Display_Header_004(unsigned  char * Buffer );
int Display_Trailer_998( unsigned char * Buffer );
int stripBlanks(unsigned char * , int );
extern void mipDISCONNECT(int );
extern int  conv_ascii( unsigned char * , unsigned char * , unsigned int );
extern int mipRecv(int ,char *, int * );
extern int mipCONNECT(int , char *);
extern void Dump_Message (FILE * , const char *, unsigned char *, int len);
extern int mipSend(int, char *, size_t);

extern void xferStat();


void conv_ebcdic (
        unsigned char * destino,
        unsigned char * origem,
        unsigned int tamanho
);

/*
 * prototypes END
 */

extern int fDebug;
extern char Log_Msg[256];

int Process_File_Header(
        unsigned char * Buffer,
        int Len,
        _Run * Run)
{
        /* struct stat stats; */
        int rc;
        char auxB[5];
        if ( fDebug )
        {
                Log_This("Entering Process_File_Header()", mpLOG_NORMAL);
                sprintf(Log_Msg,"Total message length = %d", Len);
                Log_This(Log_Msg, mpLOG_NORMAL );
                sprintf(Log_Msg,"Output File is %s", Run->Output_File);
                Log_This(Log_Msg, mpLOG_NORMAL );
        }
        /* Extrai o nome do arquivo no MIP header de 5 ateh 19 */
        /* Posicao 6  - Constante T
                7-9   - Tipo de arquivo
                10-14 - ENDPOINT
                15-17 - Julian Day 
                18-19 - Sequence Number */

        memcpy(Run->TID,Buffer + 5,14);
        if(Run->bOutput == 2)
        {
                conv_ascii((unsigned char *) Run->Output_File,(unsigned char *) Run->TID,14);
                memcpy(Run->Output_File+4,Run->Output_File+9,5);
                if(Run->Output_File[8] == 32)
                {
                        /* Tem um espaco no nome do arquivo */
                        Run->Output_File[8] = 0;
                }
                Run->Output_File[9] = 0;
                /* Grava o nome do arquivo no LOG */
                sprintf(Log_Msg,"*Output File is \"%s\"", Run->Output_File);
                Log_This(Log_Msg, mpLOG_NORMAL );
        }
        /* Cria o arquivo local */
        Run->fd_out = _open(Run->Output_File,_O_WRONLY|_O_CREAT|_O_EXCL, _S_IREAD | _S_IWRITE);
        if (Run->fd_out < 0)
        {
                switch(errno)
                {
                case EEXIST:
                        /* Arquivo jah existe */
                        sprintf(Log_Msg,"File %s on disk",
                                Run->Output_File);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        if ( Run->bForce )
                        {
                                /* Trunca o arquivo */
                                sprintf(Log_Msg,"File truncation specified" );
                                Log_This(Log_Msg, mpLOG_NORMAL);
                                Run->fd_out = _open(Run->Output_File,
                                                   _O_WRONLY|_O_TRUNC|_O_CREAT);
                                if (Run->fd_out < 0)
                                {
                                        /* Erro na criacao do arquivo */
                                        sprintf(Log_Msg,"Could not create %s",
                                                Run->Output_File);
                                        Log_This(Log_Msg, mpLOG_NORMAL);
                                        sprintf(Log_Msg,"system reported \"%s\"",
                                                strerror(errno));
                                        Log_This(Log_Msg, mpLOG_NORMAL);
                                        Run->Err_Msg=malloc(strlen(Log_Msg));
                                        strcpy(Run->Err_Msg,Log_Msg);
                                        return(-1);
                                }
                                sprintf(Log_Msg,"file %s is open (%d)",
                                        Run->Output_File, Run->fd_out );
                                Log_This(Log_Msg, mpLOG_NORMAL);
                                /* Altera as permissoes para rw-rw-rw */
                                rc = _chmod(Run->Output_File, 00666);
                                if(rc < 0)
                                {
                                        sprintf(Log_Msg,"*fchmod returned (%d)", rc);
                                        Log_This(Log_Msg, mpLOG_NORMAL);
                                        sprintf(Log_Msg,"*system reported \"%s\"",
                                                strerror(errno));
                                        Log_This(Log_Msg, mpLOG_NORMAL);
                                        return(-1);
                                }
                                break;
                        }
                        else
                        {
                                /* Arquivo jah existe e nao deve ser apagado */
                                sprintf(Log_Msg,
                                        "File already on disk AND truncation NOT specified" );
                                Log_This(Log_Msg, mpLOG_NORMAL);
                                Run->Err_Msg=malloc(strlen(Log_Msg));
                                strcpy(Run->Err_Msg,Log_Msg);
                                return(-1);
                        }
                default:
                        sprintf(Log_Msg,"Could not create %s",
                                Run->Output_File);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        sprintf(Log_Msg,"system reported \"%s\"",
                                strerror(errno));
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        Run->Err_Msg=malloc(strlen(Log_Msg));
                        strcpy(Run->Err_Msg,Log_Msg);
                        return(-1);
                }       /* end switch */
        }
        else
        {
                /* Arquivo estah criado */
                sprintf(Log_Msg,"File %s is OPEN (%d)",
                        Run->Output_File, Run->fd_out);
                Log_This(Log_Msg, mpLOG_NORMAL);
                rc = _chmod(Run->Output_File, 00666);
                if(rc < 0)
                {
                        /* Nao conseguiu alterar as permissoes do arquivo */
                        sprintf(Log_Msg,"*fchmod returned (%d)", rc);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        sprintf(Log_Msg,"*system reported \"%s\"",
                                strerror(errno));
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        return(-1);
                }
        }       /* end if */
        /* Obtem a hora de inicio */
        time(&Run->StartT);
        Run->intCount = 0;
        /* 1st block may follow header */
        if ( Run->bTest_Mode )
        {
                /* under test mode the block count is already set */
                memcpy(&Run->Block_Count,Buffer + 23,4);
                sprintf(Log_Msg,"*File Size SET to %d Blocks",
                        Run->Block_Count);
                Log_This(Log_Msg, mpLOG_NORMAL);
        }
        else
        {
                /* Extrai o numero de blocos a ser transferido do header (posicao 36) */
		
		memcpy(auxB,Buffer+36,4);
	
		auxB[4] = '\0';
//		Run->Block_Count,Buffer = atoi(auxB);
                // memcpy(&Run->Block_Count,Buffer+36,4);
			Run->Block_Count =   auxB[1] << 16 |  auxB[2] << 8 | auxB[3];
	            sprintf(Log_Msg,"*MIP REPORTED file BLOCK COUNT as %d Blocks",
                        Run->Block_Count);
                Log_This(Log_Msg, mpLOG_NORMAL);
        }
        /* Transferencia iniciada */
        Log_This("*File Transfer STARTED",mpLOG_NORMAL);
        if ( Len <= HEADER_LEN )
        {
                /* no data with first block: header only */
                Run->Block_Number = -1;
                return(0);
        }
        if(Run->bASCII_Output)
        {
                conv_ascii(Buffer+HEADER_LEN,Buffer+HEADER_LEN,Len-HEADER_LEN);
        }
        /* Grava no arquivo local */
        rc = _write( Run->fd_out, Buffer + HEADER_LEN, Len-HEADER_LEN );
        if ( rc < 0 )
        {
                sprintf(Log_Msg,"Write error \"%s\"",strerror(errno));
                Log_This(Log_Msg, mpLOG_NORMAL);
        }
        Run->Block_Number = 1;
        Run->Bytes_Received = Len - HEADER_LEN;
        Run->Block_Len = Len - HEADER_LEN;
        sprintf(Log_Msg,"*Block Size is SET to %d", Run->Block_Len);
         Log_This(Log_Msg,mpLOG_NORMAL);
        
        return(0);
}       /* end of Process_File_Header() */





int Send_Data_Acknowledgment(_Run * Run)
{
        unsigned char * AHeader;
        int rc;
        AHeader = malloc(13);
        memcpy(AHeader,"1980000SSSSSS",13);
        memcpy(AHeader+7,Run->Advisement_Session,6);
        conv_ebcdic( AHeader, AHeader, 13 );
        if ( fDebug )
        {
                Log_This("SENDING DATA ACK", mpLOG_NORMAL);
                Dump_Message(Run->fd_log,"Data Acknowledgment Message ->",
                             AHeader,13);
        }
        rc =mipSend(Run->socket, (char *) AHeader,13);
        if ( fDebug )
        {
                sprintf(Log_Msg,"mipSend() = %d",rc);
                Log_This(Log_Msg, mpLOG_NORMAL);
        }
        free(AHeader);
        if (rc <=0)
        {
                Log_This("Abort: error on mipSend()", mpLOG_NORMAL);
                return(-1);
        }
        return(rc);
}       /* end of Send_Data_Acknowledgment() */


int Process_Advisement_Start(_Run * Run)
{
        int rc;
        Run->fd_out = _open(Run->Output_File,_O_WRONLY|_O_CREAT|_O_EXCL, _S_IREAD | _S_IWRITE);
        if(Run->fd_out>=0)
        {
                sprintf(Log_Msg,"File %s is OPEN (%d)",
                        Run->Output_File, Run->fd_out);
                Log_This(Log_Msg, mpLOG_NORMAL);
                rc = _chmod(Run->Output_File, 00666);
                if(rc<0)
                {
                        sprintf(Log_Msg,"*fchmod returned (%d)", rc);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        sprintf(Log_Msg,"*system reported \"%s\"",
                                strerror(errno));
                        Log_This(Log_Msg, mpLOG_NORMAL);
                }
                time(&Run->StartT);
                Run->intCount = 0;
                return(0);
        }

        switch(errno)
        {
        case EEXIST:
                sprintf(Log_Msg,"File %s on disk",
                        Run->Output_File);
                Log_This(Log_Msg, mpLOG_NORMAL);
                if ( Run->bForce )
                {
                        sprintf(Log_Msg,"File truncation specified" );
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        Run->fd_out = _open(Run->Output_File,
                                          _O_WRONLY|_O_TRUNC|_O_CREAT);
                        if (Run->fd_out<0)
                        {
                                sprintf(Log_Msg,"Could not create %s",
                                        Run->Output_File);
                                Log_This(Log_Msg, mpLOG_NORMAL);
                                sprintf(Log_Msg,"system reported \"%s\"",
                                        strerror(errno));
                                Log_This(Log_Msg, mpLOG_NORMAL);
                                return(-1);
                        }
                        sprintf(Log_Msg,"file %s is open (%d)",
                                Run->Output_File, Run->fd_out );
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        rc = _chmod(Run->Output_File, 00666);
                        if(rc<0)
                        {
                                sprintf(Log_Msg,"*fchmod returned (%d)", rc);
                                Log_This(Log_Msg, mpLOG_NORMAL);
                                sprintf(Log_Msg,"*system reported \"%s\"",
                                        strerror(errno));
                                Log_This(Log_Msg, mpLOG_NORMAL);
                        }
                        break;
                }
                else
                {
                        sprintf(Log_Msg,"File truncation NOT specified" );
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        return(-1);
                }
        default:
                sprintf(Log_Msg,"Could not create %s",
                        Run->Output_File);
                Log_This(Log_Msg, mpLOG_NORMAL);
                sprintf(Log_Msg,"system reported \"%s\"",
                        strerror(errno));
                Log_This(Log_Msg, mpLOG_NORMAL);
                return(-1);
        }       /* end switch */
        time(&Run->StartT);
        Run->intCount = 0;
        return(0);
}       /* end of Process_Advisement_Start() */


int Process_Advisement_Response(
        unsigned char * Buffer,
        int Len,
        _Run * Run)
{
        char Line[80];
/*        struct stat stats; */
        int rc;
        if ( fDebug )
        {
                Log_This("Entering Process_Advisement_Response()",
                         mpLOG_NORMAL);
                sprintf(Log_Msg,"Total message length = %d", Len);
                Log_This(Log_Msg, mpLOG_NORMAL );
                sprintf(Log_Msg,"Output File is %s", Run->Output_File);
                Log_This(Log_Msg, mpLOG_NORMAL );
        }
        if( Run->Block_Number == 0)
        {
                if (Process_Advisement_Start(Run)!=0)
                {
                        return(-1);
                }
        }
        /* /////////////////////////////////////////////////////////// */
        Run->Block_Number += 1;
        Run->Bytes_Received += Len;

        memcpy(&Run->File_Size, Buffer+74, 4);
        memcpy(&Run->Block_Len, Buffer+94, 2);
        memcpy(&Run->Record_Len,Buffer+96, 2);

        conv_ascii(Buffer,Buffer,Len);
        *(Buffer+Len)=0;
        memset(Line,32,80);
        memcpy(Line,Buffer+11,14);
        sprintf(Line+16,"%8d",Run->File_Size);
        *(Line+24)=32;
        memcpy(Line+25,Buffer+100,3);
        memcpy(Line+29,Buffer+103,4);
        *(Line+33)='/';
        memcpy(Line+34,Buffer+107,3);
        memcpy(Line+38,Buffer+43,1);
        sprintf(Line+40,"%03d",Run->Block_Len);
        *(Line+43)='/';
        sprintf(Line+44,"%03d",Run->Record_Len);
        *(Line+47)=32;
        memcpy(Line+48,Buffer+110,3);
        *(Line+51)='-';
        memcpy(Line+52,Buffer+113,2);
        *(Line+54)=':';
        memcpy(Line+55,Buffer+115,2);
        *(Line+57)=':';
        memcpy(Line+58,Buffer+117,2);
        *(Line+60)=0;
        if( Run->Block_Number == 1)
        {
                sprintf(Log_Msg,
                        "*    Bulk ID        SIZE  DAY  EXPIRE  S  RECORD  RECEIVED\n");
                /*   BBBBBBBBBBBBBB NNNNNNNN CCC YYYY/DDD S BBB/RRR DDD-hh:MM:ss */
                Log_This(Log_Msg, mpLOG_NORMAL);
        }
        sprintf(Log_Msg,"*%s",Line);
        Log_This(Log_Msg, mpLOG_NORMAL);
        *(Line+60)=10;
        rc = _write(Run->fd_out,Buffer,Len);
        return(0);
}       /* end of Process_Advisement_Response() */

/* Recebe arquivo do MIP */
int mipReceive_File(_Run * Run)
{
        int rc;
        int mipBlocks = 0;
        unsigned char * THeader;
        unsigned char Buffer[DATA_BUFSIZE];
        int Len;
        int Status = stCONNECT;
        char lstr[DATA_BUFSIZE+1];
        char sBuf[DATA_BUFSIZE];
	 char auxB[5];
        int Fail=0;

        while (1) {
                memset(lstr,0,DATA_BUFSIZE+1);
                switch(Status)
                {
                case stCONNECT:
                /*        Set_Term(0,1); */
                        if (fDebug)
                        {
                                Log_This("CONNECT CONNECT CONNECT CONNECT CONNECT", mpLOG_NORMAL);
                                Log_This("Starting File Receive", mpLOG_NORMAL);
                        }
                        Run->socket = mipCONNECT(atoi(Run->Socket_Port), Run->IpAddress);
                        if (Run->socket != -1 )
                        {
                                /* Conectado ao MIP */
                                sprintf(Log_Msg,
                                        "*CONNECTED to Socket channel \"%s\" IpAddress \"%s\" fd is \"%d\"",
                                        Run->Socket_Port,
                                        Run->IpAddress,
                                        Run->socket);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                        }
                        else
                        {
                                /* A conexao com o MIP falhou */
                                sprintf(Log_Msg,
                                        "*CONNECT to socket channel \"%s\" subnet \"%s\" FAILED, code is \"%d\"",
                                        Run->Socket_Port,
                                        Run->IpAddress,
                                        rc);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                                Run->Err_Msg=malloc(15);
                                strcpy(Run->Err_Msg,"CONNECT failed");
                                Fail = 1;
                                Status = stDISCONNECT;
                                break;
                        }
                        /* Proximo status eh: Requisita arquivo */
                        Status = stREQUESTBULK;
                        break;
                case stREQUESTBULK:
                        /* Requisita o arquivo */
                        THeader = malloc(19);
                        /* Prepara o Header para requisitar o arquivo */
                        memcpy(THeader,"10101BBBBEEEEE     ",19);
                        /* Copia o tipo */
                        memcpy(THeader+5,Run->Bulk_Type, 4);
                        /* Copia o ENDPOINT */
                        memcpy(THeader+9,Run->Endpoint, 5);
                        memset(THeader+14,0,5);
                        if ( Run->bJulian_Date )
                        {
                                /* Copia a data juliana */
                                memcpy(THeader+14,Run->Julian_Date, 3);
                                if ( Run->bSequence_Number )
                                {
                                        /* Copia o sequence Number */
                                        /* Dentro de um periodo de 24 hrs o SN eh crescente */
                                        memcpy(THeader+17,Run->Sequence_Number, 2);
                                }
                        }
                        /* Converte para EBCDIC */
                        conv_ebcdic( THeader, THeader, 19 );
                        if ( fDebug )
                        {
                                Log_This("REQUEST REQUEST REQUEST REQUEST REQUEST ",
                                         mpLOG_NORMAL);
                                Dump_Message(Run->fd_log,"Bulk File Request ->", THeader, 19);
                        }
                        /* Envia a requisicao para o MIP */
                        rc = mipSend(Run->socket, (char * ) THeader, 19);
                        if ( fDebug )
                        {
                                sprintf(Log_Msg,"mipSend() = %d",rc);
                                Log_This(Log_Msg, mpLOG_NORMAL);
                        }
                        free(THeader);
                        if (rc <= 0)
                        {
                                /* Ocorreu erro no envio */
                                Log_This("Abort: error on mipSend()", mpLOG_NORMAL);
                                Status = stDISCONNECT;
                                break;
                        }
                        /* Proximo status eh: Recebe o Header do MIP */
                        Status = stWAITFORHEADER;
                        break;
                case stWAITFORHEADER:
                        /* Recebe o Header do MIP */
                        if ( fDebug )
                        {
                                Log_This("WAIT HDR WAIT HDR WAIT HDR WAIT HDR ", mpLOG_NORMAL);
                        }
                        Len = DATA_BUFSIZE;
                        /* Recebe o header do MIP */
                        rc = mipRecv( Run->socket, (char *) Buffer, &Len );
                        if ( fDebug )
                        {
                                sprintf(Log_Msg,"mipRecv returned %d, Len %d",rc,Len);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                                if ( (rc == 0) && (Len > 0) )
                                {
                                        Dump_Message(Run->fd_log,"Data from MIP ->",Buffer,Len );
                                }
                        }
                        if ( rc != 0 )
                        {
                                Status = stDISCONNECT;
                                Fail=1;
                                break;
                        }
                        if ( Len < 5 )
                        {
                                /* Tamanho do header eh menor que o esperado */
                                Log_This("Message possibly truncated. Aborting",
                                         mpLOG_NORMAL);
                                Status = stDISCONNECT;
                                break;
                        }
                        /* ok: we have a response from MIP */
                        if ((*Buffer==0xf0)&&(*(Buffer+1)==0xf0)&&(*(Buffer+2)==0xf4))
                        {
                                /* and we have a header */
                                Log_This("*Bulk File Header Received", mpLOG_NORMAL);
                                /* Processa o Header do arquivo */
                                rc = Process_File_Header(Buffer,Len,Run);
                                if ( rc != 0 )
                                {
                                        Run->iPhase = Status = stDISCONNECT;
                                        Fail = 1;
                                }
                                else
                                {
                                        /* O mipXfer esta recebendo o arquivo */
                                        Run->iPhase = Status = stRECEIVING;
                                }
                                break;
                        }
                        if ((*Buffer==0xf9)&&(*(Buffer+1)==0xf9)&&(*(Buffer+2)==0xf8))
                        {
                                /* O MIP reportou um erro na requisicao do arquivo */
                                Log_This("ERROR message reported by MIP follows", mpLOG_NORMAL);
                                /* Converte a mensagem de erro para ASCII */
                                conv_ascii((unsigned char *  ) (1+lstr),(unsigned char *  ) (Buffer+7),Len-7);
                                *lstr='*';
                                Log_This(lstr,mpLOG_NORMAL);
                                Run->Err_Msg=malloc(strlen(lstr));
                                strcpy(Run->Err_Msg,lstr);
                                Fail=1;
                                Status = stDISCONNECT;
                                break;
                        }
                        Log_This("Message RECEIVED OK, BUT NOT 004 NOT 99X",mpLOG_NORMAL);
                        Status = stDISCONNECT;
                        break;
                case stDISCONNECT:
                        if ( fDebug )
                        {
                                Log_This("DISCONNECT DISCONNECT DISCONNECT DISCONNECT DISCONNECT ",mpLOG_NORMAL);
                        }
                        sprintf(Log_Msg,"%d Bytes Received in %d Blocks",
                                Run->Bytes_Received, Run->Block_Number);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        mipDISCONNECT(Run->socket);
                        sprintf(Log_Msg,"TCP DISCONNECT (%d)",rc);
                        Log_This(Log_Msg,mpLOG_NORMAL);
                        Status = stEND;
                        break;
                case stRECEIVING:
                        if ( fDebug )
                        {
                                Log_This("RECEIVE RECEIVE RECEIVE RECEIVE RECEIVE ",
                                         mpLOG_NORMAL);
                        }
                        Len = 0;
                        rc = mipRecv( Run->socket, (char *) Buffer, &Len );
                        if ( rc != 0 )
                        {
                                sprintf(Log_Msg,"Error after %d bytes received",
                                        Run->Bytes_Received);
                                Log_This(Log_Msg, mpLOG_NORMAL );
                                Fail = 1;
                                _close( Run->fd_out );
                                Status = stDISCONNECT;
                                break;
                        }
						/*
                        if(_read(0,sBuf,1)>0)
                        {
                               //  Usuario solicitu estatisca de transferencia 
                                xferStat();
                                if(Run->bTimeout==3)
                                {
                                        Fail = 1;
                                        _close( Run->fd_out );
                                        Status = stDISCONNECT;
                                        break;
                                }
                        }
						*/
                        if ( fDebug )
                        {
                                /* Modo debug */
                                if(Len == 0)
                                {
                                        /* Numero de bytes recebidos eh zero */
                                        Log_This("Received o bytes", mpLOG_NORMAL);
                                        break;
                                }
                                else
                                {
                                        if(fDebug)
                                        {
                                                /* Faz um dump hexa dos dados recebidos */
                                                Dump_Message(Run->fd_log,"Data from MIP (2) ->",Buffer,Len);
                                        }
                                }
                        }
                        if ( Buffer[0] == 0xE3 )
                        {
                                /* cada bloco deve iniciar com 0xE3 */
                                if(Run->Block_Number<=0)
                                {
                                        sprintf(Log_Msg,"*Block Size is SET to %d", Len-1);
                                        Log_This(Log_Msg,mpLOG_NORMAL);
                                        Run->Block_Len = Len-1;
                                }
                                else
                                {
                                        if(     (Run->bReport_Frequency)&&
                                                        ((Run->Block_Number%Run->bReport_Frequency)
                                                         ==(Run->bReport_Frequency-1))
                                          )
                                        {
                                                /* Mostra estatisticas */
                                                xferStat();
                                                if(Run->bTimeout==3)
                                                {
                                                        Fail = 1;
                                                        _close( Run->fd_out );
                                                        Status = stDISCONNECT;
                                                        break;
                                                }
                                        }
                                }
                                if(Run->bASCII_Output)
                                {
                                        /* Converte saida para ASCII */
                                        conv_ascii(Buffer+1,Buffer+1,Len-1);
                                }
			   else {
				Len = stripBlanks(Buffer, Len);

			   }
                                /* Grava no arquivo local */
			    if(Len > 0)
                                rc = _write( Run->fd_out, Buffer+1, (Len-1) );
                                if ( rc < 0 )
                                {
                                        /* Erro na gravacao do arquivo */
                                        perror("write");
                                        Status = stDISCONNECT;
                                }
                                else
                                {
                                        /* Incrementa o numero de bytes recebidos */
                                        Run->Bytes_Received+=rc;
                                        /* incrementa o número de blocos recebidos */
                                        Run->Block_Number++;
                                }
                        }
                        else
                        {
                                Status = stENDING;
                        }
                        break;
                case stENDING:
                        /* may be we have a 998 trailer */
                        if(fDebug)
                        {
                                Log_This("ENDING ENDING ENDING ENDING ENDING ",mpLOG_NORMAL);
                        }
                        Dump_Message(Run->fd_log,"Data from MIP ->",Buffer,Len);
                        if ((*Buffer==0xf9)&&(*(Buffer+1)==0xf9)&&(*(Buffer+2)==0xf8))
                        {
                                /* Trailer 998  */
                                if (*(Buffer + 6)==0xf1)
                                {
                                        /* Buffer[6] eh o return code */
                                        /* Ocorreu erro, transferencia foi encerrada pelo MIP */
                                        sprintf(Log_Msg,"Transfer aborted by MIP");
                                        Log_This(Log_Msg,mpLOG_NORMAL);
                                        if (Len>7)
                                        {
                                                Run->Err_Msg=malloc(1+Len);
                                                Run->Err_Msg[0]='*';
                                                memcpy(Run->Err_Msg,1+Buffer+6,1+Len-6);
                                                conv_ascii((unsigned char *) Run->Err_Msg,(unsigned char * ) Run->Err_Msg,1+Len-6);
                                                *(Run->Err_Msg+Len-6+1+1)=0;
                                                Log_This(Run->Err_Msg, mpLOG_NORMAL);
                                        }
                                        Status= stDISCONNECT;
                                        break;
                                }
                                /* Numero de blocos que o MIP informou ter enviado */
                                memcpy(auxB,Buffer+7,4);
				      mipBlocks =   auxB[1] << 16 |  auxB[2] << 8 | auxB[3];
                                sprintf(Log_Msg,"*MIP reports %d blocks sent",mipBlocks);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                                /* Numero de blocos computado pelo mipXfer */
                                sprintf(Log_Msg,"*HOST reports %d blocks read",
                                        Run->Block_Number + 2);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                                if(mipBlocks == ( 2 + Run->Block_Number))
                                {
                                        /* Todos os blocos foram tranferidos */
                                        Log_This("*Sending PURGE REQUEST", mpLOG_NORMAL);
                                        /* Solicita que o arquivo seja apagado */
                                        Status = stPURGE;
                                        break;
                                }
                        }
                        /* O numero de blocos informado pelo MIP e o contado nao batem */
                        /* O arquivo nao foi transferido corretamente */
                        Log_This("*PURGE REQUEST supressed: block count does not match",
                                 mpLOG_NORMAL);

                        Log_This("*file transfer is now ENDING", mpLOG_NORMAL);
                        _close(Run->fd_out);
                        Status = stDISCONNECT;
                        break;
                case stPURGE:
                        /* Solicita que o arquivo seja apagado */
                        /* Prepara o header */
                        memcpy(Buffer,"9990100              ",21);
                        /* Converte para EBCDIC */
                        conv_ebcdic(Buffer, Buffer, 21);
                        memcpy(7+Buffer,Run->TID,14);
                        if (fDebug)
                        {
                                Dump_Message(Run->fd_log,"PURGE request ->",Buffer,21);
                        }
                        /* Envia a requisicao de apagar para o MIP */
                        rc = mipSend(Run->socket, (char * )Buffer, 21);
                        if( rc <= 0)
                        {
                                Log_This("*Error sending PURGE request", mpLOG_NORMAL);
                                Status = stDISCONNECT;
                                break;
                        }
                        Status = stCONFIRMPURGE;
                        break;
                case stCONFIRMPURGE:
                        /* Obtem a confirmacao do apagamento do arquivo no MIP */
                        if(fDebug)
                        {
                                Log_This("CONFIRM PURGE CONFIRM PURGE ",mpLOG_NORMAL);
                        }
                        Len = DATA_BUFSIZE;
                        /* Recebe a confirmacao do MIP */
                        rc = mipRecv( Run->socket, (char *) Buffer, &Len );
                        if ( rc != 0 )
                        {
                                sprintf(Log_Msg,"*Error receiving PURGE confirmation");
                                Log_This(Log_Msg, mpLOG_NORMAL);
                                Status = stDISCONNECT;
                                break;
                        }
                        if(Len == 0)
                        {
                                Status = stDISCONNECT;
                                break;
                        }
                        if (fDebug)
                        {
                                /* Faz um dump hexa da mensagem de confirmacao */
                                Dump_Message(Run->fd_log,
                                             "PURGE request response ->",Buffer,Len);
                        }
                        if (
                                (* Buffer   ==0xf9)&&
                                (*(Buffer+1)==0xf9)&&
                                (*(Buffer+2)==0xf8)&&
                                (*(Buffer+3)==0xf0)&&
                                (*(Buffer+4)==0xf1)&&
                                (*(Buffer+5)==0xf0)&&
                                (*(Buffer+6)==0xf0)             )
                        {
                                /* Arquivo foi apagado do MIP */
                                Log_This("*PURGE request confirmed",mpLOG_NORMAL);
                        }
                        else
                        {
                                Log_This("Error on PURGE request. MIP message follows",
                                         mpLOG_NORMAL);
                                /* Converte mensagem de erro para ASCII */
                                conv_ascii((unsigned char *)sBuf+1,Buffer,Len);
                                sBuf[0]='*';
                                sBuf[1+Len]=0;
                                Log_This(sBuf, mpLOG_NORMAL);
                                Run->Err_Msg= (char *) malloc(Len+2);
                                strcpy(Run->Err_Msg,sBuf);
                                Fail = 1;
                        }
                        Status = stDISCONNECT;
                        break;
                case stEND:
                        /* Termina */
                        /* Set_Term(0,0); */
                        if (fDebug)
                        {
                                Log_This("END END END END END ",mpLOG_NORMAL);
                        }
                        Log_This("*bye",mpLOG_NORMAL);
                        if (Fail) return(-1);
                        if (Run->iTimeout) return(-1);
                        return(0);
                default:
                        break;
                }       /* end switch */
        }
		return 0;
}       /* end of mipReceive_File() */

int stripBlanks(unsigned char * bp, int len)
{
	unsigned char *ptr;
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


int mipSend_File(_Run * Run)
{
	char                    Request004[61];
	char                    Request998[12];
	int                     r;
	int						rc;
	int                     DataType;
	unsigned char           DataBuffer[1024];
	struct _stat		FileInfo;
	char *			parm = "BLKSIZE_000";
	int			Status = stCONNECT;
	int			in;
	int			Len,Fail, BytesSoFar;
	time_t		StartT, EndT;

	memset( Request004, 32, 61 );
	memcpy( Request004, "00401", 5 );
	memcpy( Request004 + 5, Run->Bulk_Type, 4);
	memcpy( Request004 + 9, Run->Endpoint, 5);
	memcpy( Request004 + 14, Run->Julian_Date, 3);
	memcpy(Request004 + 17, Run->Sequence_Number, 2);

	memset( Request998, 32, 12 );
	memcpy( Request998, "99801000000", 11 );
	if ( fDebug )
	{
		printf (
		    "\nHeader Record [004] %03d bytes :\t[%s]\n",
		    strlen(Request004),
		    Request004
		    );
		printf (
		    "End of Data Record [998] %03d bytes :\t[%s]\n\n",
		    strlen(Request998),
		    Request998
		    );
	} /* end if */
//	printf("Antes do Conv\n");
	conv_ebcdic( 
		(unsigned char *) &Request004[0],
		(unsigned char *) &Request004[0], 60 );
	conv_ebcdic( 
		(unsigned char *) &Request998[0],
		(unsigned char *) &Request998[0], 11 );
//	printf("Depois do conv\n");

	memset( Request004+36, 0, 4 );	/* file size MIP (future) */
	memset( Request004+40, 0, 4 );	/* sequence number (future) */
	memcpy( Request004+36, &Run->iFileSize, 4 );	/* file size MIP (future) */
	memcpy( Request004+40, &Run->iRSequence, 4 );	/* sequence number (future) */

	/* Open file to transfer */
	in = _open(Run->sFileName, _O_RDONLY );
	if ( in == -1 )
	{
		perror("input file");
		return -1;
	}
	Status = stCONNECT;
	_fstat(in, &FileInfo);
	sprintf(Log_Msg,"File #%s is open. Local name \'%s\'\n",
		Run->Bulk_Type, Run->sFileName );
	Log_This(Log_Msg,mpLOG_NORMAL);
	sprintf(Log_Msg, "File size %ld bytes.\n", FileInfo.st_size );
	Log_This(Log_Msg,mpLOG_NORMAL);
//	Run->Block_Len = 1014;
//	Run->Block_Len = 900;
	/*
	 * a parameter (in the config file) named BLKSIZE_nnn 
	 * must exist for each and every BulkFile nnn. 
	 * A default of 1014 is assumed. Apr 10, 97
	 */
//	printf("Procura texto\n");
//	memcpy( parm+8, Run->Bulk_Type + 1, 3 );
	/*
	if ( Procura_Texto( parm, valor ) == 0 )
	{
		Run->Block_Len = 1014;
		sprintf(Log_Msg, "\tblock size not specified. Defaulting to %d bytes\n",
			Run->Block_Len );
			Log_This(Log_Msg,mpLOG_NORMAL);
		
	}
	else
	{
		Run->Block_Len = atoi(valor);
		sprintf(Log_Msg,"\tblock size set to %d bytes\n", Run->Block_Len );
		Log_This(Log_Msg,mpLOG_NORMAL);
	} 
	*/
	/* if */
	printf("File Size\n");
	if ( FileInfo.st_size < Run->Block_Len )
	{
		Run->Block_Count = 1;
	}
	else
	{
		Run->Block_Count = FileInfo.st_size / Run->Block_Len;
		if ( Run->Block_Count % Run->Block_Len != 0 ) Run->Block_Count++;
	}
//	printf( "\t%ld blocks will be sent to MIP\n",	Run->Block_Count );
	Log_This( "%d blocks will be sent to MIP\n",
		Run->Block_Count );
	do
	{
		switch ( Status )
		{
		case stCONNECT:
                        /* Set_Term(0,1); */
		    printf("Connect\n");
                        if (fDebug)
                        {
                                Log_This("CONNECT CONNECT CONNECT CONNECT CONNECT", mpLOG_NORMAL);
                                Log_This("Starting File Receive", mpLOG_NORMAL);
                        }
                        Run->socket = mipCONNECT(atoi(Run->Socket_Port), Run->IpAddress);
                        if (Run->socket != -1 )
                        {
                                /* Conectado ao MIP */
                                sprintf(Log_Msg,
                                        "*CONNECTED to Socket channel \"%s\" IpAddress \"%s\" fd is \"%d\"",
                                        Run->Socket_Port,
                                        Run->IpAddress,
                                        Run->socket);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                        }
                        else
                        {
                                /* A conexao com o MIP falhou */
                                sprintf(Log_Msg,
                                        "*CONNECT to socket channel \"%s\" subnet \"%s\" FAILED, code is \"%d\"",
                                        Run->Socket_Port,
                                        Run->IpAddress,
                                        rc);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                                Run->Err_Msg=malloc(15);
                                strcpy(Run->Err_Msg,"CONNECT failed");
                                Fail = 1;
                                Status = stDISCONNECT;
                                break;
                        }
                        /* Proximo status eh: Requisita arquivo */
                        Status = stREQUESTBULK;
                        break;	
		case stREQUESTBULK: /* sending file transfer request */
		   	rc = mipSend(Run->socket,Request004, 60);
                        if ( fDebug )
                        {
                                sprintf(Log_Msg,"mipSend() = %d",rc);
                                Log_This(Log_Msg, mpLOG_NORMAL);
                        }
                        
			if (rc <= 0)
                        {
                                /* Ocorreu erro no envio */
                                Log_This("Abort: error on mipSend()", mpLOG_NORMAL);
                                Status = stDISCONNECT;
                                break;
                        }
		
			Log_This("Bulk File transfer request sent\n", mpLOG_NORMAL);
			Status = stWAITFORHEADER;
			break;
		case stDISCONNECT:
                        if ( fDebug )
                        {
                                Log_This("DISCONNECT DISCONNECT DISCONNECT DISCONNECT DISCONNECT ",mpLOG_NORMAL);
                        }
                        sprintf(Log_Msg,"%d Bytes Sent in %d Blocks",
                                Run->Bytes_Received, Run->Block_Number);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        mipDISCONNECT(Run->socket);
                        sprintf(Log_Msg,"TCP DISCONNECT (%d)",rc);
                        Log_This(Log_Msg,mpLOG_NORMAL);
                        Status = stEND;
                        break;
		case stWAITFORHEADER: /* looking for 998 from MIP */
			Len = DATA_BUFSIZE;
                        /* Recebe o header do MIP */
                        rc = mipRecv( Run->socket, (char *) &DataBuffer[0], &Len );
                        if ( fDebug )
                        {
                                sprintf(Log_Msg,"mipRecv returned %d, Len %d",rc,Len);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                                if ( (rc == 0) && (Len > 0) )
                                {
                                        Dump_Message(Run->fd_log,"Data from MIP ->",DataBuffer,Len );
                                }
                        }
                        if ( rc != 0 )
                        {
                                Status = stDISCONNECT;
                                Fail=1;
                                break;
                        }
			checkMipResponse(DataBuffer, &DataType);
			if ( DataType == T_GoodReturnRecord )
			{
				Log_This("MIP is ready to send file\n",mpLOG_NORMAL);
				Status = stSENDING;
			}
			else
			{
				Status = stDISCONNECT; 
			}
			break;
		case stSENDING: /* Sending Data to MIP */
//			memset(DataBuffer, 0x40, sizeof(DataBuffer));
			DataBuffer[0] = 0xd9;	/* 'R' */
			while ( (r = _read( in, &DataBuffer[1], Run->Block_Len ) ) > 0 )
			{
		//		DataBuffer[0] = 0xd9;	/* 'R' */
				if ( Run->Block_Number == 0 )
				{
					Log_This("Bulk File Transfer starting now\n",mpLOG_NORMAL);
					time(&StartT);
				}
				Run->Block_Number++; 
				BytesSoFar += r+1;
				BytesSoFar += r;
                                      Dump_Message(Run->fd_log,"Data to MIP (2) ->",DataBuffer,  r );
				rc = mipSend(Run->socket, (char *) DataBuffer, 1 + r);
				if ( fDebug )
				{
					sprintf(Log_Msg,"mipSend() = %d",rc);
					Log_This(Log_Msg, mpLOG_NORMAL);
				}
                       
				if (rc <= 0)
				{
                                /* Ocorreu erro no envio */
                                	Log_This("Abort: error on mipSend()", mpLOG_NORMAL);
					Status = stDISCONNECT;
					break;
				}
				// memset(DataBuffer, 0x40, sizeof(DataBuffer));
				// DataBuffer[0] = 0xd9;	/* 'T' */
			} /* while */
			if ( r < 0 )
			{
				perror("read");
			}
			_close(in);
			Status = stSENDTRAILER;
			break;
		case stSENDTRAILER: /* send trailer to MIP */
			time(&EndT);
			sprintf(Log_Msg,"%ld bytes transferred in %d secs at %4.2f bytes/sec\n",
				BytesSoFar, (EndT - StartT), (float) BytesSoFar/(EndT-StartT) );
			Log_This(Log_Msg,mpLOG_NORMAL);
			
			Run->Block_Number++;
//			Run->Block_Number+=2;
			Request998[7] = 0;
			Request998[8] = 0;
			Request998[9] = (char ) ((Run->Block_Number & 0xff00) >> 8);
			Request998[10] = (char ) (Run->Block_Number & 0x00ff);
			// memcpy( Request998+7, &Run->Block_Number, 4 );
			sprintf(Log_Msg,"Sending trailer to MIP, %d blocks sent\n", Run->Block_Number );
			Log_This(Log_Msg,mpLOG_NORMAL);
			rc = mipSend(Run->socket, (char * ) Request998, 11);
			if (rc <= 0) 
			{
                                /* Ocorreu erro no envio */
                               	Log_This("Abort: error on mipSend()", mpLOG_NORMAL);
				Status = stDISCONNECT;
				break;
			}
			Status = stENDING;
			break;
		case stENDING: /* receive final 998 from MIP */		
			Len = DATA_BUFSIZE;
                        rc = mipRecv( Run->socket, (char *) &DataBuffer[0], &Len );
                        if ( fDebug )
                        {
                                sprintf(Log_Msg,"mipRecv returned %d, Len %d",rc,Len);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                                if ( (rc == 0) && (Len > 0) )
                                {
                                        Dump_Message(Run->fd_log,"Data from MIP ->", DataBuffer,Len );
                                }
                        }
                        if ( rc != 0 )
                        {
                                Status = stDISCONNECT;
                                Fail=1;
                                break;
                        }
	
			Status = stEND;
			break;
		} /* end switch */
	} while ( Status != stEND );
	/* Termina */
    /* Set_Term(0,0);  */
    if (fDebug)
    {
            Log_This("END END END END END ",mpLOG_NORMAL);
    }
    Log_This("*bye",mpLOG_NORMAL);
    if (Fail) return(-1);

    return(0);
}       /* end of mipSend_File() */


int checkMipResponse( unsigned char *Buffer, int *Type)
{
	
	int result;
	
	unsigned char ErrMsg[80];

	    /*
         *      Message types
         *
         *      004 = file header
         *      998 = info
         *      others = data, up to 1015 bytes
         *
         */
	switch ((int) Buffer[0] )
	{
	case  0xf0:	/* 004 ? */
		if (    ( Buffer[1] ==  0xf0  ) &&
		    	( Buffer[2] ==  0xf4 ))
		{
			*Type = -4;
			Log_This("Bulk File Header Received\n",mpLOG_NORMAL);
			Display_Header_004(Buffer);
		}
		break;
	case  0xf9:       /* 998 ? */
		/*
                 * 9980100 - no message         - Good Return Record
                 * 9980100ABCD                  - End of Data Record
                 * 9980101 - error message      - Error Record
                 */
		if (    ( Buffer[1] ==  0xf9 ) &&
		    ( Buffer[2] ==  0xf8  ))
		{
			if ( Buffer[6] == 0xf0 )
			{ 	/* 9980100XX: End of Data Record or
                 * Good Return Record
				 */
				if ( (result == 7) || (
					(Buffer[7] == 0) && 
					(Buffer[8] == 0) && 
					(Buffer[9] == 0) && 
					(Buffer[10] == 0) ) ) 
				{       /* 9980100 Good Data Record */
					*Type = T_GoodReturnRecord;
					Log_This("Good Return Record received\n",mpLOG_NORMAL);
				}
				else
				{
					*Type = T_EndOfDataRecord;
					Log_This("End Of Data Record received\n",mpLOG_NORMAL );
					Display_Trailer_998(Buffer);
				}
			}
			else
			{       /* 998010X... Error Record */
				*Type = T_ErrorRecord;
				conv_ascii( &ErrMsg[0], Buffer+32, 55 );
				sprintf(Log_Msg, "MIP: %s\n", ErrMsg );
				Log_This(Log_Msg,mpLOG_NORMAL);
			}
		}
		break;
	case 0xe3:               /* data buffer */
		*Type = result - 1;
		break;
	default:
		*Type = T_UndefinedData;
		Log_This("Undefined Data 0X%02X\n", Buffer[0] );
		break;
	} /* switch */
	return(*Type);
} 

int Display_Header_004(unsigned  char * Buffer )
{

	long BlockCount;
	char BulkFileName[15];
	memcpy( &BlockCount, Buffer+36, 4 );
	memcpy( &BulkFileName[0], Buffer+5, 14 );
	BulkFileName[14]=0;
	
	sprintf(Log_Msg, "MIP reported %ld blocks for bulk file %s\n", 
		BlockCount, BulkFileName );
	Log_This(Log_Msg, mpLOG_NORMAL);
	return 0;
}

int Display_Trailer_998( unsigned char * Buffer )
{
	long BlockCount;
	memcpy( &BlockCount, Buffer+7, 4 );
	sprintf(Log_Msg, "\tFinal total of %ld blocks transmitted\n",
		BlockCount - 1 );
	Log_This(Log_Msg, mpLOG_NORMAL);
	Log_This("Bulk File transfer is ending now\n",mpLOG_NORMAL);
	return 0;
}

int mipAdvisement_File(_Run * Run)
{
        int rc;
        unsigned char * THeader;
        unsigned char Buffer[DATA_BUFSIZE];
        int Len;
        int Status = stCONNECT;
        char lstr[DATA_BUFSIZE+1];
        int Fail=0;

        while(1) {
                memset(lstr,0,DATA_BUFSIZE+1);
                switch(Status)
                {
                case stCONNECT:
                        Run->socket = mipCONNECT(atoi(Run->Socket_Port),     Run->IpAddress);
                        if (Run->socket != -1 )
                        {
                                sprintf(Log_Msg,
                                        "CONNECTed to TCP channel \"%s\" IpAddress \"%s\" fd is \"%d\"",
                                        Run->Socket_Port,
                                        Run->IpAddress,
                                        Run->socket);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                        }
                        else
                        {
                                sprintf(Log_Msg,
                                        "CONNECT to TCP channel \"%s\" subnet \"%s\" failed, code is \"%d\"",
                                        Run->Socket_Port,
                                        Run->IpAddress,
                                        rc);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                                Status = stDISCONNECT;
                                break;
                        }
                        Status = stSTARTADVISEMENT;
                        break;
                case stSTARTADVISEMENT:
                        THeader = malloc(32);
                        memset(THeader,32,32);
                        memcpy(THeader,"20100",5);
                        *(THeader+5)=Run->Advisement_Filter;
                        conv_ebcdic( THeader, THeader, 32);
                        if ( fDebug )
                        {
                                Log_This("START ADVISEMENT",
                                         mpLOG_NORMAL);
                                Dump_Message(Run->fd_log,"File Advisement Solicit Message ->",
                                             THeader, 32);
                        }
                        rc =mipSend(Run->socket, (char *) THeader, 32);
                        if ( fDebug )
                        {
                                sprintf(Log_Msg,"mipSend() = %d",rc);
                                Log_This(Log_Msg, mpLOG_NORMAL);
                        }
                        free(THeader);
                        if (rc <=0)
                        {
                                Log_This("Abort: error on mipSend()", mpLOG_NORMAL);
                                Status = stDISCONNECT;
                                break;
                        }
                        Status = stINSIDEADVISEMENT;
                        break;
                case stINSIDEADVISEMENT:
                        if ( fDebug )
                        {
                                Log_This("INSIDE ADVISEMENT", mpLOG_NORMAL);
                        }
                        Len = DATA_BUFSIZE;
                        rc = mipRecv( Run->socket, (char *) Buffer, &Len );
                        if ( fDebug )
                        {
                                sprintf(Log_Msg,"mipRecv returned %d, Len %d",rc,Len);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                                if ( (rc==0) && (Len > 0) )
                                {
                                        Dump_Message(Run->fd_log,"Data from MIP ->",Buffer,Len );
                                }
                        }
                        if ( rc != 0 )
                        {
                                Status = stABORTADVISEMENT;
                                break;
                        }
                        if ( Len < 5 )
                        {
                                Log_This("Message possibly truncated. Aborting",
                                         mpLOG_NORMAL);
                                Status = stABORTADVISEMENT;
                                break;
                        }
                        /* ok: we have a response from MIP */
                        if ((*Buffer==0xf2)&&(*(Buffer+1)==0xf0)&&(*(Buffer+2)==0xf2))
                        {
                                rc = Process_Advisement_Response(Buffer,Len,Run);
                                if (rc)
                                {
                                        Status = stABORTADVISEMENT;
                                        break;
                                }
                                rc = Send_Data_Acknowledgment(Run);
                                if (rc <= 0)
                                {
                                        Status = stABORTADVISEMENT;
                                        break;
                                }
                                break;
                        }
                        if ((*Buffer==0xf2)&&(*(Buffer+1)==0xf9)&&(*(Buffer+2)==0xf8))
                        {
                                Status = stDISCONNECT;
                                break;
                        }
                        Log_This("MIP reported end of advisement",mpLOG_NORMAL);
                        Status = stDISCONNECT;
                        break;
                case stDISCONNECT:
                        if ( fDebug )
                        {
                                Log_This("DISCONNECT DISCONNECT DISCONNECT DISCONNECT DISCONNECT ",mpLOG_NORMAL);
                        }
                        sprintf(Log_Msg,"%d Bytes Received in %d Blocks",
                                Run->Bytes_Received, Run->Block_Number);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        mipDISCONNECT(Run->socket);
                        sprintf(Log_Msg,"TCP DISCONNECT (%d)",rc);
                        Log_This(Log_Msg,mpLOG_NORMAL);
                        Status = stEND;
                        break;
                case stEND:
                        if (fDebug)
                        {
                                Log_This("END END END END END ",mpLOG_NORMAL);
                        }
                        if(Fail) return(-1);
                        return(0);
                default:
                        if (fDebug)
                        {
                                Log_This("Advisement ERROR",mpLOG_NORMAL);
                        }
                        return(-1);
                }       /* end switch */

        }
		return 0;
}       /* end of mipAdvisement_File() */
