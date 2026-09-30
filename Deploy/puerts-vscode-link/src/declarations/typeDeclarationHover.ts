import * as vscode from "vscode";

type DeclarationLocation = { uri: vscode.Uri; range: vscode.Range };
type DeclarationCommandArgs = {
    uri: string;
    range: [number, number, number, number];
    originUri: string;
    originPosition: [number, number];
};

export function registerDeclarationFeatures(context: vscode.ExtensionContext): void {
    const selector: vscode.DocumentSelector = [
        { language: "typescript", scheme: "file" },
        { language: "typescriptreact", scheme: "file" },
    ];
    context.subscriptions.push(
        vscode.languages.registerHoverProvider(selector, new PuertsTypeHoverProvider()),
        vscode.commands.registerCommand("puertsTool.peekTypeDeclaration", peekTypeDeclaration),
        vscode.commands.registerCommand("puertsTool.openTypeDeclaration", openTypeDeclaration),
    );
}

class PuertsTypeHoverProvider implements vscode.HoverProvider {
    async provideHover(
        document: vscode.TextDocument,
        position: vscode.Position,
        token: vscode.CancellationToken,
    ): Promise<vscode.Hover | undefined> {
        const wordRange = document.getWordRangeAtPosition(position, /[$A-Za-z_][\w$]*/);
        if (!wordRange) {
            return undefined;
        }

        const declaration = await getTypeDeclaration(document.uri, position, token);
        if (!declaration || token.isCancellationRequested) {
            return undefined;
        }

        const sourceDocument = await vscode.workspace.openTextDocument(declaration.uri);
        const preview = buildDeclarationPreview(sourceDocument, declaration.range, getHoverPreviewMaxLines());
        if (!preview.code.trim()) {
            return undefined;
        }

        const args = makeDeclarationCommandArgs(declaration, document.uri, position);
        const peekUri = makeCommandUri("puertsTool.peekTypeDeclaration", args);
        const openUri = makeCommandUri("puertsTool.openTypeDeclaration", args);
        const relativePath = vscode.workspace.asRelativePath(declaration.uri, false);
        const lineNumber = declaration.range.start.line + 1;
        const markdown = new vscode.MarkdownString(undefined, true);
        markdown.isTrusted = true;
        markdown.appendMarkdown("**PuerTS Type Declaration**\n\n");
        markdown.appendMarkdown(`Declared in \`${relativePath}:${lineNumber}\`\n\n`);
        markdown.appendMarkdown(`[Peek Declaration](${peekUri}) | [Open Declaration](${openUri})\n\n`);
        if (preview.truncated) {
            markdown.appendMarkdown("_Declaration preview truncated. Use Peek Declaration for the full context._\n\n");
        }
        markdown.appendCodeblock(preview.code, "ts");
        return new vscode.Hover(markdown, wordRange);
    }
}

async function getTypeDeclaration(
    uri: vscode.Uri,
    position: vscode.Position,
    token: vscode.CancellationToken,
): Promise<DeclarationLocation | undefined> {
    const locations = await vscode.commands.executeCommand<Array<vscode.Location | vscode.LocationLink>>(
        "vscode.executeTypeDefinitionProvider",
        uri,
        position,
    );
    if (token.isCancellationRequested || !locations?.length) {
        return undefined;
    }

    const normalized = locations.map(normalizeDeclarationLocation).filter((location): location is DeclarationLocation =>
        Boolean(location),
    );
    return (
        normalized.find(isPuertsTypeDeclaration) ??
        normalized.find((location) => location.uri.toString() !== uri.toString() && !isTypeScriptStandardLibrary(location))
    );
}

function normalizeDeclarationLocation(location: vscode.Location | vscode.LocationLink): DeclarationLocation | undefined {
    if (location instanceof vscode.Location) {
        return { uri: location.uri, range: location.range };
    }
    return "targetUri" in location ? { uri: location.targetUri, range: location.targetRange } : undefined;
}

