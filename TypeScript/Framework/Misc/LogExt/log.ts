export function LogError(msg: string): void {
    console.error(`[ERROR] ${msg}`);
    // 或使用 UE 的日志系统：UE.UKismetSystemLibrary.PrintString(undefined, msg, false, false, [1,0,0,1], 5);
}