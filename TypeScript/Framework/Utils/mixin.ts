import * as UE from "ue";
import { blueprint, releaseManualReleaseDelegate, toManualReleaseDelegate } from "puerts";
import Misc from "../Misc/Misc";

type MixinConstructor = new () => UE.Object;

interface AutoMixinInfo {
    target: MixinConstructor;
    objectTakeByNative: boolean;
}

export enum EMixinMode {
    Persistent,
    Listening,
}

const autoMixinInfos = new Map<string, AutoMixinInfo>();
let autoMixinCallbackRegistered = false;

function ApplyAutoMixin(_eventName: string, eventData: UE.TArray<UE.TSEventData>): void {
    if (!eventData || eventData.Num() <= 0) {
        return;
    }

    const data = eventData.Get(0);
    const classPath = data.StringData;
    const info = autoMixinInfos.get(classPath);
    if (!info || !data.ObjectData) {
        return;
    }

    const ucls = data.ObjectData as UE.Class;
    const blueprintClass = blueprint.tojs(ucls);
    try {
        blueprint.mixin(blueprintClass, info.target, { objectTakeByNative: info.objectTakeByNative });
        console.log(`[AutoMixin] mixin succeeded: ${classPath}`);
    } catch (error) {
        const message = error?.toString?.() ?? `${error}`;
        if (!message.includes("had mixin")) {
            console.error(`[AutoMixin] mixin failed: ${classPath}, ${message}`);
        }
    }
}

// 注册C++ -> TS的自动mixin回调。只需注册一次，后续多个@mixin(..., Listening)共用同一个事件入口。
function EnsureAutoMixinCallback(): void {
    if (autoMixinCallbackRegistered) {
        return;
    }

    const tsSubsys = Misc.GetTSSubsys();
    if (!tsSubsys) {
        return;
    }

    tsSubsys.PassTSFunctionAsEvent("ApplyAutoMixin", toManualReleaseDelegate(ApplyAutoMixin));
    autoMixinCallbackRegistered = true;
}

// 清理TS侧自动mixin配方和C++侧路径列表，这里主要负责释放JS手动delegate和TS Map。
export function ShutdownAutoMixin(): void {
    autoMixinInfos.clear();

    const tsSubsys = Misc.GetTSSubsys();
    if (tsSubsys) {
        tsSubsys.ClearAutoMixinClasses();
    }

    if (autoMixinCallbackRegistered) {
        releaseManualReleaseDelegate(ApplyAutoMixin);
        autoMixinCallbackRegistered = false;
    }
}

export default function mixin(blueprintPath: string, objectTakeByNative = false, mode = EMixinMode.Persistent) {
    return function <T extends UE.Object>(target: new () => T) {
        if (mode === EMixinMode.Listening) {
            // 仅把蓝图路径映射到TS类，等待C++在UClass创建时回调ApplyAutoMixin。
            autoMixinInfos.set(blueprintPath, {
                target: target as unknown as MixinConstructor,
                objectTakeByNative,
            });

            EnsureAutoMixinCallback();

            const tsSubsys = Misc.GetTSSubsys();
            if (tsSubsys) {
                // C++只保存路径字符串，用来在NotifyUObjectCreated里判断新UClass是否需要mixin。
                tsSubsys.RegisterAutoMixinClass(blueprintPath);

                // 如果目标UClass在TS注册前已经加载，主动扫已加载对象并触发一次mixin。
                tsSubsys.RefreshAutoMixinLoadedClasses();
            }

            return target;
        }

        if (mode === EMixinMode.Persistent) {
            // 持久模式保持旧行为：立即加载UClass并缓存，避免UClass被GC后mixin失效
            const ucls = UE.Class.Load(blueprintPath);

            const tsSubsys = Misc.GetTSSubsys();
            if (tsSubsys) {
                tsSubsys.CacheClass(ucls);
            }

            const BlueprintClass = blueprint.tojs(ucls);
            return blueprint.mixin(BlueprintClass, target, {objectTakeByNative}) as any;
        }
    };
}
