/**
 * 实现单例设计模式的接口。
 * 
 * @template T - 单例实例的类型。
 */
export interface ISingleton<T> {
    /**
     * 获取单例实例。
     * 
     * @returns T - 返回单例实例。
     */
    Initialize(): void;
    
    /**
     * 销毁单例实例。
     */
    Destroy(): void;
}