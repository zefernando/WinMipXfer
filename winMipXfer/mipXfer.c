/*
*       @(#)
*       @(#) MIP  TCP/IP interface
*       @(#) Modulo mipXfer.c 1.0 09 Oct 2004
*       @(#) JFAB 2004 José Fernando Alvim Borges
*       @(#) ultima alteracao:
*       @(#)
*
*/
#include		<stdio.h>
#include		<stdlib.h>
#include		<fcntl.h>
#include		<io.h>
#include		<conio.h>
#include		<process.h>
#include		<sys/types.h>
#include		<sys/stat.h>
#include		<string.h>
#include		<signal.h>


#include        <time.h>

static  char _SCCSid0_[] =      "@(#) Versao MFT 1.0.0 2004 JFAB";
static  char _SCCSid1_[] =      "@(#) mipXfer.c 1.0 09-10-2004";

#include "mipXfer.h"

int fDebug;
FILE * fTraceFile;
struct _Run Run;
int Status;
char * Log_Msg;
char Aux_Msg[256];

/*
 * prototypes
 */
int mipParm_Build(char *,_mipParm_List * *);
int mipParm_Scan(_mipParm_List * *);
char * mipParm_Value(char *,_mipParm_List *);
int mipFile_OK(char *);
int Log_This(char *,char);
void How_to_INT(void);


/* int Set_Term(int,int); */

void conv_ebcdic (
        unsigned char * destino,
        unsigned char * origem,
        unsigned int tamanho
);

int mipReceive_File(_Run *);
int mipSend_File(_Run *);
int mipAdvisement_File(_Run *);
int EchoParms(void);
void xferStat();
extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char **argv, const char *options);


/*
 * prototypes END
 */

/* EchoParms() */
int EchoParms(void)
{
        fprintf(stderr, "\n%s\n%s\n\n", _SCCSid0_, _SCCSid1_ );
        return(0);
}

void How_to_INT(void)
{
        printf("\n\t------------------------------------------------------\n");
        printf("\tTo end transfer, press INTR twice in less than 3 seconds\n"
              );
        printf("\t------------------------------------------------------\n");
} /* end How_to_INT() */

void handleSIGINT (int signo)
{
        char msg[]="*Transfer INTERRUPTED by USER";
        /* Set_Term(0,0); */
        Log_This(msg, mpLOG_NORMAL);
        Run.Err_Msg=malloc(1+strlen(msg));
        strcpy(Run.Err_Msg,msg);
        Run.iTimeout = 1;
        (void)signal(SIGINT,handleSIGINT);
}

/* Mostra as estatisticas de uso */
void xferStat()
{
        static time_t now;
        static time_t last;
        float   work;
        long    Elapsed;
        long    BytesSoFar = 0;

        if ( Run.intCount == 0 )
        {
                time(&last);
                Run.intCount++;
                if ( Run.Block_Number > 1 )
                {
                        time(&now);
                        Elapsed = now-Run.StartT;
                        printf( "\nTransfer time: %lds\n", Elapsed );
                        work = Run.Block_Number / (float) Run.Block_Count * 100.;
                        printf(
                                "Blocks Transferred: %d out of %d (%4.2f%%) [Block Length %d]\n",
                                Run.Block_Number, Run.Block_Count, work , Run.Block_Len );
                        printf( "Bytes Transferred: %ld\n", BytesSoFar );
                        if ( Elapsed )
                        {
                                work = Run.Block_Number/(float) Elapsed;
                                printf( "Transferring %4.2f blocks/s (%dcps)\n", work,
                                        (int) (work * Run.Block_Len) );
                                work = (Run.Block_Count - Run.Block_Number) / work;
                                printf( "Estimated time to end %4.2fs [%d:%02d]\n",
                                        work , (int) work / 60, (int) work % 60 );
                        }
                }
          }
        else
        {
                time(&now);
                /* BEGIN MFT.1.1.0.0 */
                /*******
                if( (now-last) < 3 )
        {
                        Log_This("*Transfer aborted by user",mpLOG_NORMAL);
                        (void)signal(SIGINT,SIG_IGN);
                        Run.bTimeout = 3;
                        return;
        }
                *******/
                /* END MTF.1.1.0.0 */
                Elapsed = now-Run.StartT;
                /* Tempo decorrido na transferencia */
                printf( "\nTransmit time: %lds\n", Elapsed );
                work = Run.Block_Number / (float) Run.Block_Count * 100.;
                /* Numero de blocos transferidos */
                printf(
                        "Blocks Transferred: %d out of %d (%4.2f%%) [Block Length %d]\n",
                        Run.Block_Number, Run.Block_Count, work , Run.Block_Len );
                /* Quantidade de bytes transferido */
                printf( "Bytes Transferred: %ld\n", BytesSoFar );
                if ( Elapsed )
                {
                        work = Run.Block_Number/(float) Elapsed;
                        /* Taxa de Transferencia em blocos por segundo */
                        printf( "Transferring %4.2f blocks/s (%dcps)\n", work,
                                (int) (work * Run.Block_Len) );
                        work = (Run.Block_Count - Run.Block_Number) / work;
                        /* Tempo estimado para terminar */
                        printf( "Estimated time to end %4.2fs [%ld:%02d]\n",
                                work , (int) work / 60, (int) work % 60 );
                }
                Run.intCount++;
                last = now;
        }
        
}       /* end of xferStat() */

