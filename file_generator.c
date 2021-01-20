#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* Simple data generator used for testing eraser.c program  */
int main()
{
 char in[200];
 printf("Enter size in KB: ");
 scanf("%s",in);
 int size = atoi(in);
 printf("Enter name: ");
 scanf("%s",in);
 
 FILE * fPtr;
 fPtr = fopen(in, "w");
 if(fPtr == NULL)
 {
 /* File not created hence exit */
 printf("Unable to create file.\n");
 exit(EXIT_FAILURE);
 }
 unsigned char* out = malloc(size*1000);
 for(int i=0; i<size*1000 ;i++)
 {
 	out[i] = '0' + i%10;	
 }
 fputs(out, fPtr);
 fclose(fPtr);	
}
