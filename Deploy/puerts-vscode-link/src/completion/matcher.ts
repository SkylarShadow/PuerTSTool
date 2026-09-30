import { DtsApiEntry } from "./types";

export function makeSearchText(symbol: string, qualifiedName: string): string {
    const symbolWords = splitIdentifier(symbol);
    const qualifiedWords = splitIdentifier(qualifiedName);
    const abbreviations = qualifiedName
        .split(".")
        .map(makeAbbreviation)
        .filter(Boolean)
        .join(" ");

    return [symbol, qualifiedName, symbolWords, qualifiedWords, makeAbbreviation(symbol), abbreviations]
        .filter(Boolean)
        .join(" ");
}

export function tokenizeQuery(query: string): string[] {
    return splitIdentifier(query).toLowerCase().split(" ").filter(Boolean);
}

export function matchesAllTerms(entry: DtsApiEntry, terms: string[]): boolean {
    const searchable = makeSearchText(entry.symbol, entry.qualifiedName).toLowerCase();
    const words = searchable.split(" ");
    return terms.every((term) => words.some((word) => word.startsWith(term)));
}

export function scoreMatch(entry: DtsApiEntry, query: string, parent?: string): number {
    const normalizedQuery = query.toLowerCase().replace(/\s+/g, "");
    const symbol = entry.symbol.toLowerCase();
    const qualifiedName = entry.qualifiedName.toLowerCase();
    let score = 0;

    if (symbol === normalizedQuery) {
        score += 1000;
    } else if (symbol.startsWith(normalizedQuery)) {
        score += 800;
    }
    if (parent && parent === getParentName(entry)) {
        score += 600;
    }
    if (entry.kind === "method" || entry.kind === "function") {
        score += 100;
    }
    if (qualifiedName.startsWith("UE.")) {
        score += 50;
    }
    return score;
}

export function getParentName(entry: DtsApiEntry): string | undefined {
    const separator = entry.qualifiedName.lastIndexOf(".");
    return separator >= 0 ? entry.qualifiedName.slice(0, separator) : undefined;
}

function splitIdentifier(value: string): string {
    return value
        .replace(/([a-z0-9])([A-Z])/g, "$1 $2")
        .replace(/[$_.:-]+/g, " ")
        .replace(/\s+/g, " ")
        .trim();
}

function makeAbbreviation(value: string): string {
    return splitIdentifier(value)
        .split(" ")
        .filter(Boolean)
        .map((word) => word[0])
        .join("");
}
