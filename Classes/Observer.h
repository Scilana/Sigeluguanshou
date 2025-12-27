// Observer.h
// Generic Observer Pattern Template
// Provides type-safe event notification system

#ifndef __OBSERVER_H__
#define __OBSERVER_H__

#include <vector>
#include <algorithm>
#include <functional>
#include <memory>

/**
 * @brief 观察者接口模板
 *
 * @tparam EventType 事件数据类型
 *
 * 任何想要监听事件的类都应实现此接口
 */
template<typename EventType>
class IObserver {
public:
    virtual ~IObserver() = default;

    /**
     * @brief 当事件发生时被调用
     * @param event 事件数据
     */
    virtual void onNotify(const EventType& event) = 0;
};

/**
 * @brief 被观察者(Subject)模板类
 *
 * @tparam EventType 事件数据类型
 *
 * 维护观察者列表并通知它们事件
 *
 * 用法示例:
 * @code
 * struct HealthChangedEvent {
 *     int oldHealth;
 *     int newHealth;
 * };
 *
 * class Player : public Observable<HealthChangedEvent> {
 * public:
 *     void takeDamage(int damage) {
 *         int oldHp = hp_;
 *         hp_ -= damage;
 *         notifyObservers({oldHp, hp_});
 *     }
 * };
 *
 * class HealthBar : public IObserver<HealthChangedEvent> {
 * public:
 *     void onNotify(const HealthChangedEvent& event) override {
 *         updateDisplay(event.newHealth);
 *     }
 * };
 * @endcode
 */
template<typename EventType>
class Observable {
public:
    virtual ~Observable() = default;

    /**
     * @brief 添加观察者
     * @param observer 观察者指针
     */
    void addObserver(IObserver<EventType>* observer) {
        if (observer && std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
            observers_.push_back(observer);
        }
    }

    /**
     * @brief 移除观察者
     * @param observer 观察者指针
     */
    void removeObserver(IObserver<EventType>* observer) {
        auto it = std::find(observers_.begin(), observers_.end(), observer);
        if (it != observers_.end()) {
            observers_.erase(it);
        }
    }

    /**
     * @brief 移除所有观察者
     */
    void clearObservers() {
        observers_.clear();
    }

    /**
     * @brief 获取观察者数量
     * @return size_t 观察者数量
     */
    size_t getObserverCount() const {
        return observers_.size();
    }

protected:
    /**
     * @brief 通知所有观察者
     * @param event 事件数据
     */
    void notifyObservers(const EventType& event) {
        // 使用副本遍历，防止在通知过程中修改列表导致迭代器失效
        auto observersCopy = observers_;
        for (auto* observer : observersCopy) {
            if (observer) {
                try {
                    observer->onNotify(event);
                } catch (const std::exception& e) {
                    // 捕获观察者中的异常，防止影响其他观察者
                    // 在实际项目中应该记录日志
                    (void)e; // Suppress unused variable warning
                }
            }
        }
    }

private:
    std::vector<IObserver<EventType>*> observers_;
};

/**
 * @brief 函数式观察者模板
 *
 * @tparam EventType 事件数据类型
 *
 * 允许使用lambda或函数对象作为观察者
 */
template<typename EventType>
class FunctionObserver : public IObserver<EventType> {
public:
    using CallbackType = std::function<void(const EventType&)>;

    explicit FunctionObserver(CallbackType callback)
        : callback_(std::move(callback)) {}

    void onNotify(const EventType& event) override {
        if (callback_) {
            callback_(event);
        }
    }

private:
    CallbackType callback_;
};

#endif // __OBSERVER_H__
