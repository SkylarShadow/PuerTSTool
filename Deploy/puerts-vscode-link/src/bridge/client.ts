import * as net from "net";
import * as vscode from "vscode";

type BridgeMode = "open" | "reveal";

type BridgeResponse = {
    ok: boolean;
    message: string;
};

export async function openAssetInEditor(assetPath: string, mode: BridgeMode): Promise<void> {
    const config = vscode.workspace.getConfiguration("puertsTool");
    const host = config.get<string>("bridgeHost", "127.0.0.1");
    const port = config.get<number>("bridgePort", 18777);

    try {
        const response = await sendBridgeCommand(host, port, { cmd: "openAsset", assetPath, mode });
        if (!response.ok) {
            vscode.window.showErrorMessage(`PuerTS bridge failed: ${response.message}`);
        }
    } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        vscode.window.showErrorMessage(`Cannot connect to PuerTS bridge at ${host}:${port}. ${message}`);
    }
}

function sendBridgeCommand(host: string, port: number, payload: Record<string, unknown>): Promise<BridgeResponse> {
    return new Promise((resolve, reject) => {
        const socket = net.createConnection({ host, port });
        let response = "";

        socket.setEncoding("utf8");
        socket.setTimeout(3000);
        socket.on("connect", () => socket.write(`${JSON.stringify(payload)}\n`));
        socket.on("data", (chunk: string) => {
            response += chunk;
            if (response.includes("\n")) {
                socket.end();
            }
        });
        socket.on("timeout", () => socket.destroy(new Error("request timed out")));
        socket.on("error", reject);
        socket.on("close", () => {
            if (!response.trim()) {
                reject(new Error("empty bridge response"));
                return;
            }

            try {
                const parsed = JSON.parse(response.trim()) as Partial<BridgeResponse>;
                resolve({ ok: Boolean(parsed.ok), message: parsed.message ?? "" });
            } catch {
                reject(new Error(`invalid bridge response: ${response.trim()}`));
            }
        });
    });
}
