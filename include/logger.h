#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>

// Простой класс для логирования ошибок и информационных сообщений
class Logger {
public:
    // Получение единственного экземпляра логгера (паттерн Singleton)
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Запись ошибки в лог-файл
    void logError(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream log_file(log_filename_, std::ios::app);
        if (log_file.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            log_file << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") 
                     << " [ERROR] " << message << std::endl;
            log_file.close();
        }
    }

    // Запись информационного сообщения в лог-файл
    void logInfo(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream log_file(log_filename_, std::ios::app);
        if (log_file.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            log_file << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") 
                     << " [INFO] " << message << std::endl;
            log_file.close();
        }
    }

private:
    // Приватный конструктор для реализации Singleton
    Logger() : log_filename_("metrics.log") {}
    std::string log_filename_;
    std::mutex mutex_;
};