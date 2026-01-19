import { IModule } from "../Interface/IModule";
import EventDispatcher from "../Utils/EventDispatcher";

// 模块基类
/**
 * @class ModuleBase
 * @extends EventDispatcher
 * @implements IModule
 * 
 * @description
 * 框架中所有模块的基类。提供基本的生命周期方法，如初始化、销毁和更新。
 */
export default abstract class ModuleBase extends EventDispatcher implements IModule {
    /**
     * 初始化模块。
     * 
     * @remarks
     * 应该在使用模块之前调用此方法进行设置。
     * 在控制台记录 "模块已初始化"。
     */
    Initialize(): void {
        console.log("模块已初始化");
        // 实现代码
    }

    Start(): void {
        // 实现代码
        console.log("模块已开始");
    }

    /**
     * 销毁模块。
     * 
     * @remarks
     * 当不再需要模块时，应该调用此方法进行清理。
     * 在控制台记录 "模块已销毁"。
     */
    Destroy(): void {
        console.log("模块已销毁");
        this.RemoveAllEventListeners();
        // 实现代码
    }

}
