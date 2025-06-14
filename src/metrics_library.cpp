#include "metrics_library.h"
#include <utility>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <stdexcept>

// ================= Gauge =================
Gauge::Gauge(const std::string &name) : name_(name), value_(0.0) {}

void Gauge::update(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = value;
}

std::string Gauge::getName() const {
    return name_;
}

std::string Gauge::getValueAsString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << value_;
    return ss.str();
}

void Gauge::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = 0.0;
}

// ================= Counter =================
Counter::Counter(const std::string &name) : name_(name), value_(0) {}

void Counter::increment(int value) {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ += value;
}

std::string Counter::getName() const {
    return name_;
}

std::string Counter::getValueAsString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::to_string(value_);
}

void Counter::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = 0;
}

// ================= ThreadSafeQueue =================
// Добавляет набор метрик в очередь и уведомляет ожидающий поток
void ThreadSafeQueue::push(const std::vector<std::pair<std::string, std::string>> &data) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(data);
    cond_var_.notify_one();
}

// Пытается извлечь элемент из очереди без ожидания; возвращает true, если удалось
bool ThreadSafeQueue::tryPop(std::vector<std::pair<std::string, std::string>> &data) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    data = std::move(queue_.front());
    queue_.pop();
    return true;
}

// Ожидает появления данных в очереди и извлекает их; если очередь остановлена и пуста — возвращает управление
void ThreadSafeQueue::waitAndPop(std::vector<std::pair<std::string, std::string>> &data) {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.wait(lock, [this]{ return !queue_.empty() || stopped_; });
    if (queue_.empty() && stopped_) {
        return;
    }
    data = std::move(queue_.front());
    queue_.pop();
}

// Останавливает очередь и пробуждает все ожидающие потоки
void ThreadSafeQueue::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
    cond_var_.notify_all();
}

// ================= MetricsWriter =================
MetricsWriter::MetricsWriter(const std::string &filename)
    : filename_(filename), running_(true) {
    writer_thread_ = std::thread(&MetricsWriter::run, this);
}

MetricsWriter::~MetricsWriter() {
    running_ = false;
    queue_.stop();
    if (writer_thread_.joinable()) {
        writer_thread_.join();
    }
}

// Передает набор метрик в очередь на запись
void MetricsWriter::write(const std::vector<std::pair<std::string, std::string>> &metrics) {
    queue_.push(metrics);
}

// Основной цикл записи: извлекает метрики из очереди и сохраняет их в файл с временной меткой
void MetricsWriter::run() {
    std::ofstream file(filename_, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Error opening file: " + filename_);
    }
    while (running_) {
        std::vector<std::pair<std::string, std::string>> metrics;
        queue_.waitAndPop(metrics);
        if (metrics.empty() && queue_.tryPop(metrics)) {
            continue;
        }
        if (!metrics.empty()) {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            auto timer = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&timer), "%Y-%m-%d %H:%M:%S");
            ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            file << ss.str();
            for (const auto &[name, value] : metrics) {
                file << " \"" << name << "\" " << value;
            }
            file << '\n';
            file.flush();
        }
    }
    file.close();
}

// ================= MetricsCollector =================
MetricsCollector::MetricsCollector(const std::string &filename)
    : writer_(filename) {}

// Добавляет новую метрику в коллекцию для последующего сбора
void MetricsCollector::addMetric(std::shared_ptr<Metric> metric) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.push_back(metric);
}

// Собирает значения всех метрик, сбрасывает их и отправляет на запись
void MetricsCollector::collectAndWrite() {
    std::vector<std::pair<std::string, std::string>> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &metric : metrics_) {
            snapshot.emplace_back(metric->getName(), metric->getValueAsString());
            metric->reset();
        }
    }
    writer_.write(snapshot);
}
