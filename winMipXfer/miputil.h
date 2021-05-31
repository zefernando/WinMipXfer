/*
 *      @(#)
 *      @(#) MIP  TCP/IP Interface
 *      @(#) header inicial
 *      @(#) Modulo mipUtil.h 1.0 10 Oct 2004
 *      @(#) JFAB 2004 Jose Fernando Alvim Borges
 *      @(#) ultima alteracao:
 *      @(#)
 *
 */
#include        <time.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <fcntl.h>
#include        <sys/types.h>
#include        <sys/stat.h>
#include        <string.h>
#include        <termio.h>
#include        <signal.h>
#include        <unistd.h>
#include        <libgen.h>

#define         mpLOG_OPEN              (0)
#define         mpLOG_RESET             (1)
#define         mpLOG_NORMAL            (2)
#define         mpMAX_LINE_SIZE         (128)

#define         MAX_EVENT_SIZE          (128)
#define         LOCK                    ".BulkXfer.lck"
#define         OPTSTRING               "hvj:d:t:s:HVJ:D:T:S:"

#define DATA_BUFSIZE                    (1024)
#define CONTROL_BUFSIZE                 (1024)

#define         stATTACH                (0)
#define         stREQUESTBULK           (1)
#define         stABORT                 (2)
#define         stDETACH                (3)
#define         stENDING                (4)
#define         stRECEIVING             (5)
#define         stWAITFORHEADER         (6)
#define         stEND                   (7)
#define         stPURGE                 (8)
#define         stCONFIRMPURGE          (9)
#define         stSTARTADVISEMENT       (10)
#define         stINSIDEADVISEMENT      (11)
#define         stABORTADVISEMENT       (12)
#define         stRESET                 (13)
#define         stRESETCONFIRM          (14)

typedef struct
{
        char *          N;
        char *          V;
        void *          nx;
}       _zfParm_List;

typedef struct _Run
{
        char    Bulk_Type[5];
        char    Endpoint[6];
        char    Julian_Date[4];
        char    Sequence_Number[3];
        char    Debug_Level;

        char    Advisement_Session[6];
        char    Advisement_Filter;

        char *  IpAddress;
        char *  Socket_Port;
        char *  Err_Msg;

        int     socket;
        int     fd_out;

        unsigned long Bytes_Received;
        unsigned long Max_Log_Size;
        unsigned int Block_Number;
        unsigned int Block_Count;
        unsigned short Block_Len;
        unsigned short Record_Len;
        unsigned long File_Size;

        int verbose;
        int iPhase;

        char *                  Trace_File_Name;
        char *                  Output_File;
        _zfParm_List *  Parm_List;
        char *                  Ok_Script;
        char *                  Fail_Script;

        time_t last;
        time_t now;
        time_t StartT;
        time_t EndT;

        int bBulk_Type;
        int bJulian_Date;
        int bSequence_Number;
        int bOutput;
        int bTest_Mode;
        int bASCII_Output;
        int bReport_Frequency;
        int Fail_Rate;
        int bTimeout;
        int iTimeout;
        int bForce;
        int bDebug;
        int bRSequence;
        int icfg;
        int maxcfg;
        int iTamBloco;

        char * sLog_File;
        char * sFileName;

        unsigned char *psnd_buffer;
        unsigned char *prcv_buffer;

        char    TID[14];                // Transmission ID

        unsigned int    iBulkFile;
        unsigned int    iFileSize;
        unsigned int    iJulianDay;
        unsigned int    iSeqno;
        unsigned int    iSpecial_Size;
        unsigned int    iRSequence;

    int  File_Type;
    int  Block_Size;

        int     intCount;

        char    File_Name[128];

        int     fd_in;
        int     fd_lock;
        int     fd_config;
        int             fd_Temp;
        FILE *  fd_log;

}       _Run;

typedef struct
{
        char    Type[1];
        char    Bulk_Type[3];
        char    Endpoint[5];
        char    Julian_Date[3];
        char    Sequence[2];
} Outbound_Header;

