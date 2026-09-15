import * as net from "net";
import * as vscode from "vscode";

const assetPathPattern = /\b(?:const|let|var)\s+assetPath\b(?:\s*:\s*[^=;]+)?\s*=\s*["']([^"']+)["']/gi;

type AssetPathMatch = {
    assetPath: string;
    range: vscode.Range;
};

type DeclarationLocation = {
    uri: vscode.Uri;
    range: vscode.Range;
};

type DeclarationCommandArgs = {
    uri: string;
    range: [number, number, number, number];
    originUri: string;
    originPosition: [number, number];
};

export function activate(context: vscode.ExtensionContext): void {
    const codeLensProvider = new AssetPathCodeLensProvider();
    const hoverProvider = new PuertsTypeHoverProvider();

    context.subscriptions.push(
        vscode.languages.registerCodeLensProvider(
            [
                { language: "typescript", scheme: "file" },
                { language: "typescriptreact", scheme: "file" },
            ],
            codeLensProvider,
        ),
        vscode.languages.registerHoverProvider(
            [
                { language: "typescript", scheme: "file" },
                { language: "typescriptreact", scheme: "file" },
            ],
            hoverProvider,
        ),
        vscode.commands.registerCommand("puertsTool.openBlueprint", (assetPath: string) =>
            sendOpenAssetCommand(assetPath, "open"),
        ),
        vscode.commands.registerCommand("puertsTool.revealBlueprint", (assetPath: string) =>
            sendOpenAssetCommand(assetPath, "reveal"),
        ),
        vscode.commands.registerCommand("puertsTool.peekTypeDeclaration", (args: DeclarationCommandArgs) =>
            peekTypeDeclaration(args),
        ),
        vscode.commands.registerCommand("puertsTool.openTypeDeclaration", (args: DeclarationCommandArgs) =>
            openTypeDeclaration(args),
        ),
    );
}

export function deactivate(): void {
}

class AssetPathCodeLensProvider implements vscode.CodeLensProvider {
    provideCodeLenses(document: vscode.TextDocument): vscode.CodeLens[] {
        return findAssetPaths(document).flatMap((match) => [
            new vscode.CodeLens(match.range, {
                title: "Open Blueprint",
                command: "puertsTool.openBlueprint",
                arguments: [match.assetPath],
            }),
            new vscode.CodeLens(match.range, {
                title: "Reveal In UE",
                command: "puertsTool.revealBlueprint",
                arguments: [match.assetPath],
            }),
        ]);
    }
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
    if (!normalized.length) {
        return undefined;
    }

    return (
        normalized.find(isPuertsTypeDeclaration) ??
        normalized.find((location) => location.uri.toString() !== uri.toString() && !isTypeScriptStandardLibrary(location)) ??
        undefined
    );
}

function normalizeDeclarationLocation(location: vscode.Location | vscode.LocationLink): DeclarationLocation | undefined {
    if (location instanceof vscode.Location) {
        return {
            uri: location.uri,
            range: location.range,
        };
    }

    if ("targetUri" in location) {
        return {
            uri: location.targetUri,
            range: location.targetRange,
        };
    }

    return undefined;
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
        if (sawOpeningBrace && braceDepth <= 0 && line > startLine) {
            break;
        }

        if (line >= hardEndLine) {
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
    if (isTypeScriptStandardLibrary(location)) {
        return false;
    }

    return (
        filePath.endsWith("/ue.d.ts") ||
        filePath.includes("/typing/") ||
        filePath.includes("/typings/") ||
        filePath.includes("/typescript/")
    );
}

function isTypeScriptStandardLibrary(location: DeclarationLocation): boolean {
    const filePath = location.uri.fsPath.replace(/\\/g, "/").toLowerCase();
    return filePath.includes("/node_modules/typescript/lib/");
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
    const config = vscode.workspace.getConfiguration("puertsTool");
    const configured = config.get<number>("hoverPreviewMaxLines", 28);
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
    const originUri = vscode.Uri.parse(args.originUri);
    const originPosition = new vscode.Position(args.originPosition[0], args.originPosition[1]);

    await vscode.commands.executeCommand(
        "editor.action.peekLocations",
        originUri,
        originPosition,
        [new vscode.Location(declaration.uri, declaration.range)],
        "peek",
    );
}

async function openTypeDeclaration(args: DeclarationCommandArgs): Promise<void> {
    const declaration = declarationLocationFromArgs(args);
    await vscode.window.showTextDocument(declaration.uri, {
        selection: declaration.range,
        preview: true,
    });
}

function declarationLocationFromArgs(args: DeclarationCommandArgs): DeclarationLocation {
    return {
        uri: vscode.Uri.parse(args.uri),
        range: new vscode.Range(
            args.range[0],
            args.range[1],
            args.range[2],
            args.range[3],
        ),
    };
}

function findAssetPaths(document: vscode.TextDocument): AssetPathMatch[] {
    const matches: AssetPathMatch[] = [];
    const text = document.getText();

    assetPathPattern.lastIndex = 0;
    for (let match = assetPathPattern.exec(text); match; match = assetPathPattern.exec(text)) {
        const assetPath = match[1];
        const assetPathStart = match.index + match[0].indexOf(assetPath);
        const start = document.positionAt(assetPathStart);
        const end = document.positionAt(assetPathStart + assetPath.length);

        matches.push({
            assetPath,
            range: new vscode.Range(start, end),
        });
    }

    return matches;
}

async function sendOpenAssetCommand(assetPath: string, mode: "open" | "reveal"): Promise<void> {
    const config = vscode.workspace.getConfiguration("puertsTool");
    const host = config.get<string>("bridgeHost", "127.0.0.1");
    const port = config.get<number>("bridgePort", 18777);

    try {
        const response = await sendBridgeCommand(host, port, {
            cmd: "openAsset",
            assetPath,
            mode,
        });

        if (!response.ok) {
            vscode.window.showErrorMessage(`PuerTS bridge failed: ${response.message}`);
        }
    } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        vscode.window.showErrorMessage(`Cannot connect to PuerTS bridge at ${host}:${port}. ${message}`);
    }
}

function sendBridgeCommand(
    host: string,
    port: number,
    payload: Record<string, unknown>,
): Promise<{ ok: boolean; message: string }> {
    return new Promise((resolve, reject) => {
        const socket = net.createConnection({ host, port });
        let response = "";

        socket.setEncoding("utf8");
        socket.setTimeout(3000);

        socket.on("connect", () => {
            socket.write(`${JSON.stringify(payload)}\n`);
        });

        socket.on("data", (chunk: string) => {
            response += chunk;
            if (response.includes("\n")) {
                socket.end();
            }
        });

        socket.on("timeout", () => {
            socket.destroy(new Error("request timed out"));
        });

        socket.on("error", reject);

        socket.on("close", () => {
            if (!response.trim()) {
                reject(new Error("empty bridge response"));
                return;
            }

            try {
                const parsed = JSON.parse(response.trim()) as { ok?: boolean; message?: string };
                resolve({
                    ok: Boolean(parsed.ok),
                    message: parsed.message ?? "",
                });
            } catch {
                reject(new Error(`invalid bridge response: ${response.trim()}`));
            }
        });
    });
}
