import * as vscode from "vscode";

export type DtsApiKind =
    | "namespace"
    | "class"
    | "interface"
    | "enum"
    | "type"
    | "function"
    | "method"
    | "property";

export type DtsApiEntry = {
    id: number;
    kind: DtsApiKind;
    symbol: string;
    qualifiedName: string;
    detail: string;
    uri: vscode.Uri;
    range: vscode.Range;
};
