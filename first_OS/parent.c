#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


int write_all(int fd, const char* buffer, size_t size) {
    size_t written = 0;


    while (written < size) {
        ssize_t result = write(fd, buffer + written, size - written);

        if (result <= 0) {
            return -1;

        }

        written += result;
    }

    return 0;

}

int main(void) {

    char filename[256];
    int index = 0;

    if (write_all(STDOUT_FILENO, "Enter the filename: ", 20) == -1) {
        return 1;
    }

    while (index < 255) {
        char symbol;

        ssize_t result = read(STDIN_FILENO, &symbol, 1);

        if (result < 0) {
            write_all(STDERR_FILENO, "read error.\n", 12);
            return 1;
            
        }

        if (result == 0 || symbol == '\n') {
            break;
        }

        filename[index] = symbol;
        index++;
    }


    filename[index] = '\0';


    int pipefd[2];

    if (pipe(pipefd) == -1) {
        write_all(STDERR_FILENO, "pipe error.\n", 12);
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        write_all(STDERR_FILENO, "Fork failed.\n", 13);
        close(pipefd[0]);
        close(pipefd[1]);

        return 1;
    }

    if (pid == 0) {
        close(pipefd[1]);

        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            write_all(STDERR_FILENO, "dup2 error.\n", 12);
            close(pipefd[0]);
            return 1;
        }

        close(pipefd[0]);
        
        char* argv[] = {"./child", filename, NULL};

        char* envp[] = {NULL};

        execve("./child", argv, envp);


        write_all(STDERR_FILENO, "execve error.\n", 14);
        return 1;
    }

    close(pipefd[0]);

    if (write_all(STDOUT_FILENO, "Enter number: (Ctrl+D) \n", 24) == -1) {
        close(pipefd[1]);
        return 1;
    }

    while (1) {
        char symbol;

        ssize_t result = read(STDIN_FILENO, &symbol, 1);

        if (result < 0) {
            write_all(STDERR_FILENO, "read error.\n", 12);
            close(pipefd[1]);
            return 1;
        }

        if (result == 0) {
            break;
        }

        if (write_all(pipefd[1], &symbol, 1) == -1) {
            write_all(STDERR_FILENO, "write error\n", 12);
            close(pipefd[1]);
            return 1;
        }
    }


    if (close(pipefd[1]) == -1) {
        write_all(STDERR_FILENO, "close error.\n", 13);
        return 1;
    }

    int status;

    if (waitpid(pid, &status, 0) == -1) {
        write_all(STDERR_FILENO, "waitpid error.\n", 15);
        return 1;
    }

    if (WIFEXITED(status)) {
        int child_status = WEXITSTATUS(status);

        if (child_status != 0) {
            write_all(STDERR_FILENO, "Child finished with error.\n", 27);
            return 1;
        }
    } else {
        write_all(STDERR_FILENO, "Child terminated. \n", 20);
        return 1;
    }

    return 0;

}