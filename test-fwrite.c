#include <stdio.h>

int main()
{
    FILE *fp = NULL;

    short x[10] = {1,2,3,4,5,6,5000,6,-10,11};
    short result[10];
    int i;

    fp=fopen("ss.txt", "w+");

    if(fp != NULL)
    {
        fwrite(x, sizeof(short), 10 /*20/2*/, fp);
        rewind(fp);
        fread(result, sizeof(short), 10 /*20/2*/, fp);
    }
    else
        return 1;

    printf("Result\n");
    for (i = 0; i < 10; i++)
        printf("%d = %d\n", i, (int)result[i]);

    fclose(fp);
    return 0;
}
