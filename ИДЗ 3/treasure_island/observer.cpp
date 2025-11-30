#define _POSIX_C_SOURCE 200809L
// --- OBSERVER ---
// Процесс-наблюдатель: получает все сообщения от пиратов и выводит их в консоль
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>
#include <cerrno>

const char *MQ_NAME_BASE = "/mq_reports_cpp_"; // Базовое имя очереди сообщений
volatile sig_atomic_t keep_running = 1;
// Обработка сигналов для корректного завершения
void sig_handler(int){ keep_running = 0; }

int main(int argc, char* argv[]) {
    // Проверяем аргумент: id observer (номер очереди)
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <observer_id>\n", argv[0]);
        return 1;
    }
    int observer_id = atoi(argv[1]);
    if (observer_id < 1 || observer_id > 5) {
        fprintf(stderr, "observer_id must be 1..5\n");
        return 1;
    }
    // Устанавливаем обработчики сигналов
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // Открываем свою очередь сообщений (broadcast)
    char mqname[64];
    snprintf(mqname, sizeof(mqname), "%s%d", MQ_NAME_BASE, observer_id);
    mqd_t mq = mq_open(mqname, O_RDONLY | O_NONBLOCK);
    if (mq == (mqd_t)-1) {
        perror("[observer] mq_open");
        fprintf(stderr, "[observer] ensure manager was started first\n");
        return 1;
    }

    printf("[observer %d pid %d] listening...\n", observer_id, getpid());
    char buf[256];
    // Основной цикл: читаем сообщения и выводим их
    while (keep_running) {
        ssize_t r = mq_receive(mq, buf, sizeof(buf), nullptr);
        if (r >= 0) { buf[r] = '\0'; printf("%s\n", buf); fflush(stdout); }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) usleep(150*1000);
        else { perror("[observer] mq_receive"); break; }
    }
    mq_close(mq);
    printf("[observer %d] exiting\n", observer_id);
    return 0;
}
