#include <ui/icons.hpp>
#include "icons_lookup.hpp"

namespace Icons
{
    const wchar_t *for_file_part1([[maybe_unused]] const std::string &name,
                                 [[maybe_unused]] const std::string &ext)
    {
        // ---- special filenames ----
        if (name == "makefile" || name == "gnumakefile" || name == "kbuild")
            return lang_makefile;
        if (name == "cmakelists.txt" || name == "cmake")
            return lang_cmake;
        if (name == "dockerfile" || starts_with(name, "dockerfile.") || name == "containerfile")
            return lang_dockerfile;
        if (name == "docker-compose.yml" || name == "docker-compose.yaml" ||
            name == "compose.yml" || name == "compose.yaml")
            return lang_compose;
        if (name == "vagrantfile")
            return lang_vagrantfile;
        if (name == "justfile" || name == ".justfile")
            return lang_justfile;
        if (name == "taskfile.yml" || name == "taskfile.yaml")
            return lang_taskfile;
        if (name == "procfile")
            return lang_procfile;
        if (name == "rakefile" || name == "gemfile" || name == "gemfile.lock")
            return lang_gemfile;
        if (name == "guardfile" || name == "capfile" || name == "thorfile" || name == "berksfile")
            return lang_rakefile;
        if (name == "brewfile")
            return lang_brewfile;
        if (name == "package.json" || name == "package-lock.json")
            return lang_package_json;
        if (name == "yarn.lock" || name == ".yarnrc" || name == ".yarnrc.yml")
            return lang_yarn;
        if (name == "pnpm-lock.yaml" || name == "pnpm-workspace.yaml")
            return lang_pnpm;
        if (name == "bun.lockb" || name == "bunfig.toml")
            return lang_bun;
        if (name == "deno.json" || name == "deno.jsonc")
            return lang_deno;
        if (name == "tsconfig.json" || starts_with(name, "tsconfig."))
            return lang_tsconfig;
        if (name == "jsconfig.json")
            return lang_jsconfig;
        if (name == ".eslintrc" || starts_with(name, ".eslintrc.") || name == "eslint.config.js" ||
            name == "eslint.config.mjs" || name == "eslint.config.cjs" || name == "eslint.config.ts")
            return lang_eslintrc;
        if (name == ".prettierrc" || starts_with(name, ".prettierrc.") || name == "prettier.config.js" ||
            name == "prettier.config.cjs" || name == "prettier.config.mjs")
            return lang_prettierrc;
        if (name == ".babelrc" || starts_with(name, ".babelrc.") || name == "babel.config.js" ||
            name == "babel.config.cjs" || name == "babel.config.json")
            return lang_babelrc;
        if (name == "webpack.config.js" || name == "webpack.config.ts" || name == "webpack.config.cjs")
            return lang_webpack;
        if (name == "vite.config.js" || name == "vite.config.ts" || name == "vite.config.mjs")
            return lang_vite;
        if (name == "next.config.js" || name == "next.config.mjs" || name == "next.config.ts")
            return lang_next;
        if (name == "nuxt.config.js" || name == "nuxt.config.ts")
            return lang_nuxt;
        if (name == "astro.config.mjs" || name == "astro.config.js" || name == "astro.config.ts")
            return lang_astro;
        if (name == "svelte.config.js" || name == "svelte.config.ts")
            return lang_sveltekit;
        if (name == "tailwind.config.js" || name == "tailwind.config.ts" || name == "tailwind.config.cjs")
            return lang_tailwind;
        if (name == "postcss.config.js" || name == "postcss.config.cjs" || name == "postcss.config.mjs")
            return lang_postcss;
        if (name == "jest.config.js" || name == "jest.config.ts" || name == "jest.config.cjs")
            return lang_jest;
        if (name == "cypress.config.js" || name == "cypress.config.ts")
            return lang_cypress;
        if (name == "playwright.config.js" || name == "playwright.config.ts")
            return lang_playwright;
        if (name == "cargo.toml" || name == "cargo.lock")
            return lang_cargo;
        if (name == "go.mod" || name == "go.sum")
            return lang_go_mod;
        if (name == "requirements.txt" || name == "requirements-dev.txt" || name == "constraints.txt")
            return lang_requirements;
        if (name == "pipfile" || name == "pipfile.lock")
            return lang_pipfile;
        if (name == "pyproject.toml" || name == "poetry.lock")
            return lang_poetry;
        if (name == "composer.json" || name == "composer.lock")
            return lang_composer;
        if (name == "mix.exs" || name == "mix.lock")
            return lang_mix;
        if (name == "pubspec.yaml" || name == "pubspec.lock")
            return lang_pubspec;
        if (name == "cartfile" || name == "cartfile.resolved")
            return lang_cartfile;
        if (name == "podfile" || name == "podfile.lock")
            return lang_podfile;
        if (name == "build.gradle" || name == "build.gradle.kts" || name == "settings.gradle" ||
            name == "settings.gradle.kts")
            return lang_build_gradle;
        if (name == "pom.xml")
            return lang_pom;
        if (name == "meson.build" || name == "meson_options.txt")
            return lang_cmake;
        if (name == "build.ninja" || name == "rules.ninja")
            return lang_ninja;
        if (name == "work.bazel" || name == "build.bazel" || name == "workspace" || name == "module.bazel")
            return lang_bazel;
        if (name == ".gitignore" || name == ".gitattributes" || name == ".gitmodules" ||
            name == ".gitkeep" || name == ".keep")
            return lang_git;
        if (name == ".editorconfig")
            return lang_editorconfig_file;
        if (name == ".env" || starts_with(name, ".env."))
            return lang_env;
        if (name == ".clang-format" || name == ".clang-tidy")
            return lang_clang_format;
        if (name == "compile_commands.json")
            return lang_compile_commands;
        if (name == "readme" || name == "readme.md" || name == "readme.txt" || name == "readme.rst" ||
            name == "readme.adoc")
            return lang_readme;
        if (name == "license" || name == "licence" || name == "copying" || name == "license.md" ||
            name == "licence.md" || name == "copying.md")
            return lang_license;
        if (name == "changelog" || name == "changelog.md" || name == "changes" || name == "changes.md" ||
            name == "history.md" || name == "news.md")
            return lang_changelog;
        if (name == "authors" || name == "authors.md" || name == "contributors" || name == "contributors.md")
            return lang_authors;
        if (name == "contributing" || name == "contributing.md")
            return lang_contributing;
        if (name == "security" || name == "security.md")
            return lang_security;
        if (name == "codeowners" || name == "code_of_conduct.md" || name == "code-of-conduct.md")
            return lang_codeowners;
        if (name == "todo" || name == "todo.md" || name == "todos.md")
            return lang_todo;
        if (name == "faq" || name == "faq.md")
            return lang_faq;
        if (name == "robots.txt")
            return lang_robots;
        if (name == "sitemap.xml" || name == "sitemap.txt")
            return lang_sitemap;
        if (name == "manifest.json" || name == "manifest.webmanifest")
            return lang_package;
        if (name == "sw.js" || name == "service-worker.js")
            return lang_js;
        if (name == "firebase.json" || name == ".firebaserc")
            return lang_firebase;
        if (name == "vercel.json" || name == "now.json")
            return lang_vercel;
        if (name == "netlify.toml")
            return lang_netlify;
        if (name == "prisma.schema" || ends_with(name, ".prisma"))
            return lang_prisma_schema;
        if (name == "schema.graphql" || name == "schema.gql")
            return lang_graphql_sdl;
        if (name == "openapi.yaml" || name == "openapi.yml" || name == "openapi.json" ||
            name == "swagger.yaml" || name == "swagger.yml" || name == "swagger.json")
            return lang_openapi;
        if (name == "jenkinsfile" || starts_with(name, "jenkinsfile."))
            return lang_jenkins;
        return nullptr;
    }
}
