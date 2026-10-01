import * as vscode from "vscode";

import { DtsIndex } from "./dtsIndex";
import { matchesAllTerms, scoreMatch, tokenizeQuery } from "./matcher";
import { DtsApiEntry, DtsApiKind } from "./types";

const selector: vscode.DocumentSelector = [
    { language: "typescript", scheme: "file" },
    { language: "typescriptreact", scheme: "file" },
];

const triggerCharacters = [
    ".",
    " ",
    "_",
    ..."abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ".split(""),
];

type CompletionContext = {
    query: string;
    range: vscode.Range;
    receiver?: string;
};

type BuiltinApiEntry = {
    symbol: string;
    qualifiedName: string;
    insertText: string | vscode.SnippetString;
    detail: string;
    documentation: string;
};

const builtinApiEntries: BuiltinApiEntry[] = [
    makeMiscApi("GetWorld", "Misc.GetWorld()", "Get current UE world."),
    makeMiscApi("GetPlayerController", new vscode.SnippetString("Misc.GetPlayerController(${1:0})"), "Get player controller by index. Defaults to 0."),
    makeMiscApi("GetGameInstance", "Misc.GetGameInstance()", "Get current game instance."),
    makeMiscApi("GetTSSubsys", "Misc.GetTSSubsys()", "Get PuerTSTool TS subsystem."),
    makeMiscApi("PrintInScreen", new vscode.SnippetString("Misc.PrintInScreen(${1:\"\"})"), "Print a debug message to the UE screen."),
    makeMiscApi("IsValid", new vscode.SnippetString("Misc.IsValid(${1:object})"), "Safely test whether a UE object reference is valid."),
    makeMiscApi("AsyncLoad", new vscode.SnippetString("Misc.AsyncLoad(${1:path})"), "Asynchronously load a UE class by asset path."),
    makeMiscApi("GetCommonText", new vscode.SnippetString("Misc.GetCommonText(${1:key})"), "Read localized text from the common string table."),
    makeMiscApi("GetErrorCodeText", new vscode.SnippetString("Misc.GetErrorCodeText(${1:key})"), "Read localized text from the error-code string table."),
];

export function registerDtsCompletion(context: vscode.ExtensionContext): void {
    const dtsIndex = new DtsIndex();
    context.subscriptions.push(
        dtsIndex,
        vscode.languages.registerCompletionItemProvider(
            selector,
            new PuertsDtsCompletionProvider(dtsIndex),
            ...triggerCharacters,
        ),
    );
}

class PuertsDtsCompletionProvider implements vscode.CompletionItemProvider {
    constructor(private readonly dtsIndex: DtsIndex) {
    }

    async provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position,
        token: vscode.CancellationToken,
    ): Promise<vscode.CompletionList | undefined> {
        const completionContext = getCompletionContext(document, position);
        if (!completionContext || completionContext.query.length < 2) {
            return undefined;
        }

        const builtinEntries = searchBuiltinApis(completionContext);
        let entries: DtsApiEntry[] = [];

        if (builtinEntries.length > 0 && !this.dtsIndex.isReady()) {
            void this.dtsIndex.ensureReady().then(undefined, () => undefined);
        } else {
            await this.dtsIndex.ensureReady();
            if (token.isCancellationRequested) {
                return undefined;
            }
            entries = this.dtsIndex.search(completionContext.query, completionContext.receiver);
        }

        return new vscode.CompletionList(
            [
                ...builtinEntries.map((entry, index) => makeBuiltinCompletionItem(entry, completionContext, index)),
                ...entries.map((entry, index) =>
                    makeCompletionItem(entry, completionContext, index + builtinEntries.length),
                ),
            ],
            false,
        );
    }
}

