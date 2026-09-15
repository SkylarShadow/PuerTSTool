import * as UE from "ue";
import { releaseManualReleaseDelegate, toManualReleaseDelegate } from "puerts";
import Misc from "./Framework/Misc/Misc";
import AssetsManager from "./Framework/Assets/AssetsManager";
import ModuleManager from "./Framework/Module/ModuleManager";
import ModuleBase from "./Framework/Module/ModuleBase";
import EventDispatcher from "./Framework/Utils/EventDispatcher";
import { ISingleton } from "./Framework/Interface/ISingleton";
import { ShutdownAutoMixin } from "./Framework/Utils/mixin";

//全局释放函数
function GlobalRelease(
  _eventName: string,
  _eventData: UE.TArray<UE.TSEventData>,
): void {
  G_App.GetInstance().Destroy();
  console.log("GlobalRelease");
}

//全局APP
export class G_App extends EventDispatcher implements ISingleton<G_App> {
  private static m_instance: G_App | null;

  //构造
  constructor() {
    super();
  }

  //初始化
  public Initialize(): void {
    this.InitModule();

    const pTSSubSys = Misc.GetTSSubsys();
    pTSSubSys.PassTSFunctionAsEvent(
      "ReleaseTS",
      toManualReleaseDelegate(GlobalRelease),
    );
    pTSSubSys.OnTSFunction.Bind((eventName, eventData) => {
      this.OnUECallTS(eventName, eventData);
    });

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
    this.DestroyModule();
    AssetsManager.getInstance().Destroy();
    ShutdownAutoMixin();

    //释放TS函数,防止内存泄漏
    releaseManualReleaseDelegate(GlobalRelease);
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
  private OnUECallTS(
    _strEventName: string,
    _eventData: UE.TArray<UE.TSEventData>,
  ): void {
    try {
      // eslint-disable-next-line no-eval
      eval(_strEventName);
    } catch (error) {
      console.error(String(error));
    }
  }

  //初始化模块
  private InitModule(): void {
    //所有TS模块放在这里初始化
    const arrModuleClass: Array<new () => ModuleBase> = [
      // TestModule,
    ];

    for (let i = 0; i < arrModuleClass.length; i++) {
      ModuleManager.GetInstance().RegisterModule(
        arrModuleClass[i].name,
        new arrModuleClass[i](),
      );
    }
    ModuleManager.GetInstance().Initialize();
  }

  //销毁模块
  private DestroyModule(): void {
    //所有TS模块放在这里销毁
    ModuleManager.GetInstance().Destroy();
  }
}
