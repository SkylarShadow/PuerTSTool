import * as path from "path";
import * as vscode from "vscode";

import { matchesAllTerms, scoreMatch, tokenizeQuery } from "./matcher";
import { DtsApiEntry } from "./types";

const configSection = "puertsTool";
const defaultScanFolders = ["TypeScript/Framework/Misc"];
const defaultScanExcludes = ["TypeScript/Framework/Misc/LogExt/**"];

export type FrameworkApiEntry = {
    symbol: string;
    qualifiedName: string;
    insertText: string | vscode.SnippetString;
    detail: string;
    documentation: string;
    uri: vscode.Uri;
    range: vscode.Range;
};

type ScanConfig = {
    folders: string[];
    excludes: string[];
};

export class FrameworkApiIndex implements vscode.Disposable {
    private entries: FrameworkApiEntry[] = [];
    private dirty = true;
    private rebuilding: Promise<void> | undefined;
    private readonly watcher: vscode.FileSystemWatcher;
    private readonly configWatcher: vscode.Disposable;

    constructor() {
        this.watcher = vscode.workspace.createFileSystemWatcher("**/*.ts");
        this.watcher.onDidCreate(() => this.markDirty());
        this.watcher.onDidChange(() => this.markDirty());
        this.watcher.onDidDelete(() => this.markDirty());
        this.configWatcher = vscode.workspace.onDidChangeConfiguration((event) => {
            if (
                event.affectsConfiguration(`${configSection}.apiScanFolders`) ||
                event.affectsConfiguration(`${configSection}.apiScanExcludes`)
            ) {
                this.markDirty();
            }
        });
    }

    dispose(): void {
        this.watcher.dispose();
        this.configWatcher.dispose();
    }

    async ensureReady(): Promise<void> {
        if (!this.dirty) {
            return;
        }
        if (!this.rebuilding) {
            this.rebuilding = this.rebuild().finally(() => {
                this.rebuilding = undefined;
            });
        }
        await this.rebuilding;
    }

    isReady(): boolean {
        return !this.dirty;
    }

    search(query: string, limit = 30): FrameworkApiEntry[] {
        const terms = tokenizeQuery(query);
        if (!terms.length) {
            return [];
        }

        return this.entries
            .filter((entry) => matchesAllTerms(toDtsEntry(entry), terms))
            .sort((left, right) =>
                scoreMatch(toDtsEntry(right), query) - scoreMatch(toDtsEntry(left), query),
            )
            .slice(0, limit);
    }

    private markDirty(): void {
        this.dirty = true;
    }

    private async rebuild(): Promise<void> {
        const files = await findFrameworkApiFiles();
        const entries: FrameworkApiEntry[] = [];

        for (const uri of files) {
            const text = await readWorkspaceText(uri);
            entries.push(...parseFrameworkApiEntries(uri, text));
        }

        this.entries = entries;
        this.dirty = false;
    }
}

function getScanConfig(): ScanConfig {
    const config = vscode.workspace.getConfiguration(configSection);
    const folders = config.get<string[]>("apiScanFolders", defaultScanFolders);
    const excludes = config.get<string[]>("apiScanExcludes", defaultScanExcludes);
    return {
        folders: folders.map(normalizeSettingPath).filter(Boolean),
        excludes: excludes.map(normalizeSettingPath).filter(Boolean),
    };
}

async function findFrameworkApiFiles(): Promise<vscode.Uri[]> {
    const files = new Map<string, vscode.Uri>();
    const scanConfig = getScanConfig();

    for (const folder of vscode.workspace.workspaceFolders ?? []) {
        for (const scanFolder of scanConfig.folders) {
            const pattern = scanFolder.endsWith(".ts") ? scanFolder : `${trimTrailingSlash(scanFolder)}/**/*.ts`;
            const uris = await vscode.workspace.findFiles(new vscode.RelativePattern(folder, pattern));
            for (const uri of uris) {
                const relativePath = normalizeSettingPath(vscode.workspace.asRelativePath(uri, false));
                if (!isExcluded(relativePath, scanConfig.excludes)) {
                    files.set(uri.toString(), uri);
                }
            }
        }
    }

    return [...files.values()];
}

function parseFrameworkApiEntries(uri: vscode.Uri, text: string): FrameworkApiEntry[] {
    const entries: FrameworkApiEntry[] = [];
    const moduleName = path.basename(uri.fsPath, ".ts");
    const exportedClassNames = findExportedClassNames(text);

    for (const className of exportedClassNames) {
        for (const match of text.matchAll(/^\s*static\s+([A-Za-z_$][\w$]*)\s*(?:<[^>]+>)?\s*\(([^)]*)\)\s*(?::\s*([^{;]+))?/gm)) {
            entries.push(makeFrameworkApiEntry(uri, text, match, className));
        }
    }

    for (const match of text.matchAll(/^\s*export\s+(?:async\s+)?function\s+([A-Za-z_$][\w$]*)\s*(?:<[^>]+>)?\s*\(([^)]*)\)\s*(?::\s*([^{;]+))?/gm)) {
        entries.push(makeFrameworkApiEntry(uri, text, match, moduleName));
    }

    return entries;
}

