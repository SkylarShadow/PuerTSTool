

/**
 * 判断字符串是否为 null 或仅包含空白字符。
 * 
 * @param input - 要检查的字符串。
 * @returns 如果字符串为 null 或仅包含空白字符，则返回 true；否则返回 false。
 */
export function IsStrNullOrWhitespace(input: string): boolean {
    return !input || input.trim().length === 0;
}