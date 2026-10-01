import { Index } from "flexsearch";
import * as vscode from "vscode";

import { parseDtsEntries } from "./dtsParser";
import { getParentName, makeSearchText, matchesAllTerms, scoreMatch, tokenizeQuery } from "./matcher";
import { DtsApiEntry } from "./types";

export class DtsIndex implements vscode.Disposable {
    private entriesById = new Map<number, DtsApiEntry>();
    private searchIndex = createSearchIndex();
    private dirty = true;
    private rebuilding: Promise<void> | undefined;
    private readonly watcher: vscode.FileSystemWatcher;

    constructor() {
        this.watcher = vscode.workspace.createFileSystemWatcher("**/*.d.ts");
        this.watcher.onDidCreate((uri) => this.markDirtyIfTypingFile(uri));
        this.watcher.onDidChange((uri) => this.markDirtyIfTypingFile(uri));
        this.watcher.onDidDelete((uri) => this.markDirtyIfTypingFile(uri));
    }

    dispose(): void {
        this.watcher.dispose();
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

    search(query: string, parent?: string, limit = 60): DtsApiEntry[] {
        const terms = tokenizeQuery(query);
        if (!terms.length) {
            return [];
        }

        const candidateIds = intersectCandidateIds(terms, this.searchIndex);
        return candidateIds
            .map((id) => this.entriesById.get(id))
            .filter((entry): entry is DtsApiEntry => Boolean(entry))
            .filter((entry) => !parent || getParentName(entry) === parent)
            .filter((entry) => matchesAllTerms(entry, terms))
            .sort((left, right) => scoreMatch(right, query, parent) - scoreMatch(left, query, parent))
            .slice(0, limit);
    }

    private markDirtyIfTypingFile(uri: vscode.Uri): void {
        if (uri.path.toLowerCase().includes("/typing/")) {
            this.dirty = true;
        }
    }

    private async rebuild(): Promise<void> {
        const files = await findTypingFiles();
        const nextEntries = new Map<number, DtsApiEntry>();
        const nextSearchIndex = createSearchIndex();
        let nextId = 0;

        for (const uri of files) {
            const text = await readWorkspaceText(uri);
            const entries = parseDtsEntries(uri, text, nextId);
            nextId += entries.length;
            for (const entry of entries) {
                nextEntries.set(entry.id, entry);
                nextSearchIndex.add(entry.id, makeSearchText(entry.symbol, entry.qualifiedName));
            }
        }

        this.entriesById = nextEntries;
        this.searchIndex = nextSearchIndex;
        this.dirty = false;
    }
}

function createSearchIndex(): Index {
    return new Index({ tokenize: "forward", cache: 100, resolution: 9 });
}

function intersectCandidateIds(terms: string[], searchIndex: Index): number[] {
    const matches = terms.map((term) => new Set(
        searchIndex.search(term, { limit: 3000 }).map((id) => Number(id)),
    ));
    if (!matches.length || matches.some((match) => !match.size)) {
        return [];
    }

    const [firstMatch, ...remainingMatches] = matches.sort((left, right) => left.size - right.size);
    return [...firstMatch].filter((id) => remainingMatches.every((match) => match.has(id)));
}

async function findTypingFiles(): Promise<vscode.Uri[]> {
    const files = new Map<string, vscode.Uri>();
    for (const folder of vscode.workspace.workspaceFolders ?? []) {
        const typingFiles = await vscode.workspace.findFiles(new vscode.RelativePattern(folder, "Typing/**/*.d.ts"));
        for (const uri of typingFiles) {
            files.set(uri.toString(), uri);
        }
    }
    return [...files.values()];
}

async function readWorkspaceText(uri: vscode.Uri): Promise<string> {
    const openDocument = vscode.workspace.textDocuments.find((document) => document.uri.toString() === uri.toString());
    if (openDocument) {
        return openDocument.getText();
    }
    return new TextDecoder("utf-8").decode(await vscode.workspace.fs.readFile(uri));
}