/* Mostra o uso do Programa */
/* usage() */
int usage (char * prog)
{
        fprintf(stderr,"\nUse: \" %s\t\\\n", prog);
        fprintf(stderr,"\t\t [-d TraceLevel ] \\\n");
        fprintf(stderr,"\t\t [-j Julian Date]\\\n");
        fprintf(stderr,"\t\t -t FileType \\\n");
        fprintf(stderr,"\t\t [-v ] [LocalFile] \"\n");
	 fprintf(stderr,"\t\t [-b ] [Block Size] \"\n");
        fprintf(stderr,"\twhere\n");
        fprintf(stderr,"\t\t -v echoes parameters to stdout\n\n");
        fprintf(stderr,"\t\t \"LocalFile\" is the data file to be created\n");
        fprintf(stderr,"\t\t \"JulianDay\" is the Julian day of the file\n");
        fprintf(stderr,"\t\t \"FileType\" is the Bulk File #\n");
        fprintf(stderr,
                "\t\t \"TraceLevel\" is a bit-mask of debug level info\n\n");
        exit(1);
}


int MIP_Session(_Run * Run)
{
        int rc;
        char Bulk[5];
        char * param;
        if ( fDebug )
        {
                /* Entra em modo Debug */
                Log_This("MIP Session",mpLOG_NORMAL);
                strcpy(Log_Msg,
                       "*Bulk File BBBB EndPoint EEEEE Julian date is DDD");
                /* Log do tipo de arquivo */
                memcpy(Log_Msg+11, Run->Bulk_Type, 4);
                /* Log do End Point */
                memcpy(Log_Msg+25, Run->Endpoint, 5);
                /* Log da data Juliana */
                memcpy(Log_Msg+46, Run->Julian_Date, 3);
                Log_This(Log_Msg,mpLOG_NORMAL);
        }       /* end if */
        sprintf(Log_Msg,"Bulk Type is %c", Run->Bulk_Type[0]);
        Log_This(Log_Msg,mpLOG_NORMAL);
        Run->fd_out = -1;
        switch(Run->Bulk_Type[0])
        {
        case 'T':
        case 't':
                /* Recebe arquivo do MIP */
                /* Inicializa o file descriptor do arquivo local */
                rc = mipReceive_File(Run);
                break;
        case 'R':
        case 'r':
                /* Envia arquivo para o MIP */
	       Run->sFileName = (char *) malloc(10);
                memset(Run->sFileName, 0 , 10);
	       memcpy(Run->sFileName, Run->Bulk_Type, 4);
	       memcpy(Run->sFileName + 4, Run->Julian_Date, 3);
	       memcpy(Run->sFileName + 7, Run->Sequence_Number, 2);
                rc = mipSend_File(Run);
                break;
        default:
                if ( Run->Advisement_Filter )
                {
                        sprintf(Log_Msg,"Advisement Filter set to %c",
                                Run->Advisement_Filter);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                }
                else
                {
                        Log_This("NO Filter set for Bulk Advisement Routine",
                                 mpLOG_NORMAL);
                }
                rc = mipAdvisement_File(Run);
                break;
        }       /* end switch */
        /* Mostra no log o resultado da execucao */
        sprintf(Aux_Msg,"Completion Code for run is %d",rc); 
        Log_This(Aux_Msg, mpLOG_NORMAL);  
	/* printf(Log_Msg); */
        if ((Run->Bulk_Type[0] == 'A')||(Run->Bulk_Type[0] == 'a'))
        {
                if(rc == 0)
                {
                        return(0);
                }
                else
                {
                        return(-1);
                }
        }
        if(rc == 0)
        {
                /* A transferencia foi OK */
                printf("OK");
                param= mipParm_Value("OnSuccess", Run->Parm_List);
                if( param!=NULL)
                {
                        /* Executa um script para processar o arquivo transferido */
                        memcpy(Bulk,Run->Bulk_Type,4);
                        Bulk[4]=0;
                        /*execlp(
                                (const char *)param,(const char *)param,
                                (const char *)Bulk,
                                (const char *) "Bulk File Transfer OK", NULL);
								
                        perror("execlp"); */
                }
                return(0);
        }
        else
        {
                /* A Transferencia falhou */
                        /* Executa o script de falha */
                if(Run->fd_out >= 0 ) {
                        _close(Run->fd_out);
                        _unlink(Run->Output_File);
                        sprintf(Log_Msg,"Output file \"%s\" DELETED", Run->Output_File);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                }
                param= mipParm_Value("OnFailure", Run->Parm_List);
                if( param!=NULL)
                {
                        memcpy(Bulk,Run->Bulk_Type,4);
                        Bulk[4]=0;
                       // execlp( (const char *)param,(const char *)param,  (const char *)Bulk, (const char *)  Run->Err_Msg, NULL);
                       // perror("execlp");
                }
                return(-1);
        }
}       /* end MIP_Session() */





