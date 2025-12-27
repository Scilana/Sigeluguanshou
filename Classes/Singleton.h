// Singleton.h
// Generic Singleton Template for Game Managers
// Provides thread-safe singleton pattern with RAII

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <memory>
#include <mutex>

/**
 * @brief 单例模板基类
 *
 * 使用CRTP (Curiously Recurring Template Pattern) 实现通用单例
 * 线程安全，使用智能指针管理内存
 *
 * 用法示例:
 * @code
 * class MyManager : public Singleton<MyManager> {
 *     friend class Singleton<MyManager>;
 * private:
 *     MyManager() = default;
 * public:
 *     void doSomething();
 * };
 *
 * // 使用:
 * MyManager::getInstance().doSomething();
 * @endcode
 */
template<typename T>
class Singleton {
public:
    // 禁止拷贝和赋值
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    /**
     * @brief 获取单例实例
     * @return T& 单例引用
     *
     * 线程安全的懒汉式单例实现
     */
    static T& getInstance() {
        std::call_once(init_flag_, &Singleton::initSingleton);
        return *instance_;
    }

    /**
     * @brief 销毁单例实例
     *
     * 显式销毁单例，释放资源
     * 主要用于程序退出时的清理
     */
    static void destroyInstance() {
        std::lock_guard<std::mutex> lock(mutex_);
        instance_.reset();
    }

    /**
     * @brief 检查单例是否已初始化
     * @return bool 是否已初始化
     */
    static bool isInitialized() {
        std::lock_guard<std::mutex> lock(mutex_);
        return instance_ != nullptr;
    }

protected:
    Singleton() = default;
    virtual ~Singleton() = default;

private:
    static void initSingleton() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!instance_) {
            instance_ = std::unique_ptr<T>(new T());
        }
    }

    static std::unique_ptr<T> instance_;
    static std::mutex mutex_;
    static std::once_flag init_flag_;
};

// 静态成员定义
template<typename T>
std::unique_ptr<T> Singleton<T>::instance_ = nullptr;

template<typename T>
std::mutex Singleton<T>::mutex_;

template<typename T>
std::once_flag Singleton<T>::init_flag_;

#endif // __SINGLETON_H__
