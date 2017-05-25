/* uvsh.c

Requirements: 	https://connex.csc.uvic.ca/access/content/attachment/9ad57451-aeeb-44ea-9900-3e4871d16fa4/Assignments/29d50c2c-9759-4035-a9f0-5694475f2278/2017_summer_a1_writeup.pdf

Github Link: 	https://github.com/colemacdonald/shell

Created On: 	Wed May 24
By:				Cole Macdonald

*/

#include <stdio.h>
#include <string.h>

int main(void)
{
	for(;;)
	{

		printf("> ");
		char str[100];

		scanf("%s", str);

		if(strcmp(str, "exit") == 0)
			break;

		printf("%s\n", str);
	}

	return 0;
}