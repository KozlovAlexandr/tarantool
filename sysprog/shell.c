#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

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

struct string
{
    char *ptr;
    size_t size;
    size_t capacity;
};

int is_special(int ch)
{
    return ch == ' ' || ch == '|' || ch == '>' || ch == '#';
}

void push(struct string *s, int ch)
{
    if (s->capacity <= s->size) {
        s->capacity = s->capacity * 2 + 1;
        s->ptr = realloc(s->ptr, s->capacity);
        if (!s->ptr) {
            exit(1);
        }
    }
    s->ptr[s->size++] = ch;
}

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
    free(f.filename);
    free(cmds);
}

char *get_token(char *begin, char *end)
{
    char *res = calloc(end - begin + 1, sizeof(char));
    memcpy(res, begin, end - begin);
    res[end - begin] = 0;
    return res;
}


void parse_token(struct cmd **cmds, char *begin, char *end, int cur)
{
    char *token = get_token(begin, end);
    struct cmd *cur_cmd = *cmds + cur;
    cur_cmd->argv = realloc(cur_cmd->argv, sizeof(char*) * (cur_cmd->argc + 1));
    if (cur_cmd->argc == 0) {
        cur_cmd->name = token;
    }
    cur_cmd->argv[cur_cmd->argc++] = token;
}

void parse_string_in_q(int q_type, struct string *s)
{
    int state = 0;
    while (1) {
        int ch = getchar();
        if (!state) {
            if (ch == '\\') {
                state = 1;
            } else if (ch == q_type) {
                return;
            } else {
                push(s, ch);
            }
        } else {
            if (ch == '\\' || ch == q_type) {
                push(s, ch);
            } else if (ch != '\n') {
                push(s, '\\');
                push(s, ch);
            }
            state = 0;
        }
    }
}

char parse_string_without_q(struct string *s)
{
    int carry_flag = 0, ch;
    do {
        ch = getchar();
    } while (ch == ' ');
    if (ch == EOF) {
        return  EOF;
    }
    while (1) {
        if (!carry_flag) {
            if (ch == '\'' || ch == '"') {
                parse_string_in_q(ch, s);
            } else if (ch == '\\') {
                carry_flag = 1;
            } else if (is_special(ch) || ch == '\n') {
                return ch;
            } else {
                push(s, ch);
            }
        } else {
            if (!isspace(ch) || ch == ' ') {
                push(s, ch);
            }
            carry_flag = 0;
        }
        ch = getchar();
    }
}

int parse_file(struct file *f, struct string *s)
{
    int ch;
    int start = s->size;
    if ((ch = getchar()) == '>') {
        f->mode = O_APPEND;
    } else {
        f->mode = O_TRUNC;
    }
    int res = parse_string_without_q(s);
    if (f->mode == O_APPEND || ch == ' ') {
        f->filename = get_token(s->ptr + start, s->ptr + s->size);
    } else {
        f->filename = get_token(s->ptr + start - 1, s->ptr + s->size);
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

void parse_string(struct string *s)
{
    int ch, start = 0, cur_cmd = 0;
    struct cmd *cmds = NULL;
    my_realloc(&cmds, cur_cmd);
    struct file f;
    f.filename = NULL;
    while ((ch = parse_string_without_q(s)) != '\n') {
        if (ch == EOF) {
            free(cmds);
            exit(0);
        }
        if (is_special(ch) && start != s->size) {
            parse_token(&cmds, s->ptr + start, s->ptr + s->size, cur_cmd);
            start = s->size;
        }
        if (ch == '|') {
            my_realloc(&cmds, ++cur_cmd);
        } else if (ch == '#') {
            while ((ch = getchar()) != '\n');
            break;
        } else if (ch == '>') {
            if (parse_file(&f, s) == '\n') {
                start = s->size;
                break;
            }
            start = s->size;
        }
    }
    if (start != s->size && !f.filename) {
        parse_token(&cmds, s->ptr + start, s->ptr + s->size, cur_cmd);
    }
    if (cmds[0].argc)
        execute(cmds, f, cur_cmd + 1);
    my_free(cmds, f, cur_cmd + 1);
    s->size = 0;
}


int main()
{
    struct string input_str;
    memset(&input_str, 0, sizeof(input_str));
    while (1) {
        parse_string(&input_str);
    }
    return 0;
}
