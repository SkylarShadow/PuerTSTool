/**
 * 模块接口
 * 
 * 定义了一个模块的基本操作方法。
 */
export interface IModule {
    /**
     * 初始化模块。
     */
    Initialize(): void;

    /**
     * 开始模块。
     */
    Start(): void;

    /**
     * 销毁模块。
     */
    Destroy(): void;

}