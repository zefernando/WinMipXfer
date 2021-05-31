/*
*       MIP File Transfer 
 *      @(#) Modulo mipUtil.c 1.0 14 Sep 1999 16:58:10 
*       @(#) JFAB 2004 Jose Fernando Alvim Borges
*       mipUtil.c versao 1.0
*       ultima alteracao:
*
*/


#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "mipXfer.h"

extern int fDebug;
extern FILE * fTraceFile;
extern struct _Run Run;
extern int Status;
extern char Log_Msg[256];
extern int mipFile_OK(char *);

int mipParm_Build(char * cfgFile, _mipParm_List ** parm)
{
        int fd;
        unsigned char ch;
        int ix=0;
        unsigned char buffer[mpMAX_LINE_SIZE];
        unsigned char * l = buffer;
        unsigned char state;
        _mipParm_List * p;
        _mipParm_List * np;
        int n=0;

        if(*parm!=NULL)
        {
                return(-1);
        }
        p = malloc(sizeof(_mipParm_List));
        *parm=p;
        p->N = p->V =p->nx = NULL;
        if(mipFile_OK(cfgFile))
        {
                return(-2);
        }
        fd = _open(cfgFile,_O_RDONLY);
        if(fd < 0)
        {
                return(-3);
        }
        state='n';
        while(_read(fd,&ch,1)>0)
        {
                switch(ch)
                {
                case 13:
                        break;
                case 10:
                        if(state=='v')
                        {
                                p->V=malloc(ix+1);
                                memcpy(p->V,l,ix);
                                *(p->V+ix)=0;
                                state='n';
                        }
                        else
                        {
                                p->N=malloc(ix+1);
                                memcpy(p->N,l,ix);
                                *(p->N+ix)=0;
                                p->V=malloc(1);
                                *(p->V)=0;
                                state='n';
                        }
                        memset(l,0,mpMAX_LINE_SIZE);
                        ix=0;
                        np = malloc(sizeof(_mipParm_List));
                        np->N = np->V =np->nx = NULL;
                        p->nx=np;
                        p=np;
                        break;
                case 61:
                        if(state=='n')
                        {
                                p->N=malloc(ix+1);
                                memcpy(p->N,l,ix);
                                *(p->N+ix)=0;
                                memset(l,0,mpMAX_LINE_SIZE);
                                ix=0;
                                state='v';
                        }
                        else
                        {
                                l[ix++]=ch;
                        }
                        break;
                default:
                        if (state=='n')
                        {
                                l[ix++]=(islower(ch))?ch-32:ch;
                        }
                        else
                        {
                                l[ix++]=ch;
                        }
                        break;
                }       /* end switch */
        }       /* end while */
        if(state=='v')
        {
                p->V=malloc(ix+1);
                memcpy(p->V,l,ix);
                *(p->V+ix)=0;
        }
        _close(fd);
        return(1);
}


int mipParm_Scan(_mipParm_List * * parm)
{
        int n;
        _mipParm_List * p;

        if(parm==NULL)
        {
                if(fDebug)
                {
                        fprintf(fTraceFile,
                                "mipParm_Scan: Parameter list is empty\n");
                        fflush(fTraceFile);
                }
                return(0);
        }
        p=*parm;n=0;
        while(p->N!=NULL)
        {
                if(fDebug)
                {
                        fprintf(fTraceFile,"Scan:%s=%s\n",p->N,p->V);
                        fflush(fTraceFile);
                }
                p=p->nx; n++;
        }
        if(fDebug)
        {
                fprintf(fTraceFile,"mipParm_Scan: Total of %d parms\n", n);
                fflush(fTraceFile);
        }
        return(1);
}


char * mipParm_Value(char * Name, _mipParm_List * parm)
{
        _mipParm_List * lp;

        lp=parm;
        while(lp!=NULL)
        {
                if(lp->N != NULL)
                {
                        if(!_stricmp(Name,lp->N))
                        {
                                return(lp->V);
                        }
                }
                lp=lp->nx;
        }
        return(NULL);
}


