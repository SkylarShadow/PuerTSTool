import * as ts from "typescript";
import * as vscode from "vscode";

import { DtsApiEntry, DtsApiKind } from "./types";

export function parseDtsEntries(uri: vscode.Uri, text: string, firstId: number): DtsApiEntry[] {
    const sourceFile = ts.createSourceFile(uri.fsPath, text, ts.ScriptTarget.Latest, true);
    const entries: DtsApiEntry[] = [];

    const addEntry = (kind: DtsApiKind, symbol: string, qualifiedName: string, node: ts.Node): void => {
        const start = sourceFile.getLineAndCharacterOfPosition(node.getStart(sourceFile));
        const end = sourceFile.getLineAndCharacterOfPosition(node.getEnd());
        entries.push({
            id: firstId + entries.length,
            kind,
            symbol,
            qualifiedName,
            detail: makeDetail(kind, symbol, node, sourceFile),
            uri,
            range: new vscode.Range(start.line, start.character, end.line, end.character),
        });
    };

    const visitStatements = (statements: readonly ts.Statement[], scope: string[]): void => {
        for (const statement of statements) {
            if (ts.isModuleDeclaration(statement)) {
                const name = declarationName(statement.name, sourceFile);
                addEntry("namespace", name, qualify(scope, name), statement);
                visitModuleBody(statement.body, [...scope, name]);
            } else if (ts.isClassDeclaration(statement) && statement.name) {
                visitTypeDeclaration("class", statement.name, statement.members, statement, scope, addEntry);
            } else if (ts.isInterfaceDeclaration(statement)) {
                visitTypeDeclaration("interface", statement.name, statement.members, statement, scope, addEntry);
            } else if (ts.isEnumDeclaration(statement)) {
                const name = declarationName(statement.name, sourceFile);
                addEntry("enum", name, qualify(scope, name), statement);
            } else if (ts.isTypeAliasDeclaration(statement)) {
                const name = declarationName(statement.name, sourceFile);
                addEntry("type", name, qualify(scope, name), statement);
            } else if (ts.isFunctionDeclaration(statement) && statement.name) {
                const name = declarationName(statement.name, sourceFile);
                addEntry("function", name, qualify(scope, name), statement);
            }
        }
    };

    const visitModuleBody = (body: ts.ModuleBody | undefined, scope: string[]): void => {
        if (!body) {
            return;
        }
        if (ts.isModuleBlock(body)) {
            visitStatements(body.statements, scope);
        } else if (ts.isModuleDeclaration(body)) {
            const name = declarationName(body.name, sourceFile);
            addEntry("namespace", name, qualify(scope, name), body);
            visitModuleBody(body.body, [...scope, name]);
        }
    };

    visitStatements(sourceFile.statements, []);
    return entries;
}

function visitTypeDeclaration(
    kind: "class" | "interface",
    nameNode: ts.Identifier,
    members: readonly ts.TypeElement[] | readonly ts.ClassElement[],
    declaration: ts.Declaration,
    scope: string[],
    addEntry: (kind: DtsApiKind, symbol: string, qualifiedName: string, node: ts.Node) => void,
): void {
    const name = nameNode.text;
    const qualifiedName = qualify(scope, name);
    addEntry(kind, name, qualifiedName, declaration);

    for (const member of members) {
        if (!("name" in member) || !member.name || !ts.isPropertyName(member.name)) {
            continue;
        }
        const memberName = declarationName(member.name, declaration.getSourceFile());
        const memberKind = ts.isMethodDeclaration(member) || ts.isMethodSignature(member)
            ? "method"
            : "property";
        addEntry(memberKind, memberName, `${qualifiedName}.${memberName}`, member);
    }
}

function declarationName(name: ts.DeclarationName, sourceFile: ts.SourceFile): string {
    return ts.isIdentifier(name) || ts.isPrivateIdentifier(name) ? name.text : name.getText(sourceFile);
}

function qualify(scope: string[], name: string): string {
    return [...scope, name].join(".");
}

function makeDetail(kind: DtsApiKind, symbol: string, node: ts.Node, sourceFile: ts.SourceFile): string {
    if (kind === "class" || kind === "interface" || kind === "enum" || kind === "namespace" || kind === "type") {
        return `${kind} ${symbol}`;
    }
    return node.getText(sourceFile).replace(/\s+/g, " ").slice(0, 240);
}
