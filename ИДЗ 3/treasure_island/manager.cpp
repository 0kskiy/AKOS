#define _POSIX_C_SOURCE 200809L
// --- MANAGER ---
// Управляющий процесс: делит остров на участки, создаёт IPC, раздаёт задания пиратам
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <mqueue.h>
#include <cstring>
#include <cstdlib>

using namespace std;

const char* SHM_NAME = "/treasure_shm";
const char* SEM_MUTEX_NAME = "/treasure_sem_mutex";
const char* SEM_TASKS_NAME = "/treasure_sem_tasks";
#define MAX_OBSERVERS 5
const char* MQ_NAME_BASE = "/mq_reports_cpp_";

// Структура общей памяти (shared memory), совпадает с pirate.cpp
struct SharedData {
    int M;                // Количество участков
    int treasureIndex;    // Индекс участка с кладом
    bool stop;            // Флаг завершения поиска
    int head;
    int tail;
    int qsize;
    int tasks[100];       // Очередь заданий (индексы участков)
};

SharedData* dataPtr = nullptr;
int shm_fd = -1;
sem_t* sem_mutex = nullptr;
sem_t* sem_tasks = nullptr;
mqd_t mqs[MAX_OBSERVERS];
int N_observers = 1;

// Освобождение всех ресурсов и удаление IPC-объектов
void cleanup() {
    char mqname[64];
    for (int i = 0; i < N_observers; ++i) {
        snprintf(mqname, sizeof(mqname), "%s%d", MQ_NAME_BASE, i+1);
        mq_unlink(mqname);
    }
    if (sem_mutex) { sem_close(sem_mutex); sem_unlink(SEM_MUTEX_NAME); }
    if (sem_tasks) { sem_close(sem_tasks); sem_unlink(SEM_TASKS_NAME); }
    if (dataPtr) { munmap(dataPtr, sizeof(SharedData)); }
    if (shm_fd != -1) { close(shm_fd); shm_unlink(SHM_NAME); }
}

// Обработка сигналов для корректного завершения всей системы
void signal_handler(int) {
    if (dataPtr) {
        sem_wait(sem_mutex);
        dataPtr->stop = true;
        sem_post(sem_mutex);
    }
    cleanup();
    exit(0);
}

int main(int argc, char* argv[]) {
    // Проверяем аргументы: количество участков, наблюдателей и т.д.
    if (argc < 4) { cerr << "Usage: ./manager <M> <G> <N_observers>\n"; return 1; }

    int M = atoi(argv[1]);
    int G = atoi(argv[2]);
    N_observers = atoi(argv[3]);
    if (N_observers < 1 || N_observers > MAX_OBSERVERS) { cerr << "N_observers must be 1.." << MAX_OBSERVERS << "\n"; return 1; }
    (void)G;

    // Устанавливаем обработчики сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Создаём и инициализируем общую память
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (shm_fd == -1) { perror("shm_open"); return 1; }
    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) { perror("ftruncate"); return 1; }
    dataPtr = (SharedData*) mmap(nullptr, sizeof(SharedData),
                                 PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dataPtr == MAP_FAILED) { perror("mmap"); return 1; }

    // Открываем семафоры для синхронизации
    sem_mutex = sem_open(SEM_MUTEX_NAME, O_CREAT, 0644, 1);
    sem_tasks = sem_open(SEM_TASKS_NAME, O_CREAT, 0644, 0);
    if (sem_mutex == SEM_FAILED || sem_tasks == SEM_FAILED) { perror("sem_open"); cleanup(); return 1; }

    // Инициализируем очередь заданий и параметры поиска
    sem_wait(sem_mutex);
    dataPtr->M = M;
    dataPtr->stop = false;
    dataPtr->head = 0;
    dataPtr->tail = 0;
    dataPtr->qsize = M;
    dataPtr->treasureIndex = rand() % M;
    for (int i = 0; i < M; i++) dataPtr->tasks[i] = i;
    sem_post(sem_mutex);

    // Выдаём задачи пиратам через семафор
    for (int i = 0; i < M; ++i) sem_post(sem_tasks);

    // Создаём очереди сообщений для всех observer
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 256;
    attr.mq_curmsgs = 0;
    char mqname[64];
    for (int i = 0; i < N_observers; ++i) {
        snprintf(mqname, sizeof(mqname), "%s%d", MQ_NAME_BASE, i+1);
        mqs[i] = mq_open(mqname, O_CREAT | O_WRONLY, 0644, &attr);
        if (mqs[i] == -1) { perror("mq_open"); cleanup(); return 1; }
    }

    cout << "[manager] Started. M=" << M << ", treasure at index = " << dataPtr->treasureIndex << ", observers = " << N_observers << endl;

    // Основной цикл: ждём, пока кто-то из пиратов найдёт клад (stop=true)
    while (true) {
        sem_wait(sem_mutex);
        bool stop = dataPtr->stop;
        sem_post(sem_mutex);
        if (stop) break;
        usleep(100000);
    }

    cout << "[manager] shutting down...\n";
    cleanup();
    return 0;
}
