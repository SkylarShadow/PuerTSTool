import * as UE from "ue";
import mixin, { EMixinMode } from "%ROOT_PATH%/Framework/Utils/mixin";


export interface %TS_NAME% extends %MIXIN_BLUEPRINT_TYPE% {}
const assetPath = "%BLUEPRINT_PATH%";

@mixin(assetPath, true, EMixinMode.Listening)
export class %TS_NAME% implements %TS_NAME% {


}
