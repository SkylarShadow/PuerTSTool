import UIBase from "../../Framework/UI/UIBase";
import mixin from "../../Framework/Utils/mixin";
import * as UE from "ue";
import EventDefine from "../../Define/EventDefine";
import { $ref, $unref } from "puerts";

export interface TS_TestTs extends UE.PuerTSTool.TS_Example.UI.WBP_TestTs.WBP_TestTs_C { }
export const TestTsPath = "/PuerTSTool/TS_Example/UI/WBP_TestTs.WBP_TestTs_C";

@mixin(TestTsPath, true)
export class TS_TestTs extends UIBase implements TS_TestTs {
    Construct(): void {
        super.Construct();
        this.TestInfo.SetText("This is a Test TS Widget!");
    }

    Destruct(): void {
        super.Destruct();
    }
}