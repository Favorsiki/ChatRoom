/*************************************************************************
	> File Name: test.c
	> Author: 
	> Mail: 
	> Created Time: Sat 11 Jul 2020 09:07:40 AM CST
 ************************************************************************/

#include<stdio.h>

int main() {

    FILE * fp = fopen("./photo.jpg", "r");
    char str[100] = {0};
    while (fgets(str, 100, fp) != NULL) {
        printf("%s\n", str);
    }
    fclose(fp);
    return 0;
}