int mipFile_OK(char * cfgFile)
{
        struct _stat     stats;
        int                     st;
        int                     r;

        r = _stat(cfgFile, &stats );
        if (r < 0)
        {
                st = 1;
        }
        else
        {
                if(!stats.st_size)
                {
                        st = 2;
                }
                else
                {
                        r = _open(cfgFile,_O_RDONLY);
                        if(r<0)
                        {
                                st = 3;
                        }
                        else
                        {
                                st = 0;
                                _close(r);
                        }
                }
        }
        if ( fDebug )
        {
                fprintf(fTraceFile, "mipFile_OK(%s) = %d\n",cfgFile,st);
                fflush(fTraceFile);
        }
        return(st);
}


int mipParm_Free(_mipParm_List * *parm)
{
        _mipParm_List * np;
        if(fDebug)
        {
                fprintf(fTraceFile,"mipParm_Free\n");
                fflush(fTraceFile);
        }
        while(*parm != NULL)
        {
                np=(*parm)->nx;
                free(*parm);
                *parm=np;
        }
        return(0);
}


int Log_This(char * msg , char code)
{
        time_t          h;
        struct tm       tm_s;
        struct _stat File_Info;
        int rc;
        char * Old;
        switch (code)
        {
        case mpLOG_OPEN:
                rc = _stat(Run.Trace_File_Name,&File_Info);
                if((rc==0)&&(Run.Max_Log_Size))
                {
                        if(File_Info.st_size>Run.Max_Log_Size)
                        {
                                Old = malloc(strlen(Run.Trace_File_Name)+6);
                                strcpy(Old,Run.Trace_File_Name);
                                strcat(Old,".back");
                                rc = rename(Run.Trace_File_Name,Old);
                                free(Old);
                        }
                }
                Run.fd_log = fopen(Run.Trace_File_Name,"a");
                if (Run.fd_log == NULL )
                {
                        sprintf(Log_Msg,"*Could not create or append to %s",
                                Run.Trace_File_Name);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        sprintf(Log_Msg,"*system reported \"%s\"",
                                strerror(errno));
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        return(-1);
                }
                fclose(Run.fd_log);
                rc = _chmod(Run.Trace_File_Name, 00777);
                if(rc<0)
                {
                        sprintf(Log_Msg,"*(LOG) chmod returned (%d)", rc);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        sprintf(Log_Msg,"*system reported \"%s\"",
                                strerror(errno));
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        return(-1);
                }
                Run.fd_log = fopen(Run.Trace_File_Name,"a");
                break;
        case mpLOG_RESET:
                Run.fd_log = fopen(Run.Trace_File_Name,"w");
                if (Run.fd_log == NULL )
                {
                        sprintf(Log_Msg,"*Could not create %s",
                                Run.Trace_File_Name);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        sprintf(Log_Msg,"*system reported \"%s\"",
                                strerror(errno));
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        return(-1);
                }
                fclose(Run.fd_log);
                rc = _chmod(Run.Trace_File_Name, 00777);
                if(rc<0)
                {
                        sprintf(Log_Msg,"*(LOG) chmod returned (%d)", rc);
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        sprintf(Log_Msg,"*system reported \"%s\"",
                                strerror(errno));
                        Log_This(Log_Msg, mpLOG_NORMAL);
                        return(-1);
                }
                Run.fd_log = fopen(Run.Trace_File_Name,"a");
                break;
        default:
                break;
        }
        if ( Run.fd_log == (FILE *) NULL )
        {
                Run.fd_log = fopen(Run.Trace_File_Name,"a");
                /* return(1); */
        }
        h = time(0); tm_s = *localtime(&h);
        if (msg[0]=='*')
        {
                fprintf(Run.fd_log,
                        "%02d/%02d/%d %02d:%02d:%02d: %s\n",
                        tm_s.tm_mday,  1+tm_s.tm_mon,  tm_s.tm_year,
                        tm_s.tm_hour, tm_s.tm_min, tm_s.tm_sec, msg+1 );
                fflush(Run.fd_log);
                
                fprintf(stderr,
                        "%02d/%02d/%d %02d:%02d:%02d: %s\n",
                        tm_s.tm_mday,  1+tm_s.tm_mon,  tm_s.tm_year,
                        tm_s.tm_hour, tm_s.tm_min, tm_s.tm_sec, msg+1 );
        }
        else
        {
                fprintf(Run.fd_log,
                        "%02d/%02d/%d %02d:%02d:%02d: %s\n",
                        tm_s.tm_mday,  1+tm_s.tm_mon,  tm_s.tm_year,
                        tm_s.tm_hour, tm_s.tm_min, tm_s.tm_sec, msg );
                fflush(Run.fd_log);
        }
        return(0);
}


