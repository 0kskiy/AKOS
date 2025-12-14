/*
 * treasure_semaphore.cpp
 * Задача об Острове Сокровищ - Решение на семафорах (C++)
 * Автор: Бочкарев Максим Андреевич
 * Группа: БПИ-245-2
 * Вариант: 32
 */

#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <random>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <cstdarg>
#include <condition_variable>

class Semaphore {
private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;

public:
    explicit Semaphore(int initial_count = 0) : count(initial_count) {}

    void wait() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return count > 0; });
        --count;
    }

    void post() {
        std::unique_lock<std::mutex> lock(mtx);
        ++count;
        cv.notify_one();
    }
};

struct Sector {
    int id;
    bool has_treasure;
    bool searched;
    
    Sector(int id_ = 0) 
        : id(id_), has_treasure(false), searched(false) {}
};

struct PirateGroup {
    int group_id;
    int sectors_searched;
    
    PirateGroup(int id) : group_id(id), sectors_searched(0) {}
};

std::vector<Sector> sectors;
int total_sectors = 0;
int num_groups = 0;
int current_sector = 0;
std::atomic<bool> treasure_found{false};
int treasure_sector = -1;

std::mutex portfolio_mutex;
std::mutex report_mutex;
Semaphore silver_sem(0);

std::ofstream output_file;
std::atomic<bool> stop_flag{false};

std::random_device rd;
std::mt19937 gen(rd());

void log_message(const char* format, ...) {
    char buffer[512];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::cout << buffer << std::flush;
    if (output_file.is_open()) {
        output_file << buffer << std::flush;
    }
}

void signal_handler(int signum) {
    log_message("\n[СИСТЕМА] Получен сигнал завершения. Останавливаем работу...\n");
    stop_flag = true;
}

void silver_thread() {
    log_message("\n[СИЛЬВЕР] Джон Сильвер прибыл на остров!\n");
    log_message("[СИЛЬВЕР] Разделил остров на %d участков\n", total_sectors);
    log_message("[СИЛЬВЕР] Сформировал %d поисковых групп\n", num_groups);
    log_message("[СИЛЬВЕР] Клад спрятан на участке %d\n\n", treasure_sector);
    
    int groups_returned = 0;
    
    while (groups_returned < num_groups && !stop_flag) {
        silver_sem.wait();
        
        if (stop_flag) break;
        
        groups_returned++;
        
        std::lock_guard<std::mutex> lock(report_mutex);
        if (treasure_found) {
            log_message("[СИЛЬВЕР] Отлично! Клад найден на участке %d!\n", treasure_sector);
            log_message("[СИЛЬВЕР] Все группы, возвращайтесь на корабль!\n");
        } else if (current_sector >= total_sectors) {
            log_message("[СИЛЬВЕР] Все участки обследованы. Клада нет...\n");
        }
    }
    
    log_message("\n[СИЛЬВЕР] Поиски завершены. Все группы вернулись.\n");
}

