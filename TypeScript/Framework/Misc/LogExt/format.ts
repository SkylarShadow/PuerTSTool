
import * as UE from "ue";

UE.Vector2D.prototype.toString = function () {
    return `(${this.X}, ${this.Y})`;
};

export function Format(obj: any, prettyPrint: boolean = false): string {
    return prettyPrint ? JSON.stringify(obj, null, 2) : JSON.stringify(obj);
}