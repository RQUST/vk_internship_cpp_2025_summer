#include "metrics_library.h"
#include <thread>
#include <chrono>
#include <random>

// Пример функции, имитирующей сбор данных об утилизации CPU
void simulateCpuUsage(std::shared_ptr<Gauge> cpuMetric, int durationSeconds) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 4.0); // Имитация загрузки до 4 ядер

    for (int i = 0; i < durationSeconds; ++i) {
        double cpuLoad = dis(gen); // Генерация случайной загрузки CPU
        cpuMetric->update(cpuLoad);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// Пример функции, имитирующей подсчёт HTTP-запросов
void simulateHttpRequests(std::shared_ptr<Counter> httpMetric, int durationSeconds) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100); // Случайное количество запросов

    for (int i = 0; i < durationSeconds; ++i) {
        int requests = dis(gen); // Генерация случайного числа запросов
        httpMetric->increment(requests);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    // Создание сборщика метрик с файлом для записи
    MetricsCollector collector("metrics_output.txt");

    // Создание метрик
    auto cpuMetric = std::make_shared<Gauge>("CPU");
    auto httpMetric = std::make_shared<Counter>("HTTP_requests_RPS");

    // Добавление метрик в сборщик
    collector.addMetric(cpuMetric);
    collector.addMetric(httpMetric);

    // Запуск имитации в отдельных потоках
    std::thread cpuThread(simulateCpuUsage, cpuMetric, 5);
    std::thread httpThread(simulateHttpRequests, httpMetric, 5);

    // Сбор и запись метрик каждую секунду
    for (int i = 0; i < 5; ++i) {
        collector.collectAndWrite();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Ожидание завершения потоков
    cpuThread.join();
    httpThread.join();

    // Финальный сбор метрик
    collector.collectAndWrite();

    Logger::getInstance().logInfo("Example completed, metrics written to metrics_output.txt");
    return 0;
}