int main(int argc, char *argv[])
{

        int option;
        int rc;
        int erp;

        char * param;


        struct _stat stats;


        /* Abre o terminal */
        Run.fd_in = _open("/dev/tty",_O_RDWR);
        /* Inicia o handler de tratamento de interrupcoes */
        (void)signal(SIGINT,handleSIGINT);
        /* Ecoa os parametros de chamada */
        EchoParms();
        memset((void *)&Run,0,sizeof(struct _Run));
        Log_Msg = (char *) malloc(256);
        fDebug=0;
        Run.Parm_List = NULL;
        /* Le o arquivo de configuracao */
        mipParm_Build("mipXfer.cfg",&Run.Parm_List);
        /*------------------------------------------------------------*/
        /* TRACE FILE NAME */
        param= mipParm_Value("LogFileName",Run.Parm_List);
        if( param == NULL )
        {
                Run.Trace_File_Name = malloc(12);
                memcpy(Run.Trace_File_Name,"mipXfer.log",12);
        }
        else
        {
                Run.Trace_File_Name = malloc(strlen(param)+1);
                memcpy(Run.Trace_File_Name,param,strlen(param)+1);
        }
        /*------------------------------------------------------------*/
        /* MAX LOG FILE SIZE */
        /* Define o Tamanho maximo do Log */
        param= mipParm_Value("MaxLogSize",Run.Parm_List);
        if( param == NULL )
        {
                Run.Max_Log_Size = 0;
        }
        else
        {
                Run.Max_Log_Size = atoi(param);
        }
        /* Inicia a estrutura de parametros da sessao */
        Run.socket = -1;
        Run.Debug_Level = 0; erp=0;
        Run.bBulk_Type = Run.bJulian_Date = Run.bSequence_Number = 0;
        Run.Advisement_Filter = 0;
        Run.verbose = 0;
        Run.Block_Number = 0;
        Run.Block_Count = 0;
        Run.iPhase = 0;
        Run.Err_Msg=0;
        Run.bASCII_Output=0;
        Run.bForce=0;
        Run.bDebug=0;
	 Run.Block_Len = 1012;

        rc =  Log_This(" ****** session             ******",mpLOG_OPEN);
        if (rc != 0)
        {
                fprintf(stderr,"%s: unable to open LOG File [%s]",
                        argv[0], Run.Trace_File_Name);
                exit(-1);
        }
        Log_This(" ******         starts      ******",mpLOG_NORMAL);
        Log_This(" ******                here ******",mpLOG_NORMAL);
        while ((option = getopt(argc, argv, OPTSTRING)) != EOF)
        {
                switch (option)
                {
                case 't':
                case 'T':
                        /* Define o tipo de arquivo */
                        sprintf(Log_Msg,"Bulk File Type is: [%s]", optarg);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        if(strlen(optarg)!=4)
                        {
                                if((optarg[0]=='A')||(optarg[0]=='a'))
                                {
                                        /* O tipo eh de arquivo de advises */
                                        Log_This("File Advisement Set",mpLOG_NORMAL);
                                        memcpy(Run.Bulk_Type,"A   ",4);
                                        if(strlen(optarg)>1)
                                        {
                                                Run.Advisement_Filter=optarg[1];
                                                sprintf(Log_Msg,"Advisement Filter set to %c",
                                                        optarg[1]);
                                                Log_This(Log_Msg,mpLOG_NORMAL);
                                        }
                                        Run.bBulk_Type = 1;
                                        break;
                                }
                                else
                                {
                                        fprintf(stderr,
                                                "%s: Bulk File Type \"%s\" is not in the form \"[TA]nnn\"\n",
                                                argv[0], optarg );
                                        ++erp;
                                        break;
                                }
                        }
                        Run.bBulk_Type = 1;
                        memcpy(Run.Bulk_Type,optarg,4);
                        break;
                case 'j':       /* julian day */
                case 'J':       /* julian day */
                        sprintf(Log_Msg,"Julian Date is: [%s]", optarg);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        if(strlen(optarg)!=3)
                        {
                                fprintf(stderr,
                                        "%s: Julian Date \"%s\" is not in the form \"nnn\"\n",
                                        argv[0], optarg );
                                ++erp;
                                break;
                        }
                        Run.bJulian_Date = 1;
                        memcpy(Run.Julian_Date,optarg,3);
                        break;
                case 'd':       /* debug level */
                case 'D':       /* debug level */
                        Run.Debug_Level = optarg[0]-48;
                        fDebug=1;
                        break;
                case 'v':
                case 'V':
                        /* Liga o modo de verbose */
                        Run.verbose = 1;
                        break;
                case 's':
                case 'S':
                        /* Sequence Number */
                        sprintf(Log_Msg,"Sequence Number is: [%s]", optarg);
                        Log_This( Log_Msg, mpLOG_NORMAL );
                        if(strlen(optarg)!=2)
                        {
                                fprintf(stderr,
                                        "%s: Sequence Number \"%s\" is not in the form \"nn\"\n",
                                        argv[0], optarg );
                                ++erp;
                                break;
                        }
                        Run.bSequence_Number = 1;
                        memcpy(Run.Sequence_Number,optarg,2);
                        break;
                case 'a':
                case 'A':
                        /* Indica se o mipXfer farah conversao para ASCII */
                        Run.bASCII_Output=1;
                        break;
		   case 'b':
                case 'B':
                        /* Indica  o tamanho do blocoI */
			    sprintf(Log_Msg,"Block Size is: [%s]", optarg);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        Run.Block_Len=atoi(optarg);
                        break;

                case 'r':
                case 'R':
                        Run.bForce=1;
                        break;
                }
        } /* end while */
        if ( argv[optind]==0 )
        {
                Run.bOutput = 0;
                Log_This("Output File not in command line. OK",
                         mpLOG_NORMAL);
        }
        else
        {
                /* Nome do arquivo estah na linha de comando */
                Run.bOutput = 1;
                Run.Output_File = malloc(strlen(argv[optind])+1);
                strcpy(Run.Output_File,argv[optind]);
                sprintf(Log_Msg,"Output File is \"%s\"",Run.Output_File);
                Log_This(Log_Msg,mpLOG_NORMAL);
        }
        if ( erp )
        {
                /* Os parametros estao errados. O programa termina */
                fprintf( stderr,
                         "%s: %d errors (s) in parameter (s). Aborting...\n",
                         argv[0], erp);
                exit(1);
        }
        /*------------------------------------------------------------ */
        /* Bulk File */
        if (!Run.bBulk_Type)
        {
                /* Tipo de arquivo naum foi especificado na linha de comando */
                /* Le do Arquivo de configuracao */
                param= mipParm_Value("Bulk_File",Run.Parm_List);
                if ( param== NULL )
                {
                        strcpy(Log_Msg,"Missing Bulk File Number");
                        Log_This(Log_Msg,mpLOG_NORMAL);
                        exit(1);
                }
                if(strlen(param)>4)param[4]=0;
                memcpy(Run.Bulk_Type,param,strlen(param));
                switch(param[0])
                {
                case 'a':
                case 'A':
                        Log_This("File Advisement Set",mpLOG_NORMAL);
                        Run.Bulk_Type[0]='A';

                        if(strlen(param)>1)
                        {
                                /* O arquivo de advises tem filtro */
                                Run.Advisement_Filter=param[1];
                                sprintf(Log_Msg,"Advisement Filter set to %c",param[1]);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                        }
                        Run.bBulk_Type = 1;
                        break;
                case 't':
                case 'T':
		   case 'r':
		   case 'R':
                        /* Arquivo padrao */
                        Log_This("STANDARD Bulk File Transfer SET",mpLOG_NORMAL);
                        sprintf(Log_Msg,"Bulk File is %c%c%c%c",
                                Run.Bulk_Type[0], Run.Bulk_Type[1],
                                Run.Bulk_Type[2], Run.Bulk_Type[3]);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        
                break;

                default:
                        strcpy(Log_Msg,"INVALID Bulk File Number");
                        Log_This(Log_Msg,mpLOG_NORMAL);
                        exit(-1);
                }       /* end switch */
                Run.bBulk_Type = 1;
        }
        /*------------------------------------------------------------*/
        /* Julian Date */
        if ( ! Run.bJulian_Date )
        {
                /* Le do arquivo de configuracao */
                param= mipParm_Value("Julian_Date",Run.Parm_List);
                if ( param != NULL )
                {
                        memcpy(Run.Julian_Date,param,3);
                        sprintf(Log_Msg,"Julian Date is %c%c%c",
                                Run.Julian_Date[0], Run.Julian_Date[1],
                                Run.Julian_Date[2]);
                        Run.bJulian_Date = 1;
                }
		else {
                        memcpy(Run.Julian_Date,"   ",3);
			sprintf(Log_Msg,"Generic Julian Date");

		}
        }
        /*------------------------------------------------------------*/
        /* Sequence Number */
        if ( ! Run.bSequence_Number )
        {
                /* O Sequence Number naum estah na linha de comando */
                /* Le do arquivo de configuracao */
                param= mipParm_Value("Sequence_Number",Run.Parm_List);
                if ( param == NULL )
                {
                        strcpy(Log_Msg,"Sequence Number not specified");
		     memcpy(Run.Sequence_Number,"  ",2);
                        Log_This(Log_Msg,mpLOG_NORMAL);
                }
	      else {
             	   memcpy(Run.Sequence_Number,param,2);
	      }
                Run.bSequence_Number = 1;
        }
        /*------------------------------------------------------------*/
        /* MIP END POINT */
        param= mipParm_Value("Endpoint",Run.Parm_List);
        if ( param==NULL )
        {
                strcpy(Log_Msg,"Endpoint not specified");
                Log_This(Log_Msg,mpLOG_NORMAL);
                /* Trocar pelo ENDPOINT DO BANCOB */
                sprintf(Log_Msg,"Endpoint Number is set to \"01940\"");
                Log_This(Log_Msg,mpLOG_NORMAL);
                /* Trocar pelo ENDPOINT DO BANCOB */
                memcpy(Run.Endpoint,"01940",5);
        }
        memcpy(Run.Endpoint,param,5);
        /*------------------------------------------------------------*/
        /* ICA NUMBER */
        param= mipParm_Value("ICA",Run.Parm_List);
        if ( param==NULL )
        {
                /* O ICA do BANCO naum estah no arquivode configuracao */
                strcpy(Log_Msg,"ICA number not specified");
                Log_This(Log_Msg,mpLOG_NORMAL);
                sprintf(Log_Msg,"ICA Number is set to \"7097\"");
                Log_This(Log_Msg,mpLOG_NORMAL);
                /* TROCAR PELO ICA Do BANCO */
                memcpy(Run.Endpoint,"7097",4);
        }
        /*------------------------------------------------------------*/
        /* OUTPUT FILE */
        if (!Run.bOutput)
        {
                /* Arquivo Local naum foi especificado na linha de comando */
                /* Le o nome do arquivo do arquivo de configuracao */
                param= mipParm_Value("Output",Run.Parm_List);
                if ( param == NULL )
                {
                        strcpy(Log_Msg,"output file not specified");
                        Log_This(Log_Msg,mpLOG_NORMAL);
                        if (
                                (Run.bBulk_Type==0) ||
                                ((Run.bBulk_Type==1)&&(Run.Bulk_Type[0]=='A'))
                        )
                        {
                                /* Arquivo eh de advise */
                                Run.Output_File = malloc(15);
                                strcpy(Run.Output_File,"mipAdvises.txt");
                                sprintf(Log_Msg,"output file set to %s", Run.Output_File);
                                Log_This(Log_Msg,mpLOG_NORMAL);
                        }
                        else
                        {
                                /*
                                *  Obtem o nome do arquivo no Header
                                *  enviado pelo MIP 
                                */

                                Run.Output_File = malloc(15);
                                memset((void *)Run.Output_File,32,14);
                                Run.Output_File[14]=0;
                                /*
                                * signs to change file name
                                * on receiving the file header
                                * from MIP
                                */
                                Run.bOutput=2;          
                                sprintf(Log_Msg,"output file will be generated");
                                Log_This(Log_Msg,mpLOG_NORMAL);
                        }
                }
                else
                {
                        /* Obtem o nome do arquivo local da linha de comando */
                        Run.Output_File = malloc(strlen(param)+1);
                        strcpy(Run.Output_File,param);
                        sprintf(Log_Msg,
                                "*Output file name will be \"%s\"",
                                Run.Output_File);
                        Log_This(Log_Msg,mpLOG_NORMAL);
                }
        }
        /* Obtem o endereco de IP do MIP */
        param= mipParm_Value("IpAddress",Run.Parm_List);
        if ( param == NULL )
        {
                /* Naum ha Ip no arquivo de configuracao */
                strcpy(Log_Msg,"IpAddress ADDRESS not specified. Aborting.");
                Log_This(Log_Msg,mpLOG_NORMAL);
                return(1);
        }
        Run.IpAddress = malloc(strlen(param)+1);
        strcpy(Run.IpAddress,param);
        sprintf(Log_Msg,"IpAddress id is \"%s\"",Run.IpAddress);
        Log_This(Log_Msg,mpLOG_NORMAL);
        /* Obtem a porta de sockets do MIP */
        param= mipParm_Value("Socket_Port",Run.Parm_List);
        if ( param == NULL )
        {
                /* A porta de sockets naum foi especificada no arquivo de configuracao */
                strcpy(Log_Msg,"Socket Port not specified. Aborting.");
                Log_This(Log_Msg,mpLOG_NORMAL);
                return(1);
        }
        Run.Socket_Port = malloc(strlen(param)+1);
        strcpy(Run.Socket_Port,param);
        sprintf(Log_Msg,"Socket Port is \"%s\"",Run.Socket_Port);
        /*------------------------------------------------------------*/
        /* Fail Rate */
        param= mipParm_Value("Fail_Rate",Run.Parm_List);
        if ( param!= NULL )
        {
                Run.Fail_Rate=atoi(param);
        }
        else
        {
                Run.Fail_Rate = 0;
        }
        /*------------------------------------------------------------ */
        /* BLOCK SIZE */
        param= mipParm_Value("BLKS",Run.Parm_List);
        if ( param != NULL )
        {
                Run.Block_Size=atoi(param);
        }
        else
        {
                Run.Block_Size = 931;
        }
        if(fDebug)
        {
                sprintf(Log_Msg,
                        "Block Size is set to %d (FOR TEST PURPOSES ONLY)",
                        Run.Block_Size);
                Log_This(Log_Msg, mpLOG_NORMAL );
        }
        /*------------------------------------------------------------*/
        /* Test Mode _? */
        param= mipParm_Value("Test_File",Run.Parm_List);
        if ( param != NULL )
        {
                Log_This("--------------- Test Mode ---------------",
                         mpLOG_NORMAL );
                sprintf(Log_Msg,"Test File is [%s]",param);
                Log_This(Log_Msg, mpLOG_NORMAL );
                if (!(rc = _stat(param,&stats)))
                {
                        sprintf(Log_Msg,"File Size is %d", stats.st_size);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        Run.Block_Count = stats.st_size /Run.Block_Size;
                        if((Run.Block_Count%Run.Block_Size)!=0)
                        {
                                Run.Block_Count++;
                        }
                        sprintf(Log_Msg,"Block Count is set to %d",
                                Run.Block_Count);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        Run.bTest_Mode = 1;
                        Run.fd_Temp = _open(param,_O_RDONLY);
                        if (Run.fd_Temp < 0) return(-1);
                        sprintf(Log_Msg,"Test File is open, fd=%d", Run.fd_Temp);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                }
                else
                {
                        Log_This("Test File NOT FOUND", mpLOG_NORMAL);
                        Run.bTest_Mode = 0;
                }
        }
        /*------------------------------------------------------------*/
        /* FORCE */
        if ( ! Run.bForce )
        {
                param= mipParm_Value("Recreate_File",Run.Parm_List);
                if ( param!= NULL )
                {
                        if ( (param[0]=='Y')||(param[0]=='y'))
                        {
                                Run.bForce = 1;
                        }
                }
        }
        /*------------------------------------------------------------ */
        /* DEBUG */
        if ( ! Run.Debug_Level )
        {
                param= mipParm_Value("Debug_Level",Run.Parm_List);
                if ( param!= NULL )
                {
                        if (param[0]!='0')
                        {
                                Run.Debug_Level = 1;
                                fDebug = 1;
                        }
                }
        }
        /*------------------------------------------------------------*/
        /* SESSION ADVISEMENT TEST */
        param= mipParm_Value("Session",Run.Parm_List);
        if ( param!= NULL )
        {
                memcpy(Run.Advisement_Session,param,6);
                if(fDebug)
                {
                        sprintf(Log_Msg,"%s","Advisement session id is BBBBBB");
                        memcpy(Log_Msg+25,Run.Advisement_Session,6);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                }
        }
        /*------------------------------------------------------------ */
        /* STATs frequency */
        param= mipParm_Value("ReportFrequency",Run.Parm_List);
        if ( param!= NULL )
        {
                Run.bReport_Frequency = atoi(param);
                sprintf(Log_Msg,
                        "Transfer status REPORT issued at each %d Blocks",
                        Run.bReport_Frequency);
                Log_This(Log_Msg,mpLOG_NORMAL);
        }
        else
        {
                Run.bReport_Frequency = 0;
        }
        /*------------------------------------------------------------*/
        /* TIMEOUT */
        param= mipParm_Value("Time",Run.Parm_List);
        if ( param != NULL )
        {
                Run.bTimeout = atoi(param);
        }
        else
        {
                Run.bTimeout = 0;
        }
        /*------------------------------------------------------------ */
        /* CONVERT OUTPUT TO ASCII ? */
        if ( ! Run.bASCII_Output)
        {
                param= mipParm_Value("ConvertToASCII",Run.Parm_List);
                if ( param!= NULL )
                {
                        switch(param[0])
                        {
                        case 'Y':
                        case 'y':
                        case 'S':
                        case 's':
                                Run.bASCII_Output = 1;
                                Log_This("OUTPUT will be converted to ASCII",
                                         mpLOG_NORMAL);
                                break;
                        default:
                                break;
                        }       /* end switch */
                }
        }
        /*------------------------------------------------------------*/
        /* OnFailure */
        param= mipParm_Value("OnFailure",Run.Parm_List);
        if ( param != NULL )
        {
                Run.Fail_Script = malloc(strlen(param)+1);
                strcpy(Run.Fail_Script,param);
                sprintf(Log_Msg,"On reception failure \"%s\" will run",
                        param);
                Log_This(Log_Msg,mpLOG_NORMAL);
        }
        else
        {
                Run.Fail_Script = 0;
        }
        /*------------------------------------------------------------*/
        /* OnSucess */
        param= mipParm_Value("OnSuccess",Run.Parm_List);
        if ( param!= NULL )
        {
                Run.Ok_Script = malloc(strlen(param)+1);
                strcpy(Run.Ok_Script,param);
                sprintf(Log_Msg,"On successfull reception \"%s\" will run",
                        param);
                Log_This(Log_Msg,mpLOG_NORMAL);
        }
        else
        {
                Run.Fail_Script = 0;
        }
        /* */
        if(Run.Max_Log_Size)
        {
                sprintf(Log_Msg,"Max LOG file SIZE set to %ld",Run.Max_Log_Size);
                Log_This(Log_Msg,mpLOG_NORMAL);
        }
        /* now do it */
        Run.intCount=0;
        rc = MIP_Session(&Run);
        /* Set_Term(0,0);  */
        /* sprintf(Log_Msg,"Job ended: status %d",rc); */
        /* Log_This(Log_Msg, mpLOG_NORMAL); */
	/* printf(Log_Msg); */
        exit(0);
}
/* end of main() */
