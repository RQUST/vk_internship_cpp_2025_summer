#pragma once

#include "logger.h"

#include <string>
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <ctime>

/*
    Базовый абстрактный класс для всех метрик,
    определяющий общий интерфейс для разных типов метрик.
*/
class Metric
{
public:
    // Виртуальный деструктор для корректного освобождения ресурсов производных классов.
    virtual ~Metric() = default;

    // Чисто виртуальная функция для получения имени метрики.
    // Возвращает строку, представляющую идентификатор метрики.
    virtual std::string getName() const = 0;

    // Чисто виртуальная функция для получения текущего значения метрики в виде строки.
    // Обеспечивает единый формат вывода для всех метрик.
    virtual std::string getValueAsString() const = 0;

    // Чисто виртуальная функция для сброса значения метрики.
    // Используется для инициализации значения метрики перед новым сбором.
    virtual void reset() = 0;
};

/*
    Класс метрики типа Gauge для хранения вещественных значений (например, загрузка CPU или память).
*/
class Gauge : public Metric
{
public:
    // Конструктор, инициализирующий имя метрики и начальное значение 0.0.
    Gauge(const std::string &name);

    // Обновляет значение метрики новым вещественным числом.
    void update(double value);

    // Возвращает имя метрики.
    std::string getName() const override;

    // Возвращает текущее значение метрики в виде строки с двумя знаками после запятой.
    std::string getValueAsString() const override;

    // Сбрасывает значение метрики до 0.0.
    void reset() override;

private:
    std::string name_;         // Имя метрики.
    double value_;             // Текущее значение метрики.
    mutable std::mutex mutex_; // Мьютекс для обеспечения потокобезопасности при доступе к значению.
};

/*
   Класс метрики типа Counter для хранения целочисленных значений (например, количество запросов)
*/
class Counter : public Metric
{
public:
    // Конструктор, инициализирующий имя метрики и начальное значение 0.
    Counter(const std::string &name);

    // Увеличивает значение счетчика на указанную величину (по умолчанию на 1).
    void increment(int value = 1);

    // Возвращает имя метрики.
    std::string getName() const override;

    // Возвращает текущее значение счетчика в виде строки.
    std::string getValueAsString() const override;

    // Сбрасывает значение счетчика до 0.
    void reset() override;

private:
    std::string name_;         // Имя метрики.
    int value_;                // Текущее значение счетчика.
    mutable std::mutex mutex_; // Мьютекс для обеспечения потокобезопасности при доступе к значению.
};

/*
    Потокобезопасная очередь для передачи данных метрик между потоками.
*/
class ThreadSafeQueue
{
public:
    // Добавляет данные (вектор пар имя-значение) в очередь.
    void push(const std::vector<std::pair<std::string, std::string>> &data);

    // Пытается извлечь данные из очереди без ожидания. Возвращает false, если очередь пуста.
    bool tryPop(std::vector<std::pair<std::string, std::string>> &data);

    // Ожидает, пока в очереди не появятся данные, и извлекает их.
    void waitAndPop(std::vector<std::pair<std::string, std::string>> &data);

    // Останавливает очередь, уведомляя все ожидающие потоки.
    void stop();

private:
    std::queue<std::vector<std::pair<std::string, std::string>>> queue_; // Очередь для хранения данных метрик.
    std::mutex mutex_;                                                   // Мьютекс для синхронизации доступа к очереди.
    std::condition_variable cond_var_;                                   // Условная переменная для уведомления потоков о новых данных.
    std::atomic<bool> stopped_{false};                                   // Флаг, указывающий, что очередь остановлена.
};

/*
     Класс для асинхронной записи метрик в файл.
*/
class MetricsWriter
{
public:
    // Конструктор, принимающий имя файла для записи метрик и запускающий поток записи.
    MetricsWriter(const std::string &filename);

    // Деструктор, останавливающий поток записи и освобождающий ресурсы.
    ~MetricsWriter();

    // Добавляет метрики (вектор пар имя-значение) в очередь для записи в файл.
    void write(const std::vector<std::pair<std::string, std::string>> &metrics);

private:
    // Метод, выполняемый в отдельном потоке, для чтения метрик из очереди и записи в файл.
    void run();

    std::string filename_;      // Имя файла для записи метрик.
    ThreadSafeQueue queue_;     // Очередь для асинхронной обработки метрик.
    std::thread writer_thread_; // Поток, выполняющий запись в файл.
    std::atomic<bool> running_; // Флаг, указывающий, что поток записи активен.
};

/*
    Класс для управления сбором и записью метрик.
*/
class MetricsCollector
{
public:
    // Конструктор, инициализирующий сборщик с указанным файлом для записи метрик.
    MetricsCollector(const std::string &filename);

    // Добавляет метрику в список для последующего сбора.
    void addMetric(std::shared_ptr<Metric> metric);

    // Собирает текущие значения всех метрик и отправляет их на запись в файл.
    void collectAndWrite();

private:
    std::vector<std::shared_ptr<Metric>> metrics_; // Список зарегистрированных метрик.
    MetricsWriter writer_;                         // Объект для записи метрик в файл.
    std::mutex mutex_;                             // Мьютекс для синхронизации доступа к списку метрик.
};