function getCompletionContext(document: vscode.TextDocument, position: vscode.Position): CompletionContext | undefined {
    const linePrefix = document.lineAt(position.line).text.slice(0, position.character);
    const queryMatch = /[$A-Za-z_][\w$]*(?:\s+[$A-Za-z_][\w$]*)?$/.exec(linePrefix);
    if (!queryMatch) {
        return undefined;
    }

    const queryStart = position.character - queryMatch[0].length;
    const beforeQuery = linePrefix.slice(0, queryStart);
    const receiverMatch = /([$A-Za-z_][\w$]*(?:\.[$A-Za-z_][\w$]*)*)\.$/.exec(beforeQuery);
    return {
        query: queryMatch[0],
        range: new vscode.Range(position.line, queryStart, position.line, position.character),
        receiver: receiverMatch?.[1],
    };
}

function makeCompletionItem(
    entry: DtsApiEntry,
    completionContext: CompletionContext,
    rank: number,
): vscode.CompletionItem {
    const item = new vscode.CompletionItem(entry.symbol, completionItemKind(entry.kind));
    item.detail = entry.qualifiedName;
    item.documentation = new vscode.MarkdownString(
        `\`\`\`ts\n${entry.detail}\n\`\`\`\n\nDeclared in \`${vscode.workspace.asRelativePath(entry.uri, false)}:${entry.range.start.line + 1}\``,
    );
    item.range = completionContext.range;
    item.insertText = completionContext.receiver ? entry.symbol : entry.qualifiedName;
    item.filterText = `${entry.symbol} ${entry.qualifiedName} ${completionContext.query}`;
    item.sortText = String(rank).padStart(3, "0");
    return item;
}

function makeBuiltinCompletionItem(
    entry: BuiltinApiEntry,
    completionContext: CompletionContext,
    rank: number,
): vscode.CompletionItem {
    const item = new vscode.CompletionItem(entry.symbol, vscode.CompletionItemKind.Function);
    item.detail = `PuerTS API -> ${entry.qualifiedName}()`;
    item.documentation = new vscode.MarkdownString(`${entry.documentation}\n\nRequires \`import Misc ...\` in the current file.`);
    item.range = completionContext.range;
    item.insertText = entry.insertText;
    item.filterText = `${entry.symbol} ${entry.qualifiedName} ${completionContext.query}`;
    item.sortText = String(rank).padStart(3, "0");
    return item;
}

function searchBuiltinApis(completionContext: CompletionContext): BuiltinApiEntry[] {
    if (completionContext.receiver) {
        return [];
    }

    const terms = tokenizeQuery(completionContext.query);
    if (!terms.length) {
        return [];
    }

    return builtinApiEntries
        .map((entry) => ({ entry, dtsEntry: toDtsEntry(entry) }))
        .filter(({ dtsEntry }) => matchesAllTerms(dtsEntry, terms))
        .sort((left, right) =>
            scoreMatch(right.dtsEntry, completionContext.query) - scoreMatch(left.dtsEntry, completionContext.query),
        )
        .map(({ entry }) => entry);
}

function makeMiscApi(symbol: string, insertText: string | vscode.SnippetString, documentation: string): BuiltinApiEntry {
    return {
        symbol,
        qualifiedName: `Misc.${symbol}`,
        insertText,
        detail: `Misc.${symbol}`,
        documentation,
    };
}

function toDtsEntry(entry: BuiltinApiEntry): DtsApiEntry {
    return {
        id: -1,
        kind: "function",
        symbol: entry.symbol,
        qualifiedName: entry.qualifiedName,
        detail: entry.detail,
        uri: vscode.Uri.file(""),
        range: new vscode.Range(0, 0, 0, 0),
    };
}

function completionItemKind(kind: DtsApiKind): vscode.CompletionItemKind {
    switch (kind) {
        case "class":
            return vscode.CompletionItemKind.Class;
        case "interface":
            return vscode.CompletionItemKind.Interface;
        case "enum":
            return vscode.CompletionItemKind.Enum;
        case "namespace":
            return vscode.CompletionItemKind.Module;
        case "type":
            return vscode.CompletionItemKind.TypeParameter;
        case "function":
        case "method":
            return vscode.CompletionItemKind.Method;
        default:
            return vscode.CompletionItemKind.Property;
    }
}
