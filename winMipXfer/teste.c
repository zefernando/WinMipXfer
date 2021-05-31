#include <stdio.h>

/* entry point */
int main(void)
{
	int siz;
	char aux[5] = { 0x00, 0x00, 0x00, 0x11 };

	siz = aux[0] << 32| aux[1]<<16|aux[2]<<8 | aux[3] ;
    printf("siz %d\n", siz);
    return 0;
}
