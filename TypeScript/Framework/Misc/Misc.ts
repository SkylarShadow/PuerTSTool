import * as UE from 'ue';
import { argv } from 'puerts';


export default class Misc {

    //获取公共文本
    static GetCommonText(_strKey: string): string {
        let str = UE.KismetTextLibrary.TextFromStringTable("/Plugins/PuerTSTool/DataTable/ST_Common.ST_Common", _strKey);
        return str;
    }

    //获取错误码文本
    static GetErrorCodeText(_strKey: string): string {
        let str = UE.KismetTextLibrary.TextFromStringTable("/Game/PuerTSTool/DataTable/ST_ErrorCode.ST_ErrorCode", _strKey);
        return str;
    }

    static GetWorld(): UE.World {
        return (argv.getByName("GameInstance") as UE.GameInstance).GetWorld();
    }

    static GetPlayerController(): UE.PlayerController {
        return UE.GameplayStatics.GetPlayerController(Misc.GetWorld(), 0);
    }

    //获取GameInstance
    static GetGameInstance(): UE.GameInstance {
        return UE.GameplayStatics.GetGameInstance(Misc.GetWorld());
    }

    //获取TS管理器
    static GetTSSubsys(): UE.TSSubsystem {
        let pTSSubSys = UE.TSSubsystem.Get(Misc.GetWorld());
        return pTSSubSys;
    }

    //打印日志到屏幕上
    static PrintInScreen(_strLog: string, Duration?: number /* = 2.000000 */, TextColor?: UE.LinearColor /* = (R=0.000000,G=0.660000,B=1.000000,A=1.000000) */): void {
        UE.KismetSystemLibrary.PrintString(Misc.GetWorld(), _strLog, true, true, TextColor, Duration);
    }

    //判断UE Object是否有效，puerts不会自动释放引用，所以这种判断尽量少用，尽量在逻辑上重置对象
    static IsValid(_object: UE.Object): boolean {
        try {
            return UE.KismetSystemLibrary.IsValid(_object);
        } catch (error) {
            return false;
        }
    }

    static WaitLatentActionState(state: UE.LatentActionState) : Promise<void> {
        return new Promise<void>((resolve, reject) => {
            state.LatentActionCallback.Bind(() => {
                state.LatentActionCallback.Unbind();
                resolve();
            });
        });
    
    }

    static AsyncLoad(path:string): Promise<UE.Class> {
        return new Promise<UE.Class>((resolve, reject) => {
            let asyncLoadObj = new UE.AsyncLoadState();
            asyncLoadObj.LoadedCallback.Bind((cls:UE.Class) => {
                asyncLoadObj.LoadedCallback.Unbind();
                if (cls) {
                    resolve(cls);
                }
                else {
                    reject(`load ${path} fail`);
                }
            });
            asyncLoadObj.StartLoad(path);
        });
    }
}