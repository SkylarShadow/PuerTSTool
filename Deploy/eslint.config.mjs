import js from "@eslint/js";
import prettier from "eslint-config-prettier";
import globals from "globals";
import tseslint from "typescript-eslint";

export default tseslint.config(
    {
        ignores: [
            "Binaries/**",
            "Content/JavaScript/**",
            "Intermediate/**",
            "Saved/**",
            "node_modules/**",
        ],
    },
    js.configs.recommended,
    ...tseslint.configs.recommended,
    {
        files: ["TypeScript/**/*.ts"],
        languageOptions: {
            globals: {
                ...globals.node,
            },
        },
        rules: {
            "no-debugger": "warn",
            "no-eval": "warn",
            "no-var": "error",
            "prefer-const": "warn",
            "@typescript-eslint/no-empty-object-type": "off",
            "@typescript-eslint/no-unsafe-declaration-merging": "off",
            "@typescript-eslint/no-empty-function": "off",
            "@typescript-eslint/no-explicit-any": "off",
            "@typescript-eslint/no-unused-vars": [
                "warn",
                {
                    argsIgnorePattern: "^_",
                    varsIgnorePattern: "^_",
                },
            ],
            "@typescript-eslint/naming-convention": [
                "warn",
                {
                    selector: ["variable", "parameter"],
                    format: ["camelCase"],
                    leadingUnderscore: "allow",
                },
            ],
        },
    },
    prettier,
);
