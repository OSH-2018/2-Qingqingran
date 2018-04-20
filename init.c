#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>

int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];    
    while (1) {
        /* 提示符 */
        printf("# ");
        fflush(stdin);
        fgets(cmd, 256, stdin);
        /* 清理结尾的换行符 */
        int i;
        for (i = 0; cmd[i] != '\n'; i++)
            ;
        cmd[i] = '\0';
        char *seek = cmd;
        /* 拆解命令行 */
        int flag  = 0;
        int fd[2];
        int prev = -1;
        while (flag == 0) {
            for (i = 0; ; ++i) {
                while (*seek == ' ') 
                    seek++;
                if (*seek == '|'){
                    seek++;
                    break;
                }
                if (*seek == '\0'){
                    flag = 1;
                    break;
                }                   
                char tmp[256];
                int cnt = 0;
                while (*seek != ' ' && *seek != '\0') {
                    tmp[cnt++] = *seek;
                    seek++;
                }
                tmp[cnt] = '\0';
                args[i] = strdup(tmp);
            }
            args[i] = NULL;

            /* 没有输入命令 */
            if (!args[0])
                continue;

            /* 内建命令 */
            if (strcmp(args[0], "cd") == 0) {
                if (args[1])
                    chdir(args[1]);
                continue;
            }
            if (strcmp(args[0], "pwd") == 0) {
                char wd[4096];
                puts(getcwd(wd, 4096));
                continue;
            }
            if (strcmp(args[0], "export") == 0) {
                for (int j = 1; args[j] != NULL; j++) {
                    char *s = args[j];
                    while (*s != '=')
                        s++;
                    *s = '\0';
                    s++;
                    setenv(args[j], s, 1);
                }
            }
            if (strcmp(args[0], "exit") == 0)
                return 0;
            
            /* 外部命令 */
            int ret;
            if ((ret = pipe(fd)) == -1){
                perror("fork error!");
                exit(0);
            }
            pid_t pid = fork();
            /* 创建失败 */
            if (pid < 0) {
                perror("fork error!");
                exit(0);
            }
            /* 子进程 */
            else if (pid == 0) {               
                close(fd[0]);
                if (prev != -1)
                    if ((ret = dup2(prev,STDIN_FILENO)) == -1){
                        perror("dup error!");
                        exit(0);
                    }
                if (flag == 0)
                    if ((ret = dup2(fd[1],STDOUT_FILENO)) == -1){
                        perror("dup error!");
                        exit(0);
                    }
                else
                    close(fd[1]);
                execvp(args[0], args);
                /* execvp失败 */
                return 255;
            }
            /* 父进程 */
            else {
                prev = fd[0];
                close(fd[1]);
            }    
        }
        while (wait(NULL) > 0)
            ;
    }
}