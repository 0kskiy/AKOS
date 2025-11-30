#!/bin/bash
set -e

# Очистка IPC-объектов
sudo rm -f /dev/mqueue/mq_reports_cpp_* /dev/shm/treasure_shm /dev/shm/sem.treasure_sem_mutex /dev/shm/sem.treasure_sem_tasks 2>/dev/null || true

# Сборка
make

# Количество observer (можно менять)
N_OBS=3

# Запуск manager с количеством observer
./manager 20 5 $N_OBS &
MANAGER_PID=$!
sleep 0.5

# Запуск observer с разными id
for i in $(seq 1 $N_OBS); do
  ./observer $i &
done

# Запуск 5 пиратов с разными id
for i in {1..5}; do
  ./pirate $i &
done

wait $MANAGER_PID
