import * as vscode from "vscode";

import { registerAssetPathFeatures } from "./assets";
import { registerDtsCompletion } from "./completion";
import { registerDeclarationFeatures } from "./declarations";

export function activate(context: vscode.ExtensionContext): void {
    registerAssetPathFeatures(context);
    registerDeclarationFeatures(context);
    registerDtsCompletion(context);
}

export function deactivate(): void {
}
