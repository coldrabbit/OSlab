#include<gtk/gtk.h>                             //引用gtk/gtk.h这个头文件
#include <stdio.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 32

int shmid_1;
int shmid_2;
int semid;
pid_t p1, p2, p3;

union semun{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* array for GETALL, SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
}arg;

struct share_buf{
	int lenth;
	char buffer[BUFFER_SIZE];
};
struct share_buf *share_buf_1;
struct share_buf *share_buf_2;


void P(int semid, int index){
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_op = -1;
    sem.sem_flg = 0;
    semop(semid, &sem, 1);
    return;
}

void V(int semid, int index){
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_op = 1;
    sem.sem_flg = 0;
    semop(semid, &sem, 1);
    return;
}

int main(int argc, char *argv[])                //标准c语言主函数的声明
{
    void *shmaddr_1 = NULL;
    void *shmaddr_2 = NULL;
    shmid_1 = shmget(IPC_PRIVATE, sizeof(struct share_buf), IPC_CREAT|0666); //申请共享内存空间
    shmaddr_1 = shmat(shmid_1, NULL, SHM_R|SHM_W);          //将共享内存空间连接到当前进程
    share_buf_1 = (struct share_buf *)shmaddr_1;

    shmid_2 = shmget(IPC_PRIVATE, sizeof(struct share_buf), IPC_CREAT|0666);
    shmaddr_2 = shmat(shmid_2, NULL, SHM_R|SHM_W);
    share_buf_2 = (struct share_buf *)shmaddr_2;

    /*创建信号量*/
	semid = semget(IPC_PRIVATE, 4, IPC_CREAT|0666); //创建4个信号灯，分别对应两个共享内存空间的可写入和可读取的资源数
	if(semid == -1){
		printf("semget failed\n");
		return -1;
	}
    /*第一个缓存空间*/
	arg.val = 1;
    semctl(semid, 0, SETVAL, arg);
    arg.val = 0;
    semctl(semid, 1, SETVAL, arg);
    /*第二个缓存空间*/
    arg.val = 1;
    semctl(semid, 2, SETVAL, arg);
    arg.val = 0;
    semctl(semid, 3, SETVAL, arg);

    p1 = fork();
    if(p1 < 0){
        printf("fork child process 1 failed\n");
        exit(-1);
    }
    else if(p1 == 0){
        //printf("get");
        GtkWidget *window;
        gtk_init(&argc, &argv);
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window),"GET");
        gtk_window_set_default_size(GTK_WINDOW(window),500,500);
        gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_NONE);
        GtkWidget *vbox = gtk_vbox_new(TRUE, 10);
        gtk_container_add(GTK_CONTAINER(window), vbox);
        GtkWidget *label_1 = gtk_label_new("由文件input.txt写入第一个缓冲空间");
        gtk_container_add(GTK_CONTAINER(vbox), label_1);
        GtkWidget *label_2 = gtk_label_new("label_two");
        gtk_container_add(GTK_CONTAINER(vbox), label_2);


        int fd;
        int lenth;
    	int times = 1;
        struct share_buf *in = share_buf_1;
	    fd = open("./input.txt", O_RDONLY);
	    if(fd == -1){
        	printf("file open failed(W)\n");
	        return 0;
        }
	    while(1){
	        P(semid, 0);
	        lenth = read(fd, in->buffer, BUFFER_SIZE);
	        if(lenth != BUFFER_SIZE){
		        printf("the last time read %d bytes from input file to buffer 1\n", lenth);
		        in->lenth = lenth;
                gtk_label_set_line_wrap(GTK_LABEL(label_2), TRUE);
                gtk_label_set_text(GTK_LABEL(label_2), in->buffer);
		        V(semid, 1);
		        break;
	        }
	        printf("the %d th read %d bytes from input file to buffer 1\n", times++, lenth);
            in->lenth = lenth;
	        V(semid, 1);
        }
        gtk_widget_show_all(window);
        g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
        gtk_main();
    }
    else{
        //printf("p2");
        p2 = fork();
        if(p2 < 0){
            printf("fork child process 2 failed\n");
            exit(-1);
        }
        else if(p2 == 0){
            GtkWidget *window;
            gtk_init(&argc, &argv);
            window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_title(GTK_WINDOW(window),"TRANS");
            gtk_window_set_default_size(GTK_WINDOW(window),500,500);
            gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_NONE);
            GtkWidget *vbox = gtk_vbox_new(TRUE, 10);
            gtk_container_add(GTK_CONTAINER(window), vbox);
            GtkWidget *label_1 = gtk_label_new("由第一个缓冲空间写入第二个缓冲空间");
            gtk_container_add(GTK_CONTAINER(vbox), label_1);
            GtkWidget *label_2 = gtk_label_new("label_two");
            gtk_container_add(GTK_CONTAINER(vbox), label_2);


            int times = 1;
            struct share_buf *in = share_buf_1;
            struct share_buf *out = share_buf_2;
            while(1){
                P(semid, 1);
                P(semid, 2);
                if(in->lenth != BUFFER_SIZE){
                    printf("the last time trans %d bytes from buffer 1 to buffer 2\n", in->lenth);
                    strncpy(out->buffer, in->buffer, in->lenth);
                    out->lenth = in->lenth;
                    gtk_label_set_line_wrap(GTK_LABEL(label_2), TRUE);
                    gtk_label_set_text(GTK_LABEL(label_2), in->buffer);
                    V(semid, 0);
                    V(semid, 3);
                    break;
                }
                strncpy(out->buffer, in->buffer, in->lenth);
                printf("the %d th trans %d bytes from buffer 1 to buffer 2\n", times++,  in->lenth);
                out->lenth = in->lenth;
                V(semid, 0);
                V(semid, 3);
            }
            gtk_widget_show_all(window);
            g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
            gtk_main();
        }
        else{
            p3 = fork();
            if(p3 < 0){
                printf("fork child process 3 failed\n");
                exit(-1);
            }
            else if(p3 == 0){
            GtkWidget *window;
            gtk_init(&argc, &argv);
            window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_title(GTK_WINDOW(window),"PUT");
            gtk_window_set_default_size(GTK_WINDOW(window),500,500);
            gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_NONE);
            GtkWidget *vbox = gtk_vbox_new(TRUE, 10);
            gtk_container_add(GTK_CONTAINER(window), vbox);
            GtkWidget *label_1 = gtk_label_new("由第二个缓冲空间写入文件output.txt");
            gtk_container_add(GTK_CONTAINER(vbox), label_1);
            GtkWidget *label_2 = gtk_label_new("label_two");
            gtk_container_add(GTK_CONTAINER(vbox), label_2);

            

            int fd;
	        int times = 1;
        	struct share_buf *out = share_buf_2;
        	fd = open("./output.txt", O_WRONLY|O_CREAT|O_TRUNC);
            if(fd == -1){
        		printf("file open failed(R)\n");
        		return 0;
        	}
	        while(1){
        		P(semid, 3);
                // sleep(1);
                if(out->lenth != BUFFER_SIZE){
        			printf("the last time write %d bytes from buffer 2 to output file\n", out->lenth);
                    gtk_label_set_line_wrap(GTK_LABEL(label_2), TRUE);
                    gtk_label_set_text(GTK_LABEL(label_2), out->buffer);
        			write(fd, out->buffer, out->lenth);
                    V(semid, 2);
        			break;
        		}
                write(fd, out->buffer, BUFFER_SIZE);
        		printf("the %d th write %d bytes from buffer 2 to output file\n", times++, out->lenth);
        		V(semid, 2);
        	}
            gtk_widget_show_all(window);
            g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
            gtk_main();
            }   
            else{
                waitpid(p1, NULL, 0);
                waitpid(p2, NULL, 0);
                waitpid(p3, NULL, 0);
                printf("child processes are killed\n");
                semctl(semid, 0, IPC_RMID, arg);
                semctl(semid, 1, IPC_RMID, arg);
                semctl(semid, 2, IPC_RMID, arg);
                semctl(semid, 3, IPC_RMID, arg);
                printf("semid deleted\n");
                shmctl(shmid_1, IPC_RMID, 0);
                shmctl(shmid_2, IPC_RMID, 0);
                printf("buffer deleted\n");
                printf("father process is killed\n");
            }
        }
    }
    return 0;
}
