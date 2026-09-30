import * as vscode from "vscode";

import { openAssetInEditor } from "../bridge/client";

const assetPathPattern = /\b(?:const|let|var)\s+assetPath\b(?:\s*:\s*[^=;]+)?\s*=\s*["']([^"']+)["']/gi;

type AssetPathMatch = {
    assetPath: string;
    range: vscode.Range;
};

export function registerAssetPathFeatures(context: vscode.ExtensionContext): void {
    const selector: vscode.DocumentSelector = [
        { language: "typescript", scheme: "file" },
        { language: "typescriptreact", scheme: "file" },
    ];

    context.subscriptions.push(
        vscode.languages.registerCodeLensProvider(selector, new AssetPathCodeLensProvider()),
        vscode.commands.registerCommand("puertsTool.openBlueprint", (assetPath: string) =>
            openAssetInEditor(assetPath, "open"),
        ),
        vscode.commands.registerCommand("puertsTool.revealBlueprint", (assetPath: string) =>
            openAssetInEditor(assetPath, "reveal"),
        ),
    );
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

function findAssetPaths(document: vscode.TextDocument): AssetPathMatch[] {
    const matches: AssetPathMatch[] = [];
    const text = document.getText();
    assetPathPattern.lastIndex = 0;

    for (let match = assetPathPattern.exec(text); match; match = assetPathPattern.exec(text)) {
        const assetPath = match[1];
        const assetPathStart = match.index + match[0].indexOf(assetPath);
        const start = document.positionAt(assetPathStart);
        const end = document.positionAt(assetPathStart + assetPath.length);
        matches.push({ assetPath, range: new vscode.Range(start, end) });
    }

    return matches;
}
