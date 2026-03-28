import * as UE from "ue";
import { blueprint } from "puerts";
export default function mixin(blueprintPath: string, objectTakeByNative = false) {
    return function <T extends UE.Object>(target: new () => T) {
        const ucls = UE.Class.Load(blueprintPath);
        const BlueprintClass = blueprint.tojs(ucls);
        return blueprint.mixin(BlueprintClass, target, { objectTakeByNative }) as any;
    };
}
