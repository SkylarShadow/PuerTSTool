/**
 * 表示可销毁对象的接口。
 */
export interface IDestroyable {
    /**
     * 销毁对象，释放它持有的任何资源。
     */
    Destroy(): void;
}