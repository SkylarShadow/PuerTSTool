import * as UE from "ue";
import mixin,{ EMixinMode } from "%ROOT_PATH%/Framework/Utils/mixin";


export interface %TS_NAME% extends %MIXIN_BLUEPRINT_TYPE% {}
const AssetPath = "%BLUEPRINT_PATH%";

@mixin(AssetPath,false,EMixinMode.Listening)
export class %TS_NAME% implements %TS_NAME% {


}