function findExportedClassNames(text: string): string[] {
    const names = new Set<string>();
    for (const match of text.matchAll(/\bexport\s+(?:default\s+)?class\s+([A-Za-z_$][\w$]*)/g)) {
        names.add(match[1]);
    }
    return [...names];
}

function makeFrameworkApiEntry(
    uri: vscode.Uri,
    text: string,
    match: RegExpMatchArray,
    qualifier: string,
): FrameworkApiEntry {
    const symbol = match[1];
    const params = match[2] ?? "";
    const returnType = (match[3] ?? "").trim();
    const qualifiedName = `${qualifier}.${symbol}`;
    const range = makeRange(text, match.index ?? 0);
    const signature = `${qualifiedName}(${params})${returnType ? `: ${returnType}` : ""}`;

    return {
        symbol,
        qualifiedName,
        insertText: makeCallSnippet(qualifiedName, params),
        detail: signature,
        documentation: `Framework API from ${vscode.workspace.asRelativePath(uri, false)}:${range.start.line + 1}`,
        uri,
        range,
    };
}

function makeCallSnippet(qualifiedName: string, params: string): string | vscode.SnippetString {
    const placeholders = splitParams(params)
        .map(makePlaceholder)
        .filter(Boolean);

    if (!placeholders.length) {
        return `${qualifiedName}()`;
    }

    return new vscode.SnippetString(`${qualifiedName}(${placeholders.join(", ")})`);
}

function splitParams(params: string): string[] {
    return params
        .split(",")
        .map((param) => param.trim())
        .filter(Boolean);
}

function makePlaceholder(param: string, index: number): string {
    const defaultValue = /\s=\s*(.+)$/.exec(param)?.[1]?.trim();
    const name = param
        .replace(/\/\*.*?\*\//g, "")
        .replace(/\?.*$/, "")
        .replace(/:.+$/, "")
        .replace(/=.+$/, "")
        .trim()
        .replace(/^(public|private|protected|readonly)\s+/, "");
    const placeholder = defaultValue || name || `arg${index + 1}`;
    return `\${${index + 1}:${placeholder}}`;
}

function makeRange(text: string, index: number): vscode.Range {
    const before = text.slice(0, index);
    const lines = before.split(/\r\n|\r|\n/);
    const line = lines.length - 1;
    const character = lines[lines.length - 1].length;
    return new vscode.Range(line, character, line, character);
}

function toDtsEntry(entry: FrameworkApiEntry): DtsApiEntry {
    return {
        id: -1,
        kind: "function",
        symbol: entry.symbol,
        qualifiedName: entry.qualifiedName,
        detail: entry.detail,
        uri: entry.uri,
        range: entry.range,
    };
}

async function readWorkspaceText(uri: vscode.Uri): Promise<string> {
    const openDocument = vscode.workspace.textDocuments.find((document) => document.uri.toString() === uri.toString());
    if (openDocument) {
        return openDocument.getText();
    }
    return new TextDecoder("utf-8").decode(await vscode.workspace.fs.readFile(uri));
}

function isExcluded(relativePath: string, excludes: string[]): boolean {
    return excludes.some((exclude) => matchPathPattern(relativePath, exclude));
}

function matchPathPattern(relativePath: string, pattern: string): boolean {
    const normalizedPattern = trimTrailingSlash(pattern);
    if (!hasGlob(normalizedPattern)) {
        return relativePath === normalizedPattern || relativePath.startsWith(`${normalizedPattern}/`);
    }
    return globToRegExp(normalizedPattern).test(relativePath);
}

function globToRegExp(pattern: string): RegExp {
    let source = "";
    for (let index = 0; index < pattern.length; index++) {
        const char = pattern[index];
        const nextChar = pattern[index + 1];
        if (char === "*" && nextChar === "*") {
            source += ".*";
            index++;
        } else if (char === "*") {
            source += "[^/]*";
        } else if (char === "?") {
            source += "[^/]";
        } else {
            source += escapeRegExp(char);
        }
    }
    return new RegExp(`^${source}$`, "i");
}

function hasGlob(pattern: string): boolean {
    return /[*?]/.test(pattern);
}

function normalizeSettingPath(value: string): string {
    return trimTrailingSlash(value.replace(/\\/g, "/").replace(/^\.\//, ""));
}

function trimTrailingSlash(value: string): string {
    return value.replace(/\/+$/, "");
}

function escapeRegExp(value: string): string {
    return value.replace(/[|\\{}()[\]^$+*?.]/g, "\\$&");
}
