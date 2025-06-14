#include "metrics_library.h"
#include <thread>
#include <chrono>
#include <random>
#include <iostream>
#include <vector>

// Функция для имитации утилизации CPU (Gauge)
void simulateCpuUsage(std::shared_ptr<Gauge> cpuMetric, int durationSeconds) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 8.0); // Имитация загрузки до 8 ядер

    for (int i = 0; i < durationSeconds; ++i) {
        try {
            double cpuLoad = dis(gen);
            cpuMetric->update(cpuLoad);
            Logger::getInstance().logInfo("CPU usage simulated: " + std::to_string(cpuLoad));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (const std::exception& e) {
            Logger::getInstance().logError("Error in simulateCpuUsage: " + std::string(e.what()));
        }
    }
}

// Функция для имитации использования памяти (Gauge)
void simulateMemoryUsage(std::shared_ptr<Gauge> memoryMetric, int durationSeconds) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 16.0); // Имитация использования памяти в ГБ

    for (int i = 0; i < durationSeconds; ++i) {
        try {
            double memoryLoad = dis(gen);
            memoryMetric->update(memoryLoad);
            Logger::getInstance().logInfo("Memory usage simulated: " + std::to_string(memoryLoad));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (const std::exception& e) {
            Logger::getInstance().logError("Error in simulateMemoryUsage: " + std::string(e.what()));
        }
    }
}

// Функция для имитации HTTP-запросов (Counter)
void simulateHttpRequests(std::shared_ptr<Counter> httpMetric, int durationSeconds) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 150); // Случайное количество запросов

    for (int i = 0; i < durationSeconds; ++i) {
        try {
            int requests = dis(gen);
            httpMetric->increment(requests);
            Logger::getInstance().logInfo("HTTP requests simulated: " + std::to_string(requests));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (const std::exception& e) {
            Logger::getInstance().logError("Error in simulateHttpRequests: " + std::string(e.what()));
        }
    }
}

// Функция для имитации ошибок сервера (Counter)
void simulateServerErrors(std::shared_ptr<Counter> errorMetric, int durationSeconds) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 5); // Случайное количество ошибок

    for (int i = 0; i < durationSeconds; ++i) {
        try {
            int errors = dis(gen);
            errorMetric->increment(errors);
            Logger::getInstance().logInfo("Server errors simulated: " + std::to_string(errors));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (const std::exception& e) {
            Logger::getInstance().logError("Error in simulateServerErrors: " + std::string(e.what()));
        }
    }
}

int main() {
    try {
        // Инициализация сборщика метрик
        MetricsCollector collector("metrics_output.txt");
        Logger::getInstance().logInfo("MetricsCollector initialized with file: metrics_output.txt");

        // Создание метрик
        auto cpuMetric = std::make_shared<Gauge>("CPU_usage");
        auto memoryMetric = std::make_shared<Gauge>("Memory_usage_GB");
        auto httpMetric = std::make_shared<Counter>("HTTP_requests_RPS");
        auto errorMetric = std::make_shared<Counter>("Server_errors");

        // Добавление метрик в сборщик
        collector.addMetric(cpuMetric);
        collector.addMetric(memoryMetric);
        collector.addMetric(httpMetric);
        collector.addMetric(errorMetric);
        Logger::getInstance().logInfo("All metrics added to collector");

        // Запуск симуляции в отдельных потоках
        const int duration = 6; // Длительность симуляции в секундах
        std::vector<std::thread> threads;
        threads.emplace_back(simulateCpuUsage, cpuMetric, duration);
        threads.emplace_back(simulateMemoryUsage, memoryMetric, duration);
        threads.emplace_back(simulateHttpRequests, httpMetric, duration);
        threads.emplace_back(simulateServerErrors, errorMetric, duration);

        // Сбор и запись метрик каждую секунду
        for (int i = 0; i < duration; ++i) {
            collector.collectAndWrite();
            std::cout << "Metrics collected and written at second " << i + 1 << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Ожидание завершения всех потоков
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        // Финальный сбор метрик
        collector.collectAndWrite();
        Logger::getInstance().logInfo("Final metrics collection completed");
        std::cout << "Metrics collection completed, output written to metrics_output.txt" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        Logger::getInstance().logError("Main execution failed: " + std::string(e.what()));
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}