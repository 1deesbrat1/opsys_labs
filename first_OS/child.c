#include <unistd.h>
#include <fcntl.h>


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
 
int write_number(int fd, unsigned long long number) {
    char buffer[32];
    int index = 0;

    if (number == 0) {
        if (write_all(fd, "0", 1) == -1) {
            return -1;
        }

        return 0;
    }

    while (number > 0) {
        buffer[index] = '0' + (number % 10);
        number /= 10;
        index++;
    }

    while (index > 0) {
        index--;
        if (write_all(fd, &buffer[index], 1) == -1) {
            return -1;
        }
    }
    return 0;
}

int write_float(int fd, float number) {

    if (number < 0) {
        if (write_all(fd, "-", 1) == -1) {
            return -1;
        }
        number = -number;
    }

    number += 0.0000005f;

    unsigned long long integer_part = (unsigned long long)number;

    if (write_number(fd, integer_part) == -1) {
        return -1;
    }
    if (write_all(fd, ".", 1) == -1) {
        return -1;
    }

    float fraction = number - (float)integer_part;

    for (int i = 0; i < 6; i++) {
        fraction *= 10.0f;

        int digit = (int) fraction;

        char symbol = '0' + digit;

        if (write_all(fd, &symbol, 1) == -1) {
            return -1;
        }

        fraction -= digit;
    }

    return 0;
}

int main(int argc, char* argv[]) {


    if (argc < 2) {
        write_all(STDERR_FILENO, "No filename.\n", 13);
        return 1;
    }

    int file_fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (file_fd == -1) {
        write_all(STDERR_FILENO, "open error.\n", 12);
        return 1;
    }

    char buffer[256];

    float current_number = 0.0f;
    float sum = 0.0f;
    
    int sign_seen = 0;
    int invalid_line = 0;
    int in_number = 0;
    int sign = 1;
    int line_has_number = 0;
    int after_dot = 0;
    float fraction_divisor = 10.0f;

    while (1) {
        ssize_t bytes = read(STDIN_FILENO, buffer, sizeof(buffer));

        if (bytes < 0) {
            write_all(STDERR_FILENO, "read error.\n", 12);
            close(file_fd);
            return 1;
        }

        if (bytes == 0) {

            if ((sign_seen || after_dot) && !in_number) {
                invalid_line = 1;
            }

            if (in_number) {
                sum += current_number * sign;
                line_has_number = 1;
            }

            if (invalid_line) {
                write_all(STDERR_FILENO, "Invalid input.\n", 15);
            } else if (line_has_number) {
                if (write_float(file_fd, sum) == -1) {
                    write_all(STDERR_FILENO, "write error.\n", 13);
                    close(file_fd);
                    return 1;
                }
                if (write_all(file_fd, "\n", 1) == -1) {
                    write_all(STDERR_FILENO, "write error.\n", 13);
                    close(file_fd);
                    return 1;
                }
            }

            break;
        }

        for (ssize_t i = 0; i < bytes; i++) {
            char symbol = buffer[i];

            if (symbol == '-') {
                if (in_number || sign_seen || after_dot) {
                    invalid_line = 1;
                } else {
                    sign = -1;
                    sign_seen = 1;
                }

            } else if (symbol == '.') {

                if (after_dot) {
                    invalid_line = 1;
                } else {
                    after_dot = 1;
                }

            } else if (symbol >= '0' && symbol <= '9') {

                int digit = symbol - '0';

                if (!after_dot) {
                    current_number = current_number * 10.0f + digit;
                } else {
                    current_number += digit / fraction_divisor;

                    fraction_divisor *= 10.0f;
                }

                in_number = 1;

            } else if (symbol == ' ' || symbol == '\n') {
                
                if ((sign_seen || after_dot) && !in_number) {
                    invalid_line = 1;
                }

                if (in_number) {
                    sum += current_number * sign;

                    line_has_number = 1;
                    sign = 1;
                    current_number = 0.0f;
                    in_number = 0;
                    sign_seen = 0;

                    after_dot = 0;
                    fraction_divisor = 10.0f;
                }
            
                if (symbol == '\n') {

                    if (invalid_line) {
                        write_all(STDERR_FILENO, "Invalid input.\n", 15);
                    } else if (line_has_number) {
                        if (write_float(file_fd, sum) == -1) {
                            write_all(STDERR_FILENO, "write error.\n", 13);
                            close(file_fd);
                            return 1;
                        }
                        if (write_all(file_fd, "\n", 1) == -1) {
                            write_all(STDERR_FILENO, "write error.\n", 13);
                            close(file_fd);
                            return 1;
                        }
                    }

                    sum = 0.0f;
                    current_number = 0.0f;

                    sign = 1;
                    sign_seen = 0;
                    in_number = 0;

                    after_dot = 0;
                    fraction_divisor = 10.0f;

                    line_has_number = 0;
                    invalid_line = 0;
                }

            }

            else {
                invalid_line = 1;
            }
        }
    }

    if (close(file_fd) == -1) {
        write_all(STDERR_FILENO, "close error.\n", 13);
        return 1;
    }

    return 0;
}