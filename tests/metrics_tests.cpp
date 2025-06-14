#include "metrics_library.h"
#include "metrics_tests.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <thread>

// Макрос для проверки условий с выводом сообщений
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "ASSERT FAILED: " << message << " in " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

// Подготовка перед тестом
bool setup_test_environment(const std::string& test_filename) {
    std::remove(test_filename.c_str());
    return true;
}

// Очистка после теста
bool teardown_test_environment(const std::string& test_filename) {
    std::remove(test_filename.c_str());
    return true;
}

// Тест Gauge: update, getValueAsString, reset
bool test_gauge() {
    Gauge g("test_gauge");
    g.update(42.57);
    TEST_ASSERT(g.getValueAsString() == "42.57", "Invalid value after update()");
    g.reset();
    TEST_ASSERT(g.getValueAsString() == "0.00", "Invalid value after reset()");
    return true;
}

// Тест Counter: increment, getValueAsString, reset
bool test_counter() {
    Counter c("test_counter");
    c.increment(10);
    TEST_ASSERT(c.getValueAsString() == "10", "Invalid value after increment()");
    c.reset();
    TEST_ASSERT(c.getValueAsString() == "0", "Invalid value after reset()");
    return true;
}

// Тест MetricsCollector: addMetric, collectAndWrite
bool test_metrics_collector() {
    const std::string test_filename = "test_metrics.txt";
    setup_test_environment(test_filename);

    auto gauge = std::make_shared<Gauge>("test_gauge_collector");
    auto counter = std::make_shared<Counter>("test_counter_collector");

    {
        MetricsCollector collector(test_filename);
        gauge->update(123.45);
        counter->increment(7);
        collector.addMetric(gauge);
        collector.addMetric(counter);
        collector.collectAndWrite();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::ifstream file(test_filename);
    TEST_ASSERT(file.is_open(), "Failed to open file: " + test_filename);
    
    std::string line;
    bool found_gauge = false, found_counter = false;
    while (std::getline(file, line)) {
        if (line.find("test_gauge_collector") != std::string::npos && line.find("123.45") != std::string::npos) {
            found_gauge = true;
        }
        if (line.find("test_counter_collector") != std::string::npos && line.find("7") != std::string::npos) {
            found_counter = true;
        }
    }
    file.close();
    
    TEST_ASSERT(found_gauge, "Gauge metric not found in file");
    TEST_ASSERT(found_counter, "Counter metric not found in file");
    TEST_ASSERT(gauge->getValueAsString() == "0.00", "Gauge was not reset");
    TEST_ASSERT(counter->getValueAsString() == "0", "Counter was not reset");
    
    teardown_test_environment(test_filename);
    return true;
}

// Структура для хранения информации о тесте
struct TestInfo {
    const char* name;
    bool (*test_func)();
};

// Массив всех тестов
static const TestInfo TESTS[] = {
    {"test_gauge", test_gauge},
    {"test_counter", test_counter},
    {"test_metrics_collector", test_metrics_collector}
    // Новые тесты добавляются сюда
};

// Запуск всех тестов
int run_all_tests() {
    int failed = 0;
    int total = sizeof(TESTS) / sizeof(TESTS[0]);
    
    std::cout << "Running " << total << " tests...\n";
    
    for (const auto& test : TESTS) {
        std::cout << "Running " << test.name << "... ";
        bool result = test.test_func();
        if (result) {
            std::cout << "OK\n";
        } else {
            std::cout << "FAILED\n";
            ++failed;
        }
    }
    
    std::cout << "\nResults: " << (total - failed) << " passed, " << failed << " failed\n";
    return failed;
}

// main для запуска тестов
int main() {
    int failed = run_all_tests();
    if (failed == 0) {
        std::cout << "All tests passed!\n";
        return 0;
    } else {
        std::cout << failed << " test(s) failed!\n";
        return 1;
    }
}