#include "timer.h"
#include "util.h"

namespace MNSER {
	
// 取消定时器器
bool Timer::cancel() { // 将 timer 从 管理器中取出即可
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

// 刷新设置定时器的执行时间
bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
	// 在timer管理器中，找到这个timer
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
	// 因为要重置时间，所以必须先将就时间删除，就是移除它
	// 然后重新设置时间，在插入集合中
    m_manager->m_timers.erase(it);
    m_next = MNSER::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

// 重新设置定时器时间，ms 定时器执行间隔时间，from_now 是否从当前时间开始计算
bool Timer::reset(uint64_t ms, bool from_now) {
    if(ms == m_ms && !from_now) { // 如果不是从现在开始，并且间隔相同，直接返回
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now) {
        start = MNSER::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

Timer::Timer(uint64_t ms, std::function<void()> cb,
	bool recurring, TimerManager* manager)
	:m_recurring(recurring), 
	 m_cb(cb),
	 m_ms(ms),
	 m_manager(manager) {
	m_next = MNSER::GetCurrentMS() + m_ms;  // 到下一个时刻应该结束的时间
}

Timer::Timer(uint64_t next) 
    :m_next(next) {
}

bool Timer::Comparator::operator()(const Timer::ptr& lhs,
		const Timer::ptr& rhs) const {
    if(!lhs && !rhs) {
        return false;
    }
    if(!lhs) {
        return true;
    }
    if(!rhs) {
        return false;
    }
    if(lhs->m_next < rhs->m_next) {
        return true;
    }
    if(rhs->m_next < lhs->m_next) {
        return false;
    }
    return lhs.get() < rhs.get();
}

TimerManager::TimerManager() {
    m_previousTime = MNSER::GetCurrentMS();
}

TimerManager::~TimerManager() {
}

// 添加定时器，ms 定时器执行间隔时间，cb 定时器回调函数，recurring 是否循环定时器
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
		bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {
        cb();
    }
}

// 添加条件定时器，ms 定时器执行间隔时间，cb 定时器回调函数
// weak_cond 条件，recurring 是否循环
Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, 
		std::weak_ptr<void> weak_cond, bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

// 到最近一个定时器执行时间间隔 ms 级别
uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()) {
        return ~0ull;
    }

    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = MNSER::GetCurrentMS();
	//std::cout << now_ms << " ### " << next->m_next << std::endl;
    if(now_ms >= next->m_next) {
        return 0;
    } else {
        return next->m_next - now_ms;
    }
}

// 获取需要执行的定时器的回调函数列表
void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) {
    uint64_t now_ms = MNSER::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if(m_timers.empty()) {
            return;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);
    if(m_timers.empty()) {
        return;
    }
    bool rollover = detectClockRollover(now_ms);
    if(!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    while(it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());

    for(auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}

// 是否有定时器
bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

// 检测服务器是否被调后了
bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if(now_ms < m_previousTime &&
            now_ms < (m_previousTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previousTime = now_ms;
    return rollover;
}

// 将定时器添加到管理器中
void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    auto it = m_timers.insert(val).first;
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if(at_front) {
        m_tickled = true;
    }
    lock.unlock();

    if(at_front) {
        onTimerInsertedAtFront();
    }
}

}
