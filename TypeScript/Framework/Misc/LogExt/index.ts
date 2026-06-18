import "./source";
import "./format";

// 扩展console.log，使其在日志的最前面附带调用者函数名和行号
// 保存原始的console方法，避免递归调用
const originalLog = console.log;
const originalInfo = console.info;
const originalWarn = console.warn;
const originalError = console.error;

// 解析调用者信息
function getCallerInfo() {
    const stack = new Error().stack;
    if (!stack) return null;

    // 获取调用者行（跳过前两行：Error构造函数和getCallerInfo本身）
    const stackLines = stack.split("\n");
    const callerLine = stackLines[3]; // 第4行才是真正的调用者

    if (!callerLine) return null;

    // 解析函数名和位置信息
    // 匹配格式：at functionName (filePath:line:column)
    // 或者：at filePath:line:column (匿名函数)
    const match = callerLine.match(/at\s+(?:([^\s(]+)\s+)?\(([^)]+)\)|at\s+([^\s]+)/);

    if (!match) return null;

    let functionName = match[1] || "anonymous";
    let location = match[2] || match[3];

    // 简化函数名，特别是UE5蓝图相关的函数名
    // 对于格式如 /Game/Path/Blueprint.Blueprint_C.FunctionName 的函数名，只保留FunctionName
    if (functionName.includes(".")) {
        const parts = functionName.split(".");
        functionName = parts[parts.length - 1];
    }

    // 对于格式如 ClassName::FunctionName 的函数名，保留 ClassName::FunctionName
    // 对于匿名函数，保持原样

    if (!location) return null;

    // 尝试从位置信息中解析文件路径和行号
    // 对于TypeScript项目，位置信息可能包含源映射信息
    // 格式可能是：file.js:line:column
    const locationMatch = location.match(/^(.*):(\d+):(\d+)$/);

    if (!locationMatch) return null;

    const filePath = locationMatch[1];
    const lineNumber = locationMatch[2];

    // 尝试从filePath中提取TypeScript源文件信息
    // 如果启用了source map，可能需要额外处理
    // 这里简单地尝试将JavaScript路径转换为TypeScript路径
    // 只保留文件名部分
    const fileName = filePath.split(/[\\/]/).pop() || filePath.split(/[\\/]/).pop() || "unknown";

    return {
        functionName,
        fileName,
        lineNumber,
    };
}

console.log = function (...args: any[]) {
    const callerInfo = getCallerInfo();
    if (callerInfo) {
        args.unshift(`[${callerInfo.fileName}:${callerInfo.lineNumber}:${callerInfo.functionName}]`);
    }
    // 将多个参数合并成一个字符串，用空格分隔，避免底层库使用逗号分隔
    const combinedArgs = args.map((arg) => String(arg)).join("\t");
    originalLog.apply(console, [combinedArgs]);
};

console.info = function (...args: any[]) {
    const callerInfo = getCallerInfo();
    if (callerInfo) {
        args.unshift(`[${callerInfo.fileName}:${callerInfo.lineNumber}:${callerInfo.functionName}]`);
    }
    // 将多个参数合并成一个字符串，用空格分隔，避免底层库使用逗号分隔
    const combinedArgs = args.map((arg) => String(arg)).join("\t");
    originalInfo.apply(console, [combinedArgs]);
};

console.warn = function (...args: any[]) {
    const callerInfo = getCallerInfo();
    if (callerInfo) {
        args.unshift(`[${callerInfo.fileName}:${callerInfo.lineNumber}:${callerInfo.functionName}]`);
    }
    // 将多个参数合并成一个字符串，用空格分隔，避免底层库使用逗号分隔
    const combinedArgs = args.map((arg) => String(arg)).join("\t");
    originalWarn.apply(console, [combinedArgs]);
};

console.error = function (...args: any[]) {
    const callerInfo = getCallerInfo();
    if (callerInfo) {
        args.unshift(`[${callerInfo.fileName}:${callerInfo.lineNumber}:${callerInfo.functionName}]`);
    }
    // 将多个参数合并成一个字符串，用空格分隔，避免底层库使用逗号分隔
    const combinedArgs = args.map((arg) => String(arg)).join("\t");
    // 获取完整的堆栈信息
    const stack = new Error().stack;
    const errorOutput = stack ? `${combinedArgs}\n${stack}` : combinedArgs;
    originalError.apply(console, [errorOutput]);
};