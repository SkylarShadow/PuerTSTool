type EventListener = (...args: any[]) => void; // 事件监听器类型，接受任意数量和类型的参数

// 事件派发器类
class EventDispatcher {
    private listeners: { [event: string]: EventListener[] } = {}; // 存储事件及其对应的监听器数组

    // 添加事件监听器
    public AddEventListener(event: string, listener: EventListener): void {
        if (!this.listeners[event]) {
            this.listeners[event] = [];
        }
        this.listeners[event].push(listener);
    }

    // 移除事件监听器
    public RemoveEventListener(event: string, listener: EventListener): void {
        if (!this.listeners[event]) return;

        const index = this.listeners[event].indexOf(listener);
        if (index !== -1) {
            this.listeners[event].splice(index, 1);
        }
    }

    // 派发事件
    public DispatchEvent(event: string, ...args: any[]): void {
        if (!this.listeners[event]) return;

        for (const listener of this.listeners[event]) {
            listener(...args);
        }
    }

    // 移除指定事件的所有监听器
    public RemoveEventListeners(event: string): void {
        if (!this.listeners[event]) return;

        this.listeners[event] = [];
    }
    
    // 移除所有事件监听器
    public RemoveAllEventListeners(): void {
        this.listeners = {};
    }

    // 判断是否拥有事件监听器
    public HasEventListener(event: string): boolean {
        return !!this.listeners[event];
    }
}

export default EventDispatcher; // 导出事件派发器类