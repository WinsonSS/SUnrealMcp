#!/usr/bin/env node

import { main } from "./index.js";

main(process.argv.slice(2)).catch((error) => {
    const payload = {
        ok: false,
        error: {
            code: "CLI_FATAL",
            message: error instanceof Error ? error.message : String(error),
        },
    };

    process.stderr.write(`${JSON.stringify(payload, null, 2)}\n`);
    process.exitCode = 1;
});