void pirate_group_thread(PirateGroup& group) {
    log_message("[ГРУППА %d] Группа пиратов готова к поиску!\n", group.group_id);
    
    while (!treasure_found && !stop_flag) {
        int local_sector;
        
        {
            std::lock_guard<std::mutex> lock(portfolio_mutex);
            
            if (current_sector >= total_sectors || treasure_found) {
                break;
            }
            
            local_sector = current_sector;
            current_sector++;
        }
        
        log_message("[ГРУППА %d] Начинаем обследование участка %d\n", 
                   group.group_id, local_sector);
        
        std::uniform_int_distribution<> dist(1, 3);
        int search_time = dist(gen);
        std::this_thread::sleep_for(std::chrono::seconds(search_time));
        
        {
            std::lock_guard<std::mutex> lock(portfolio_mutex);
            sectors[local_sector].searched = true;
            group.sectors_searched++;
        }
        
        if (sectors[local_sector].has_treasure) {
            treasure_found = true;
            
            std::lock_guard<std::mutex> lock(report_mutex);
            log_message("\n[ГРУППА %d] *** КЛАД НАЙДЕН НА УЧАСТКЕ %d! ***\n\n", 
                       group.group_id, local_sector);
            
            silver_sem.post();
            break;
        } else {
            log_message("[ГРУППА %d] Участок %d обследован. Клада нет.\n", 
                       group.group_id, local_sector);
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(report_mutex);
        log_message("[ГРУППА %d] Возвращаемся к Сильверу. Обследовано участков: %d\n", 
                   group.group_id, group.sectors_searched);
    }
    
    silver_sem.post();
}

bool read_config(const std::string& filename, int& sectors_count, int& groups_count) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка открытия файла конфигурации: " << filename << std::endl;
        return false;
    }
    
    if (!(file >> sectors_count >> groups_count)) {
        std::cerr << "Ошибка чтения данных из файла" << std::endl;
        return false;
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::string output_filename;
    
    if (argc < 2) {
        std::cerr << "Использование:\n";
        std::cerr << "  " << argv[0] << " <участки> <группы> [-o output.txt]\n";
        std::cerr << "  " << argv[0] << " -c config.txt [-o output.txt]\n";
        return 1;
    }
    
    if (std::string(argv[1]) == "-c") {
        if (argc < 3) {
            std::cerr << "Не указан файл конфигурации\n";
            return 1;
        }
        
        if (!read_config(argv[2], total_sectors, num_groups)) {
            return 1;
        }
        
        for (int i = 3; i < argc - 1; i++) {
            if (std::string(argv[i]) == "-o") {
                output_filename = argv[i + 1];
                break;
            }
        }
    } else {
        if (argc < 3) {
            std::cerr << "Недостаточно аргументов\n";
            return 1;
        }
        
        total_sectors = std::atoi(argv[1]);
        num_groups = std::atoi(argv[2]);
        
        for (int i = 3; i < argc - 1; i++) {
            if (std::string(argv[i]) == "-o") {
                output_filename = argv[i + 1];
                break;
            }
        }
    }
    
    if (total_sectors <= 0 || num_groups <= 0) {
        std::cerr << "Некорректные параметры\n";
        return 1;
    }
    
    if (num_groups >= total_sectors) {
        std::cerr << "Количество участков должно превышать количество групп\n";
        return 1;
    }
    
    if (!output_filename.empty()) {
        output_file.open(output_filename);
        if (!output_file.is_open()) {
            std::cerr << "Ошибка открытия файла для вывода: " << output_filename << std::endl;
            return 1;
        }
    }
    
    sectors.resize(total_sectors);
    for (int i = 0; i < total_sectors; i++) {
        sectors[i] = Sector(i);
    }
    
    std::uniform_int_distribution<> sector_dist(0, total_sectors - 1);
    treasure_sector = sector_dist(gen);
    sectors[treasure_sector].has_treasure = true;
    
    log_message("=== ЗАДАЧА ОБ ОСТРОВЕ СОКРОВИЩ ===\n");
    log_message("=== Решение на семафорах (C++) ===\n\n");
    log_message("Параметры:\n");
    log_message("  Количество участков: %d\n", total_sectors);
    log_message("  Количество групп: %d\n", num_groups);
    log_message("\n");
    
    std::thread silver(silver_thread);
    
    std::vector<std::thread> group_threads;
    std::vector<PirateGroup> groups;
    
    for (int i = 0; i < num_groups; i++) {
        groups.emplace_back(i + 1);
    }
    
    for (int i = 0; i < num_groups; i++) {
        group_threads.emplace_back(pirate_group_thread, std::ref(groups[i]));
    }
    
    for (auto& thread : group_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    if (silver.joinable()) {
        silver.join();
    }
    
    log_message("\n=== ИТОГИ ПОИСКА ===\n");
    if (treasure_found) {
        log_message("Результат: КЛАД НАЙДЕН на участке %d\n", treasure_sector);
    } else {
        log_message("Результат: Клад НЕ НАЙДЕН\n");
    }
    log_message("Обследовано участков: %d из %d\n", current_sector, total_sectors);
    
    log_message("\nСтатистика по группам:\n");
    for (const auto& group : groups) {
        log_message("  Группа %d: обследовала %d участков\n", 
                   group.group_id, group.sectors_searched);
    }
    
    if (output_file.is_open()) {
        output_file.close();
    }
    
    return 0;
}