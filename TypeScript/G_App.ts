import * as UE from 'ue';
import { argv, releaseManualReleaseDelegate, toManualReleaseDelegate } from 'puerts';
import Misc from './Framework/Misc/Misc';
import ModuleManager from './Framework/Module/ModuleManager';
import EventDispatcher from './Framework/Utils/EventDispatcher';
import { ISingleton } from './Framework/Interface/ISingleton';

//全局释放函数
function GlobalDispose(eventName, eventData) {
    G_App.GetInstance().Destroy();
    console.log("GlobalDispose");
}

//全局APP
export class G_App extends EventDispatcher implements ISingleton<G_App> {
    private static m_instance: G_App;

    //构造
    constructor() {
        super();
        
    }

    //初始化
    public Initialize(): void {
        this.InitModule();

        let pTSSubSys = Misc.GetTSSubsys();
        pTSSubSys.PassTSFunctionAsEvent("DisposeTS", toManualReleaseDelegate(GlobalDispose));
        pTSSubSys.OnTSFunction.Bind((eventName, eventData) => { this.OnUECallTS(eventName, eventData); });

        setTimeout(() => {
            this.Start();
        }, 10);
    }

    //开始
    public Start(): void {
        ModuleManager.GetInstance().StartModule();
    }

    //销毁
    public Destroy(): void {
        this.RemoveAllEventListeners();
        this.DisposeModule();

        //释放TS函数,防止内存泄漏
        releaseManualReleaseDelegate(GlobalDispose);
        G_App.m_instance = null;

        console.log("G_App destroy");
    }

    static GetInstance(): G_App {
        if (G_App.m_instance == null) {
            G_App.m_instance = new G_App();
        }
        return G_App.m_instance;
    }

    //响应UE调用TS函数，注入式调用，没有编译检查，慎用！
    private OnUECallTS(_strEventName: string, _EventData: UE.TArray<UE.TSEventData>): void {
        try {
            eval(_strEventName);
        } catch (error) {
            console.error(error.toString());
        }
    }

    //初始化模块
    private InitModule(): void {
        //所有TS模块放在这里初始化
        let arrModuleClass = [
            // TestModule,
        ];

        for (let i = 0; i < arrModuleClass.length; i++) {
            ModuleManager.GetInstance().RegisterModule(arrModuleClass[i].name, new arrModuleClass[i]());
        }
        ModuleManager.GetInstance().Initialize();
    }

    //销毁模块
    private DisposeModule(): void {
        //所有TS模块放在这里销毁
        ModuleManager.GetInstance().Destroy();
    }
}
