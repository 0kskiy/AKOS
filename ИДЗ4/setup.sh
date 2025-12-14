#!/bin/bash
# Скрипт настройки проекта "Остров Сокровищ" (C++)
# Создает необходимые файлы и директории

echo "==================================================================="
echo "НАСТРОЙКА ПРОЕКТА: Задача об Острове Сокровищ (C++)"
echo "Вариант 32"
echo "==================================================================="
echo ""

# Создание конфигурационных файлов
echo "Создание конфигурационных файлов..."

cat > config1.txt << EOF
15 4
EOF
echo "✓ config1.txt создан (15 участков, 4 группы)"

cat > config2.txt << EOF
30 5
EOF
echo "✓ config2.txt создан (30 участков, 5 групп)"

cat > config3.txt << EOF
50 8
EOF
echo "✓ config3.txt создан (50 участков, 8 групп)"

echo ""

# Создание директории для результатов
echo "Создание директорий..."
mkdir -p comparison_results
echo "✓ Директория comparison_results создана"
echo ""

# Установка прав на выполнение
echo "Установка прав на выполнение скриптов..."
chmod +x compare.sh 2>/dev/null || echo "compare.sh не найден - будет создан позже"
chmod +x setup.sh
echo "✓ Права установлены"
echo ""

# Проверка компилятора C++
echo "Проверка наличия компилятора C++..."
if command -v g++ &> /dev/null; then
    gpp_version=$(g++ --version | head -n 1)
    echo "✓ G++ найден: $gpp_version"
    
    # Проверка поддержки C++17
    echo "  Проверка поддержки C++17..."
    cat > test_cpp17.cpp << 'EOF'
#include <iostream>
int main() {
    if constexpr (true) {
        std::cout << "C++17 supported" << std::endl;
    }
    return 0;
}
EOF
    if g++ -std=c++17 test_cpp17.cpp -o test_cpp17 &> /dev/null; then
        echo "  ✓ C++17 поддерживается"
        rm -f test_cpp17.cpp test_cpp17
    else
        echo "  ✗ C++17 НЕ поддерживается!"
        echo "    Обновите компилятор: sudo apt-get install g++-7 или новее"
        rm -f test_cpp17.cpp
    fi
else
    echo "✗ G++ не найден!"
    echo "  Установите: sudo apt-get install g++"
fi
echo ""

# Проверка make
echo "Проверка наличия make..."
if command -v make &> /dev/null; then
    make_version=$(make --version | head -n 1)
    echo "✓ Make найден: $make_version"
else
    echo "✗ Make не найден!"
    echo "  Установите: sudo apt-get install build-essential"
fi
echo ""

# Проверка наличия исходных файлов
echo "Проверка наличия исходных файлов..."
files_ok=true

if [ -f "treasure_semaphore.cpp" ]; then
    echo "✓ treasure_semaphore.cpp найден"
else
    echo "✗ treasure_semaphore.cpp НЕ НАЙДЕН"
    files_ok=false
fi

if [ -f "treasure_condvar.cpp" ]; then
    echo "✓ treasure_condvar.cpp найден"
else
    echo "✗ treasure_condvar.cpp НЕ НАЙДЕН"
    files_ok=false
fi

if [ -f "Makefile" ]; then
    echo "✓ Makefile найден"
else
    echo "✗ Makefile НЕ НАЙДЕН"
    files_ok=false
fi

echo ""

# Попытка сборки
if [ "$files_ok" = true ]; then
    echo "Попытка сборки проекта..."
    if make clean && make all; then
        echo ""
        echo "✓ Проект успешно собран!"
        echo ""
        echo "Доступные исполняемые файлы:"
        ls -lh treasure_semaphore treasure_condvar 2>/dev/null || echo "Файлы не найдены"
    else
        echo ""
        echo "✗ Ошибка при сборке проекта"
        echo "  Проверьте исходные файлы и компилятор"
    fi
else
    echo "⚠ Не все исходные файлы найдены. Сборка пропущена."
fi

echo ""
echo "==================================================================="
echo "НАСТРОЙКА ЗАВЕРШЕНА"
echo "==================================================================="
echo ""
echo "Следующие шаги:"
echo ""
echo "1. Если сборка прошла успешно, запустите тесты:"
echo "   ./treasure_semaphore 20 5"
echo "   ./treasure_condvar 20 5"
echo ""
echo "2. Для сравнительного анализа:"
echo "   ./compare.sh"
echo ""
echo "3. Для запуска с конфигурационным файлом:"
echo "   ./treasure_semaphore -c config1.txt -o output.txt"
echo ""
echo "4. Для просмотра справки:"
echo "   make help"
echo ""
echo "==================================================================="