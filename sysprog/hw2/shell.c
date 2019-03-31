#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define ARG_MAX 10000

struct cmd
{
    char *name;
    char **argv;
    int argc;
};

struct file
{
    char *filename;
    int mode;
};

void my_realloc(struct cmd **cmds, int cur)
{
    *cmds = realloc(*cmds, (cur + 1) * sizeof(struct cmd));
    (*cmds)[cur].argv = NULL;
    (*cmds)[cur].argc = 0;
}

void my_free(struct cmd *cmds, struct file f, int sz)
{
    for (int i = 0; i < sz; ++i) {
        for (int j = 0; j < cmds[i].argc; ++j) {
            free(cmds[i].argv[j]);
        }
        free(cmds[i].argv);
    }
    if (f.filename) {
        free(f.filename);
    }
    free(cmds);
}

char *my_cpy(char *begin, char *end)
{
    char *res = calloc(end - begin + 1, sizeof(char));
    char *t = res;
    while (begin < end) {
        *t++ = *begin++;
    }
    *t = 0;
    return res;
}


void parse_word(struct cmd **cmds, char *begin, char *end, int cur)
{
    char *s = my_cpy(begin, end);
    (*cmds)[cur].argv = realloc((*cmds)[cur].argv, sizeof(char*) * ((*cmds)[cur].argc + 1));
    if ((*cmds)[cur].argc == 0) {
        (*cmds)[cur].name = s;
    }
    (*cmds)[cur].argv[(*cmds)[cur].argc++] = s;
}

void parse_string_with_q(int c, char *s, int *j)
{
    int state = 0;
    int ch = getchar();
    while (1) {
        if (!state) {
            if (ch == '\\') {
                state = 1;
            } else if (ch == c) {
                return;
            } else {
                s[(*j)++] = ch;
            }
        } else {
            if (ch == '\\' || ch == c) {
                s[(*j)++] = ch;
            } else if (ch != '\n') {
                s[(*j)++] = '\\';
                s[(*j)++] = ch;
            }
            state = 0;
        }
        ch = getchar();
    }
}

char parse_string_without_q(char *s, int *j)
{
    int state = 0, ch;
    do {
        ch = getchar();
    } while (ch == ' ');
    if (ch == EOF) {
        return  EOF;
    }
    while (1) {
        if (!state) {
            if (ch == '\'' || ch == '"') {
                parse_string_with_q(ch, s, j);
            } else if (ch == '\\') {
                state = 1;
            } else if (ch == '|' || ch == '>' || ch == ' ' || ch == '\n' || ch == '#') {
                return ch;
            } else {
                s[(*j)++] = ch;
            }
        } else {
            if (!isspace(ch) || ch == ' ') {
                s[(*j)++] = ch;
            }
            state = 0;
        }
        ch = getchar();
    }
}

int parse_file(struct file *f, char *s, int *j)
{
    int ch, start = *j;
    if ((ch = getchar()) == '>') {
        f->mode = O_APPEND;
    } else {
        f->mode = O_TRUNC;
    }
    int res = parse_string_without_q(s, j);
    if (f->mode == O_APPEND || ch == ' ') {
        f->filename = my_cpy(s + start, s + *j);
    } else {
        f->filename = my_cpy(s + start - 1, s + *j);
        f->filename[0] = ch;
    }
    f->mode = f->mode | O_CREAT | O_WRONLY;
    return res;
}

void execute(struct cmd *cmds, struct file f, int size) {
    int fd[size][2];
    for (int i = 0; i < size; ++i) {
        if (!strcmp(cmds[i].name, "cd")) {
            chdir(cmds[i].argv[1]);
            continue;
        }
        pipe(fd[i]);
        if (!fork()) {
            if (i) {
                dup2(fd[i-1][0], 0);
            }
            if (i != size - 1) {
                dup2(fd[i][1], 1);
            } else if (f.filename) {
                int des = open(f.filename, f.mode, 0666);
                dup2(des, 1);
                close(des);
            }
            close(fd[i][1]);
            cmds[i].argv = realloc(cmds[i].argv, sizeof(char*) * (cmds[i].argc + 1));
            cmds[i].argv[cmds[i].argc] = NULL;
            if (execvp(cmds[i].name, cmds[i].argv) == -1) {
                exit(0);
            }
        }
        close(fd[i][1]);
    }
    for (int i = 0; i < size; ++i) {
        wait(NULL);
    }
}

void parse_string(char *s, int n)
{
    int j = 0, ch, start = 0, cur = 0;
    struct cmd *cmds = NULL;
    my_realloc(&cmds, cur);
    struct file f;
    f.filename = NULL;
    while ((ch = parse_string_without_q(s, &j)) != '\n') {
        if (ch == EOF) {
            free(cmds);
            exit(0);
        }
        if ((ch == ' ' || ch == '|' || ch == '>' || ch == '#') && start != j) {
            parse_word(&cmds, s + start, s + j, cur);
            start = j;
        }
        if (ch == '|') {
            cur++;
            my_realloc(&cmds, cur);
            start = ++j;
        } else if (ch == '#') {
            while ((ch = getchar()) != '\n');
            break;
        } else if (ch == '>') {
            if (parse_file(&f, s, &j) == '\n') {
                start = j;
                break;
            }
            start = j;
        }
    }
    if (start != j && !f.filename) {
        parse_word(&cmds, s + start, s + j, cur);
    }
    if (cmds[0].argc)
        execute(cmds, f, cur + 1);
    my_free(cmds, f, cur + 1);
}


int main()
{
    char s[ARG_MAX];
    while (1) {
        printf("$ ");
        parse_string(s, 1000);
    }
    return 0;
}