void DumpMsg(unsigned char *bp, int len, char *msg)
{
        int     i;
        FILE *out;
        out = fopen("mipXfer.log","a");
        if(msg != NULL)
                fprintf(out,"******************  %s **************\n", msg);
        fprintf(out,"Length: %d\n<", len);

        for(i = 0; i < len; ++i, ++bp)
        {
                fprintf(out,"%02x", *bp);
        }
        fprintf(out,">\n");
        fclose(out);
}



void Dump_Message (
        FILE * file,
        const char *sdirecao,
        unsigned char * smensagem,
        int len)
{
#define M 16
        char asciitab[] = {

                                  0x00,0x00,0x00,0x00,0x00,0x09,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x0C,0x0D,0x00,0x00,
                                  0x00,0x11,0x12,0x13,0x00,0x0A,0x00,0x00,
                                  0x00,0x19,0x00,0x00,0x1C,0x1D,0x1E,0x00,
                                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,

                                  0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x63,0x2E,0x3C,0x28,0x2B,0x7C,
                                  0x26,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x21,0x24,0x2A,0x29,0x3B,0x5E,
                                  0x2D,0x2F,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x7C,0x2C,0x25,0x5F,0x3E,0x3F,
                                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,

                                  0x00,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
                                  0x68,0x69,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,
                                  0x71,0x72,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,
                                  0x79,0x7A,0x00,0x00,0x00,0x5B,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x00,0x5D,0x00,0x5F,

                                  0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
                                  0x48,0x49,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,
                                  0x51,0x52,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x5C,0x00,0x53,0x54,0x55,0x56,0x57,0x58,
                                  0x59,0x5A,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
                                  0x38,0x39,0x00,0x00,0x00,0x00,0x00,0x00
                          };

        int inLine;
        int z;
        int y;
        char line[79];
        char ch;
        int col[16] = {  5, 8, 11, 14, 17, 20, 23, 26,
                         29, 32, 35, 38, 41, 44, 47, 50 };
        int chl[16] = { 51, 52, 53, 54, 55, 56, 57, 58,
                        60, 61, 62, 63, 64, 65, 66, 67, };
        memset( line,32,78 );
        line[78]=0;
        fprintf (file,
                 "\n%s\n EBCDIC Message: #%02d bytes\n\n", sdirecao, len );
        fprintf (file,
                 "     00.01.02.03.04.05.06.07 08.09.10.11.12.13.14.15");
        fprintf (file, " 01234567 89ABCDEF\n");
        fprintf (file, "     --.--.--.--.--.--.--.-- --.--.--.--.--.--.--.--");
        fprintf (file, " ........ ........\n");
        inLine = 0;
        y = 0;
        for (z = 0; z < len; z++)
        {
                sprintf( (line +  col[inLine]),
                         "%02X", smensagem[z]);
                line[col[inLine]+2] = 32;
                ch = asciitab[smensagem[z]];
                line[chl[inLine]+2] = isprint((int)(ch))?ch:'.';
                inLine++;
                if ( inLine != 16) continue;
                inLine = 0;
                sprintf( line, "%04d", y );
                line[4] = 32;
                /* line[5] = 32; */
                fprintf(file,"%s\n", line);
                y = z + 1;
                memset( line, 32, 78);
        } /* for */
        if ( inLine )
        {
                sprintf( line, "%04d", y );
                line[4] = 32;
                fprintf (file, "%s\n", line );
        }
        fprintf (file,
                 "     --.--.--.--.--.--.--.-- --.--.--.--.--.--.--.--");
        fprintf (file,
                 " ........ ........\n");
        fflush(file);
} /* dump_mensagem */


