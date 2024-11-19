#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

// Функция для создания и записи текста в файл
void create_text(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("File creation failed");
        exit(EXIT_FAILURE);
    }

    // Записываем текст в файл
    const char *text = "first_123_lab    ";
    fprintf(file, "%s\n", text);

    fclose(file);
}

void child_process1(int read_fd, int write_fd) {
    char buffer[BUFFER_SIZE];
    int index = 1;

    while (1) {
        int bytes_read = read(read_fd, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            break;
        }
        //buffer[bytes_read] = '\0';

        // Добавляем индекс после каждого символа
        for (int i = 0; i < bytes_read; i++) {
            // Пропускаем завершающие пробелы и символы новой строки
            if (buffer[i] == '\n' || buffer[i] == '\r') {
                continue;
            }
            // Пишем символ, затем индекс и пробел
            write(write_fd, &buffer[i], 1);
            // Преобразуем индекс в строку и записываем его
            char index_str[10];  // Буфер для индекса (до 10 разрядов)
            int index_length = snprintf(index_str, sizeof(index_str), "%d", index);
            write(write_fd, index_str, index_length);

            index++;
        }
    }

    close(read_fd);
    close(write_fd);
}


void child_process2(int read_fd) {
    char buffer[BUFFER_SIZE];
    char formatted[BUFFER_SIZE * 2] = {0};  // Для результата
    int index = 1;                          // Текущий индекс
    int i = 0;                              // Позиция в строке результата

    // Читаем данные из FIFO2
    int bytes_read = read(read_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        perror("Error reading from FIFO2");
        close(read_fd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_read] = '\0';  // Завершаем строку

    // Обрабатываем текст: разделяем символы и индексы
    while (buffer[i] != '\0') {
        // Берём текущий символ
        char current_char = buffer[i];
        i++;

        // Определяем разрядность текущего индекса
        int temp_index = index;
        int index_length = 0;
        do {
            index_length++;
            temp_index /= 10;  // Уменьшаем число, удаляя последнюю цифру
        } while (temp_index != 0);

        // Извлекаем индекс из строки
        char number[index_length + 1];  // Буфер для индекса
        for (int j = 0; j < index_length; j++) {
            number[j] = buffer[i + j];  // Копируем символы индекса
        }
        number[index_length] = '\0';    // Завершаем строку индекса
        i += index_length;              // Сдвигаем позицию

        // Добавляем символ, пробел, индекс, пробел в результирующую строку
        strncat(formatted, &current_char, 1);
        strcat(formatted, " ");                // Добавляем пробел
        strcat(formatted, number);             // Добавляем индекс
        strcat(formatted, " ");                // Добавляем пробел

        index++;  // Увеличиваем индекс

        
    }

    // Печатаем результат
    printf("Результат: %s\n", formatted);

    close(read_fd);
}




int main() {
    // Название файла, в котором будет текст
    const char *filename = "input.txt";

    // Создание файла с текстом
    create_text(filename);

    // Создание pipe-ов для межпроцессного взаимодействия
    int pipe1[2], pipe2[2];
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        // Первый дочерний процесс (child_process1)
        close(pipe1[1]); // Закрываем неиспользуемую сторону
        close(pipe2[0]); // Закрываем неиспользуемую сторону

        // Читаем файл и добавляем индексы
        child_process1(pipe1[0], pipe2[1]);

        exit(0);
    } else {
        pid_t pid2 = fork();
        if (pid2 == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid2 == 0) {
            // Второй дочерний процесс (child_process2)
            close(pipe1[0]); // Закрываем неиспользуемую сторону
            close(pipe2[1]); // Закрываем неиспользуемую сторону

            // Обрабатываем текст и выводим его
            child_process2(pipe2[0]);

            exit(0);
        } else {
            // Родительский процесс: читает файл и отправляет данные в первый pipe
            close(pipe1[0]); // Закрываем неиспользуемую сторону
            close(pipe2[0]); // Закрываем неиспользуемую сторону
            close(pipe2[1]); // Закрываем неиспользуемую сторону

            // Открытие файла для чтения
            FILE *file = fopen(filename, "r");
            if (file == NULL) {
                perror("File open failed");
                exit(EXIT_FAILURE);
            }

            char buffer[BUFFER_SIZE];
            while (fgets(buffer, sizeof(buffer), file)) {
                write(pipe1[1], buffer, strlen(buffer));
            }

            fclose(file);
            close(pipe1[1]); // Закрываем после записи

            // Ожидаем завершения процессов
            wait(NULL);
            wait(NULL);
        }
    }

    return 0;
}
