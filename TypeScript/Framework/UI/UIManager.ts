import * as UE from 'ue';
import { ISingleton } from "../Interface/ISingleton";
import Misc from "../Misc/Misc";
import EventDefineBase from '../Utils/EventDefineBase';
import EventDispatcher from '../Utils/EventDispatcher';

/**
 * UI管理器。
 * 用于管理UI的创建、销毁和缓存。
 * 
 * @implements {ISingleton<UIManager>}
 */
export default class UIManager extends EventDispatcher implements ISingleton<UIManager> {
    private static instance: UIManager;
    private m_arrCacheUI: UE.UserWidget[];

    private constructor() {
        super();

        this.m_arrCacheUI = [];
    }

    /**
     * 初始化方法
     * 用于初始化UI管理器的逻辑。
     */
    Initialize(): void {
        // 初始化逻辑
    }

    /**
     * 销毁方法
     * 销毁所有UI并清空UI集合。
     */
    Destroy(): void {
        // 销毁所有UI

    }

    /**
     * 获取单例实例
     * 如果实例不存在，则创建一个新的实例。
     * @returns {UIManager} UIManager的单例实例
     */
    public static GetInstance(): UIManager {
        if (!UIManager.instance) {
            UIManager.instance = new UIManager();
        }
        return UIManager.instance;
    }

    private OnUIDestroy(widget: UE.UserWidget): void {
        if (widget) {
            let nIndex = this.m_arrCacheUI.indexOf(widget);
            if (nIndex != -1) {
                this.DispatchEvent(EventDefineBase.UI_DESTROY, widget);
                this.m_arrCacheUI.splice(nIndex, 1);
            }
        }
    }

    /**
     * 创建UI
     * @param strUIPath UI蓝图路径
     * @returns UI实例
     */
    public CreateUI<T extends UE.UserWidget>(strUIPath: string): T {
        let World = Misc.GetWorld();
        const playerController = UE.GameplayStatics.GetPlayerController(World, 0);
        const widgetClass = UE.Class.Load(strUIPath);
        const widget = UE.WidgetBlueprintLibrary.Create(World, widgetClass, playerController) as T;
        if (widget) {
            this.m_arrCacheUI.push(widget);

            this.DispatchEvent(EventDefineBase.UI_CREATE, widget);
        }
        return widget;
    }

    public CacheUI(widget: UE.UserWidget): void {
        this.m_arrCacheUI.push(widget);
    }

    /**
     * 获取UI
     * @param strUIName UI实例名
     * @param bFuzzy 是否模糊匹配，实例名通常会跟着一个数字后缀，模糊匹配则不带后缀
     * @returns UI实例
     */
    public GetUI<T extends UE.UserWidget>(strUIName: string, bFuzzy: boolean = true): T {
        for (let i = 0; i < this.m_arrCacheUI.length; i++) {
            const widget = this.m_arrCacheUI[i];
            if (Misc.IsValid(widget)) {
                if (bFuzzy) {
                    if (widget.GetName().startsWith(strUIName)) {
                        return widget as T;
                    }
                }
                else {
                    if (widget.GetName() == strUIName) {
                        return widget as T;
                    }
                }
            }
            else {
                this.m_arrCacheUI.splice(i, 1);
                i--;
                console.warn("UI缓存被释放没有及时去除TS引用！");
            }
        }

    }

    public GetOrCreateUI<T extends UE.UserWidget>(strUIPath: string, strUIName: string, bFuzzy: boolean = true): T {
        let widget = this.GetUI(strUIName, bFuzzy);
        if (widget) {
            return widget as T;
        }
        widget = this.CreateUI(strUIPath);
        return widget as T;
    }

    /**
     * 移除UI
     * @param strUIName UI实例名
     * @param bFuzzy 是否模糊匹配，实例名通常会跟着一个数字后缀，模糊匹配则不带后缀
     * @returns 是否移除成功
     */
    public RemoveUI(strUIName: string, bFuzzy: boolean = true): boolean {
        let widget = this.GetUI(strUIName, bFuzzy);
        if (widget) {
            widget.RemoveFromParent();
            return true;
        }

        return false;
    }
}