void conv_ebcdic(
        unsigned char * destino,
        unsigned char * origem,
        unsigned int tamanho
)
{

        unsigned char psibyte;
        unsigned int index;
        static  unsigned char ebcdic_tab[256] = {
                                                        /* 0 */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                                        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                                        /* 1 */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                                        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                                        /* 2 */ 0x40,0x5a,0x7f,0x7b,0x5b,0x6c,0x50,0x7d,
                                                        0x4d,0x5d,0x5c,0x4e,0x6b,0x60,0x4b,0x61,
                                                        /* 3 */ 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
                                                        0xf8,0xf9,0x7a,0x5e,0x4c,0x7e,0x6e,0x6f,
                                                        /* 4 */ 0x7c,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
                                                        0xc8,0xc9,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,
                                                        /* 5 */ 0xd7,0xd8,0xd9,0xe2,0xe3,0xe4,0xe5,0xe6,
                                                        0xe7,0xe8,0xe9,0x4a,0xe0,0x5a,0x5f,0x6d,
                                                        /* 6 */ 0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
                                                        0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
                                                        /* 7 */ 0x97,0x98,0x99,0xa2,0xa3,0xa4,0xa5,0xa6,
                                                        0xa7,0xa8,0xa9,0xc0,0x6a,0xd0,0xa1,0x00
                                                };
        index = 0;
        while(tamanho > 0)
        {
                psibyte = origem[index];
                destino[index] = ebcdic_tab[psibyte];
                index++;
                tamanho --;
        }
}


int  conv_ascii(
        unsigned char * destino,
        unsigned char * origem,
        unsigned int tamanho
)
{

        char asciitab[] = {
                                  0x00,0x00,0x00,0x00,0x00,0x09,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x0C,0x0D,0x00,0x00,
                                  0x00,0x11,0x12,0x13,0x00,0x0A,0x00,0x00,
                                  0x00,0x19,0x00,0x00,0x1C,0x1D,0x1E,0x00,
                                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,
                                  0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x63,0x2E,0x3C,0x28,0x2B,0x7C,
                                  0x26,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x21,0x24,0x2A,0x29,0x3B,0x5E,
                                  0x2D,0x2F,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x7C,0x2C,0x25,0x5F,0x3E,0x3F,
                                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
                                  0x00,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
                                  0x68,0x69,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,
                                  0x71,0x72,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,
                                  0x79,0x7A,0x00,0x00,0x00,0x5B,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x00,0x00,0x00,0x00,0x00,0x5D,0x00,0x5F,
                                  0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
                                  0x48,0x49,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,
                                  0x51,0x52,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x5C,0x00,0x53,0x54,0x55,0x56,0x57,0x58,
                                  0x59,0x5A,0x00,0x00,0x00,0x00,0x00,0x00,
                                  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
                                  0x38,0x39,0x00,0x00,0x00,0x00,0x00,0x00
                          };


                          unsigned char psibyte;
                          unsigned int auxind;
                          unsigned int index,tam_real;


#define CODDLE  0x10
#define CODSTX  0x02
#define CODETX  0x03
#define SYNC    0x32

#define CODNUL  0X20                            /* codigo nulo */

        /* tamanho = tamanho - 5;    ** 02 27 f5 c2 03 */

        index = 0;
        /* auxind = 4;             **  02 27 f5 c2 */
        auxind = 0;             /* DLE[01] STX[02]  */

        tam_real = tamanho;

        while(tamanho > 0)
        {
                /* converte ebcdic para ascii - ignora 'ordem'*/

                psibyte = origem[auxind];

                /*
                            psibyte = prcv_buffer[auxind];
                            printf("-EBC:[%x]", origem[auxind]);
                */

                switch(psibyte)         /* EBCDIC */
                {
                case CODDLE:
                        tamanho--;   /* 0x10 */
                        auxind++;
                        if (psibyte = origem[auxind] == CODDLE)
                        {
                                tamanho--;   /* 0x10 */
                                /*
                                                    printf("-DLE:[%x]\n", origem[auxind]);
                                */
                                auxind++;
                                tam_real--;
                        }
                        else
                                if (psibyte = origem[auxind] == CODSTX)
                                {
                                        tamanho--;   /* 0x02 */
                                        tam_real--;
                                        /*
                                                           printf("-STX:[%x]\n", origem[auxind]);
                                        */
                                        auxind++;
                                        tam_real--;
                                }
                                else
                                        if (psibyte = origem[auxind] == CODETX)
                                        {
                                                tamanho--;   /* 0x03 */
                                                tam_real--;
                                                /*
                                                                  printf("-ETX:[%x]\n", origem[auxind]);
                                                */
                                                auxind++;
                                                tam_real--;
                                        }
                        break;
                case SYNC:
                        tamanho--;   /* 0x32 */
                        auxind++;
                        tam_real--;
                        break;
                default:
                        destino[auxind] = asciitab[psibyte];
                        /*
                                             printf("-ASC:[%c]\n", destino[auxind]);
                        */
                        tamanho--;
                        index++;
                        auxind++;
                }
        }
        return(tam_real);
}

