import * as UE from "ue";
import { $ref, $unref } from "puerts";
import UIBase from "%ROOT_PATH%/Framework/UI/UIBase";
import mixin,{ EMixinMode } from "%ROOT_PATH%/Framework/Utils/mixin";


export interface %TS_NAME% extends %MIXIN_BLUEPRINT_TYPE% {}
const AssetPath = "%BLUEPRINT_PATH%";

@mixin(AssetPath,false,EMixinMode.Listening)
export class %TS_NAME% extends UIBase implements %TS_NAME% {
    Construct(): void {
        super.Construct();
    }

    Destruct(): void {
        super.Destruct();
    }

}