function buildDeclarationPreview(
    document: vscode.TextDocument,
    range: vscode.Range,
    maxLines: number,
): { code: string; truncated: boolean } {
    const startLine = Math.max(0, range.start.line);
    const hardEndLine = Math.min(document.lineCount - 1, startLine + maxLines - 1);
    let endLine = hardEndLine;
    let braceDepth = 0;
    let sawOpeningBrace = false;

    for (let line = startLine; line < document.lineCount; line++) {
        const text = document.lineAt(line).text;
        for (const char of text) {
            if (char === "{") {
                braceDepth++;
                sawOpeningBrace = true;
            } else if (char === "}") {
                braceDepth--;
            }
        }
        endLine = line;
        if ((sawOpeningBrace && braceDepth <= 0 && line > startLine) || line >= hardEndLine) {
            break;
        }
    }

    const lines: string[] = [];
    for (let line = startLine; line <= endLine; line++) {
        lines.push(document.lineAt(line).text);
    }
    return {
        code: trimIndent(lines).join("\n"),
        truncated: endLine < document.lineCount - 1 && (!sawOpeningBrace || braceDepth > 0),
    };
}

function isPuertsTypeDeclaration(location: DeclarationLocation): boolean {
    const filePath = location.uri.fsPath.replace(/\\/g, "/").toLowerCase();
    return !isTypeScriptStandardLibrary(location) && (
        filePath.endsWith("/ue.d.ts") ||
        filePath.includes("/typing/") ||
        filePath.includes("/typings/") ||
        filePath.includes("/typescript/")
    );
}

function isTypeScriptStandardLibrary(location: DeclarationLocation): boolean {
    return location.uri.fsPath.replace(/\\/g, "/").toLowerCase().includes("/node_modules/typescript/lib/");
}

function trimIndent(lines: string[]): string[] {
    const nonEmptyLines = lines.filter((line) => line.trim().length > 0);
    if (!nonEmptyLines.length) {
        return lines;
    }
    const minIndent = Math.min(...nonEmptyLines.map((line) => line.match(/^\s*/)?.[0].length ?? 0));
    return lines.map((line) => line.slice(minIndent));
}

function getHoverPreviewMaxLines(): number {
    const configured = vscode.workspace.getConfiguration("puertsTool").get<number>("hoverPreviewMaxLines", 28);
    return Math.max(8, Math.min(120, configured));
}

function makeDeclarationCommandArgs(
    declaration: DeclarationLocation,
    originUri: vscode.Uri,
    originPosition: vscode.Position,
): DeclarationCommandArgs {
    return {
        uri: declaration.uri.toString(),
        range: [
            declaration.range.start.line,
            declaration.range.start.character,
            declaration.range.end.line,
            declaration.range.end.character,
        ],
        originUri: originUri.toString(),
        originPosition: [originPosition.line, originPosition.character],
    };
}

function makeCommandUri(command: string, args: DeclarationCommandArgs): vscode.Uri {
    return vscode.Uri.parse(`command:${command}?${encodeURIComponent(JSON.stringify([args]))}`);
}

async function peekTypeDeclaration(args: DeclarationCommandArgs): Promise<void> {
    const declaration = declarationLocationFromArgs(args);
    await vscode.commands.executeCommand(
        "editor.action.peekLocations",
        vscode.Uri.parse(args.originUri),
        new vscode.Position(args.originPosition[0], args.originPosition[1]),
        [new vscode.Location(declaration.uri, declaration.range)],
        "peek",
    );
}

async function openTypeDeclaration(args: DeclarationCommandArgs): Promise<void> {
    const declaration = declarationLocationFromArgs(args);
    await vscode.window.showTextDocument(declaration.uri, { selection: declaration.range, preview: true });
}

function declarationLocationFromArgs(args: DeclarationCommandArgs): DeclarationLocation {
    return {
        uri: vscode.Uri.parse(args.uri),
        range: new vscode.Range(args.range[0], args.range[1], args.range[2], args.range[3]),
    };
}
