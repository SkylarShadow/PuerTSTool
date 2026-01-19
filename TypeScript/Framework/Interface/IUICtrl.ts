import * as UE from 'ue';

/**
 * UI控制接口
 * 
 */
export interface IUICtrl {

    /**
     * 获取UI元素的名称。
     */
    get Name(): string;

    /**
     * 获取UI元素的路径。
     */
    get Path(): string;

    /**
     * 获取UI蓝图对象。
     */
    get UIBP(): UE.UserWidget;

    /**
     * 初始化UI元素。
     */
    Initialize(): void;

    /**
     * 显示UI元素。
     */
    Show(): void;

    /**
     * 隐藏UI元素。
     */
    Hide(): void;

}
