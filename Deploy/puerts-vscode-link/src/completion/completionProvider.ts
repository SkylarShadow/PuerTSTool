import * as vscode from "vscode";

import { DtsIndex } from "./dtsIndex";
import { DtsApiEntry, DtsApiKind } from "./types";

const selector: vscode.DocumentSelector = [
    { language: "typescript", scheme: "file" },
    { language: "typescriptreact", scheme: "file" },
];

type CompletionContext = {
    query: string;
    range: vscode.Range;
    receiver?: string;
};

export function registerDtsCompletion(context: vscode.ExtensionContext): void {
    const dtsIndex = new DtsIndex();
    context.subscriptions.push(
        dtsIndex,
        vscode.languages.registerCompletionItemProvider(
            selector,
            new PuertsDtsCompletionProvider(dtsIndex),
            ".",
            " ",
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

        await this.dtsIndex.ensureReady();
        if (token.isCancellationRequested) {
            return undefined;
        }

        const entries = this.dtsIndex.search(completionContext.query, completionContext.receiver);
        return new vscode.CompletionList(
            entries.map((entry, index) => makeCompletionItem(entry, completionContext, index)),
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
