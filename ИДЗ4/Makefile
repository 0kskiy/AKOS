# Makefile для задачи "Остров Сокровищ" (C++)
# Вариант 32

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread
LDFLAGS = -pthread

# Целевые файлы
TARGETS = treasure_semaphore treasure_condvar

# Исходные файлы
SRC_SEM = treasure_semaphore.cpp
SRC_COND = treasure_condvar.cpp

.PHONY: all clean test

all: $(TARGETS)

treasure_semaphore: $(SRC_SEM)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

treasure_condvar: $(SRC_COND)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Очистка
clean:
	rm -f $(TARGETS)
	rm -f *.o
	rm -f output_*.txt

# Тестовые запуски
test: all
	@echo "=== Тест 1: Семафоры (10 участков, 3 группы) ==="
	./treasure_semaphore 10 3 -o output_sem_test1.txt
	@echo ""
	@echo "=== Тест 2: Условные переменные (10 участков, 3 группы) ==="
	./treasure_condvar 10 3 -o output_cond_test1.txt
	@echo ""
	@echo "=== Тест 3: Семафоры из конфига ==="
	./treasure_semaphore -c config1.txt -o output_sem_config1.txt
	@echo ""
	@echo "=== Тест 4: Условные переменные из конфига ==="
	./treasure_condvar -c config1.txt -o output_cond_config1.txt

# Справка
help:
	@echo "Доступные цели:"
	@echo "  all     - Собрать все программы"
	@echo "  clean   - Удалить исполняемые файлы и выходные данные"
	@echo "  test    - Запустить тесты"
	@echo "  help    - Показать эту справку"
	@echo ""
	@echo "Использование программ:"
	@echo "  ./treasure_semaphore <участки> <группы> [-o output.txt]"
	@echo "  ./treasure_semaphore -c config.txt [-o output.txt]"
	@echo "  ./treasure_condvar <участки> <группы> [-o output.txt]"
	@echo "  ./treasure_condvar -c config.txt [-o output.txt]"