import * as net from "net";

const UNREAL_HOST = "127.0.0.1";
const UNREAL_PORT = 55557;

export async function sendCommand(
    command: string,
    params: Record<string, unknown> = {}
): Promise<Record<string, unknown>> {
    return new Promise((resolve, reject) => {
        const socket = new net.Socket();
        socket.setNoDelay(true);

        const chunks: Buffer[] = [];

        socket.connect(UNREAL_PORT, UNREAL_HOST, () => {
            const payload = JSON.stringify({ type: command, params });
            socket.write(payload);
        });

        socket.on("data", (chunk: Buffer) => {
            chunks.push(chunk);
            const data = Buffer.concat(chunks).toString("utf-8");
            try {
                const response = JSON.parse(data) as Record<string, unknown>;
                socket.destroy();
                resolve(response);
            } catch {
                // 数据未接收完整，继续等待
            }
        });

        socket.setTimeout(5000, () => {
            const data = Buffer.concat(chunks).toString("utf-8");
            socket.destroy();
            if (data) {
                try {
                    resolve(JSON.parse(data) as Record<string, unknown>);
                    return;
                } catch {}
            }
            reject(new Error("Timeout receiving Unreal response"));
        });

        socket.on("error", (err: Error) => {
            reject(err);
        });
    });
}
