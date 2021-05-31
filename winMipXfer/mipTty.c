/*      
 *      @(#) 
 *      @(#)  prepara terminal para uso durante file xfer
 *      @(#) Modulo mipTty.c 3.1 25 Aug 1999 19:10:08 
 *      @(#) JFAB 2004 Jose Fernando Alvim Borges
 *      @(#) MIP  TCP/IP interface
 *      @(#) MIP  TCP/IP interface
 *      @(#) ultima alteracao:
 *      @(#) 
 *
 */

#include <stdio.h>


#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

#include <time.h>

int Set_Term(int tty, int t)
{
int r; 
struct termios term; 

        if ( t == 1 )
        {
                r = ioctl(tty,TCGETS,&term); 
                if ( r < 0 )
                {
                        perror("tcgetattr (1)"); 
                        return(-1); 
                }
                term.c_cc[VMIN] = 0; 
                term.c_cc[VTIME] = 0; 
                term.c_lflag = term.c_lflag & ~ICANON & ~ECHO; 
                r = ioctl(tty,TCSETSF,&term); 
                if ( r < 0 )
                {
                        perror("tcsetattr (1)"); 
                        return(-1); 
                }
                return(0); 
        }
        r = ioctl(tty,TCGETS,&term); 
        if ( r < 0 )
        {
                perror("tcgetattr (2)"); 
                return(-1); 
        }
        term.c_cc[VMIN] = 1; 
        term.c_cc[VTIME] = 0; 
        term.c_lflag = term.c_lflag | ECHO; 
        r = ioctl(tty,TCSETSF,&term); 
        if ( r < 0 )
        {
                perror("tcsetattr (2)"); 
                return(-1); 
        }
        return(0); 
}       /* End Set_Term() */
