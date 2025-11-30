#define _POSIX_C_SOURCE 200809L
// --- PIRATE ---
// Процесс-пират: получает задания, ищет клад, отправляет отчёты наблюдателям
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <mqueue.h>
#include <signal.h>

#define MAX_OBSERVERS 5
const char *SHM_NAME = "/treasure_shm";
const char *SEM_MUTEX_NAME = "/treasure_sem_mutex";
const char *SEM_TASKS_NAME = "/treasure_sem_tasks";
const char *MQ_NAME_BASE = "/mq_reports_cpp_";

// Структура общей памяти (shared memory), совпадает с manager.cpp
struct SharedData {
    int M;                // Количество участков
    int treasureIndex;    // Индекс участка с кладом
    bool stop;            // Флаг завершения поиска
    int head;
    int tail;
    int qsize;
    int tasks[100];       // Очередь заданий (индексы участков)
};

volatile sig_atomic_t keep_running = 1;
// Обработка сигналов для корректного завершения
void sig_handler(int){ keep_running = 0; }

int main(int argc, char **argv) {
    // Проверяем аргументы: нужен id пирата
    if (argc < 2) { fprintf(stderr, "Usage: %s <pirate_id>\n", argv[0]); return 1; }
    int id = atoi(argv[1]);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // Подключаемся к общей памяти
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm_fd < 0) { perror("[pirate] shm_open"); return 1; }
    SharedData *data = (SharedData*) mmap(nullptr, sizeof(SharedData),
        PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) { perror("[pirate] mmap"); return 1; }

    // Открываем семафоры для синхронизации
    sem_t *sem_mutex = sem_open(SEM_MUTEX_NAME, 0);
    sem_t *sem_tasks = sem_open(SEM_TASKS_NAME, 0);
    if (sem_mutex == SEM_FAILED || sem_tasks == SEM_FAILED) { perror("[pirate] sem_open"); return 1; }

    // Открываем очереди сообщений для всех observer (broadcast)
    int N_observers = MAX_OBSERVERS;
    mqd_t mqs[MAX_OBSERVERS];
    char mqname[64];
    for (int i = 0; i < N_observers; ++i) {
        snprintf(mqname, sizeof(mqname), "%s%d", MQ_NAME_BASE, i+1);
        mqs[i] = mq_open(mqname, O_WRONLY | O_NONBLOCK);
        if (mqs[i] == (mqd_t)-1) mqs[i] = -1;
    }

    // Инициализация генератора случайных чисел
    srand(time(nullptr) ^ (getpid()<<8));
    printf("[pirate %d pid %d] started\n", id, getpid());

    while (keep_running) {
        // Ждём, когда появится задача (участок для поиска)
        sem_wait(sem_tasks);

        // Берём задачу из очереди (tail), защищаем доступ мьютексом
        sem_wait(sem_mutex);
        if (data->stop) { sem_post(sem_mutex); break; }
        int task_idx = data->tasks[data->tail];
        data->tail = (data->tail + 1) % data->qsize;
        sem_post(sem_mutex);

        // Имитируем поиск: задержка от 100 до 900 мс
        int ms = 100 + rand() % 800;
        usleep(ms * 1000);

        // Проверяем, найден ли клад
        sem_wait(sem_mutex);
        if (task_idx == data->treasureIndex && !data->stop) {
            data->stop = true;
            sem_post(sem_mutex);

            // Сообщаем всем observer о находке клада
            char msg[256];
            snprintf(msg, sizeof(msg), "PIRATE %d (pid %d) FOUND TREASURE at %d (%d ms)", id, getpid(), task_idx, ms);
            for (int i = 0; i < N_observers; ++i) {
                if (mqs[i] != -1) mq_send(mqs[i], msg, strlen(msg), 0);
            }
            printf("[pirate %d] FOUND TREASURE at %d\n", id, task_idx);
            break;
        } else {
            sem_post(sem_mutex);
            // Сообщаем о неудачной попытке
            char msg[256];
            snprintf(msg, sizeof(msg), "PIRATE %d (pid %d) searched %d (%d ms)", id, getpid(), task_idx, ms);
            for (int i = 0; i < N_observers; ++i) {
                if (mqs[i] != -1) mq_send(mqs[i], msg, strlen(msg), 0);
            }
            printf("[pirate %d] searched %d (no treasure)\n", id, task_idx);
        }
    }

    // Завершаем работу, освобождаем ресурсы
    printf("[pirate %d] exiting\n", id);
    for (int i = 0; i < N_observers; ++i) {
        if (mqs[i] != -1) mq_close(mqs[i]);
    }
    sem_close(sem_mutex);
    sem_close(sem_tasks);
    munmap(data, sizeof(SharedData));
    return 0;
}
