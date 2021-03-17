#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(){
    int fp_r;
    int fp_w;
    char filepath_r[50];
    char filepath_w[50];
    
    printf("input the filepath of the source file:");
    scanf("%s", filepath_r);
    printf("input the filepath of destination file:");
    scanf("%s", filepath_w);

    fp_r = open(filepath_r, O_RDONLY);
    fp_w = open(filepath_w, O_WRONLY|O_CREAT|O_TRUNC|00070);

    char buffer[1024];
    int ret = 1;
    while(1){
        ret = read(fp_r, buffer, sizeof(buffer));
        write(fp_w, buffer, ret);
        if(ret < sizeof(buffer))    break;
    }
    close(fp_r);
    close(fp_w);

    return 0;
}