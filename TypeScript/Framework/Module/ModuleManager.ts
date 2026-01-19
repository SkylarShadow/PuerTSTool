
// ModuleManager.ts
import { IModule } from "../Interface/IModule";
import { ISingleton } from "../Interface/ISingleton";

/**
 * ModuleManager 类是一个单例类，用于管理模块的生命周期。
 * 它提供了初始化、销毁、注册、注销和检索模块的方法。
 * 
 * @implements {ISingleton<ModuleManager>}
 */
export default class ModuleManager implements ISingleton<ModuleManager> {
    private static instance: ModuleManager;
    private modules: Map<string, IModule> = new Map();

    /**
     * 私有构造函数，防止直接实例化。
     */
    private constructor() {}

    /**
     * 初始化 ModuleManager 的单例实例。
     */
    Initialize(): void {
        
    }

    /**
     * 通过调用所有已注册模块的 destroy 方法来销毁它们，并清空模块映射。
     */
    Destroy(): void {
        this.modules.forEach((module) => {
            module.Destroy();
        });
        this.modules.clear();
    }

    /**
     * 返回 ModuleManager 的单例实例。
     * 如果实例不存在，则创建一个新的实例。
     * 
     * @returns {ModuleManager} ModuleManager 的单例实例。
     */
    public static GetInstance(): ModuleManager {
        if (!ModuleManager.instance) {
            ModuleManager.instance = new ModuleManager();
        }
        return ModuleManager.instance;
    }

    /**
     * 启动所有已注册模块。
     */
    public StartModule(): void {
        // 实现代码
        this.modules.forEach((module) => {
            module.Start();
        });
    }

    /**
     * 注册一个具有给定名称的模块并初始化它。
     * 如果模块已注册，则不执行任何操作。
     * 
     * @param {string} name - 模块的名称。
     * @param {IModule} module - 要注册的模块。
     */
    public RegisterModule(name: string, module: IModule): void {
        if (!this.modules.has(name)) {
            this.modules.set(name, module);
            module.Initialize();
        }
    }

    /**
     * 注销一个具有给定名称的模块并销毁它。
     * 如果模块未注册，则不执行任何操作。
     * 
     * @param {string} name - 模块的名称。
     */
    public UnregisterModule(name: string): void {
        const module = this.modules.get(name);
        if (module) {
            module.Destroy();
            this.modules.delete(name);
        }
    }

    /**
     * 通过名称检索模块。
     * 
     * @param {string} name - 模块的名称。
     * @returns {IModule | undefined} 如果找到模块则返回模块，否则返回 undefined。
     */
    public GetModule(name: string): IModule | undefined {
        return this.modules.get(name);
    }
}
