#include <ui/icons.hpp>
#include "icons_lookup.hpp"

namespace Icons
{
    const wchar_t *for_file_part2([[maybe_unused]] const std::string &name,
                                 [[maybe_unused]] const std::string &ext)
    {
        if (name == ".travis.yml")
            return lang_travis;
        if (name == "appveyor.yml" || name == ".appveyor.yml")
            return lang_appveyor;
        if (name == ".gitlab-ci.yml")
            return lang_gitlab;
        if (name == "azure-pipelines.yml" || name == "azure-pipelines.yaml")
            return lang_azure;
        if (name == "earthfile")
            return lang_earthly;
        if (name == "tiltfile")
            return lang_tilt;
        if (name == "skaffold.yaml" || name == "skaffold.yml")
            return lang_skaffold;
        if (name == "chart.yaml" || name == "chart.yml" || name == "values.yaml" || name == "values.yml")
            return lang_helm;
        if (name == "kustomization.yaml" || name == "kustomization.yml")
            return lang_kustomize;
        if (name == ".devcontainer.json" || name == "devcontainer.json")
            return lang_devcontainer;
        if (ends_with(name, ".code-workspace"))
            return lang_code_workspace;
        if (name == "nix" || ends_with(name, ".nix"))
            return lang_nix;
        if (name == "flake.nix" || name == "flake.lock" || name == "default.nix" || name == "shell.nix")
            return lang_nix;
        if (name == "snapcraft.yaml")
            return lang_snapcraft;
        if (name == "gruntfile.js" || name == "gulpfile.js" || name == "gulpfile.ts")
            return lang_js;
        if (name == "rollup.config.js" || name == "rollup.config.ts" || name == "rollup.config.mjs")
            return lang_webpack;
        if (name == "karma.conf.js" || name == "protractor.conf.js")
            return lang_test;
        if (name == "mocha.opts" || name == ".mocharc.json" || name == ".mocharc.yml")
            return lang_test;
        if (name == "vitest.config.ts" || name == "vitest.config.js" || name == "vitest.config.mjs")
            return lang_test;
        if (name == "storybook" || starts_with(name, ".storybook"))
            return lang_storybook;

        // ---- extensions ----
        if (ext == ".c")
            return lang_c;
        if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c++" || ext == ".cp" || ext == ".ii")
            return lang_cpp;
        if (ext == ".h" || ext == ".hh")
            return lang_h;
        if (ext == ".hpp" || ext == ".hxx" || ext == ".h++" || ext == ".hp" || ext == ".tcc" ||
            ext == ".inl" || ext == ".inc")
            return lang_hpp;
        if (ext == ".m")
            return lang_objc;
        if (ext == ".mm")
            return lang_objc;
        if (ext == ".cs")
            return lang_csharp;
        if (ext == ".fs" || ext == ".fsi" || ext == ".fsx" || ext == ".fsscript")
            return lang_fsharp;
        if (ext == ".vb" || ext == ".vbs")
            return lang_batch;
        if (ext == ".py" || ext == ".pyw" || ext == ".pyi" || ext == ".pyx" || ext == ".pxd" ||
            ext == ".pxi" || ext == ".pyc" || ext == ".pyo" || ext == ".pyd")
            return python;
        if (ext == ".ipynb")
            return lang_ipynb;
        if (ext == ".rb" || ext == ".rbw" || ext == ".rake" || ext == ".gemspec" || ext == ".ru")
            return ruby;
        if (ext == ".js" || ext == ".mjs" || ext == ".cjs" || ext == ".es" || ext == ".es6")
            return lang_js;
        if (ext == ".ts" || ext == ".mts" || ext == ".cts")
            return lang_ts;
        if (ext == ".jsx")
            return lang_jsx;
        if (ext == ".tsx")
            return lang_tsx;
        if (ext == ".vue")
            return lang_vue;
        if (ext == ".svelte")
            return lang_svelte;
        if (ext == ".astro")
            return lang_astro;
        if (ext == ".coffee" || ext == ".litcoffee")
            return lang_coffeescript;
        if (ext == ".ls")
            return lang_livescript;
        if (ext == ".html" || ext == ".htm" || ext == ".shtml" || ext == ".xhtml" || ext == ".htmlx")
            return lang_html;
        if (ext == ".css")
            return lang_css;
        if (ext == ".scss")
            return lang_scss;
        if (ext == ".sass")
            return lang_sass;
        if (ext == ".less")
            return lang_less;
        if (ext == ".styl" || ext == ".stylus")
            return lang_stylus;
        if (ext == ".rs" || ext == ".rlib")
            return lang_rust;
        if (ext == ".go")
            return lang_go;
        if (ext == ".java" || ext == ".jav")
            return lang_java;
        if (ext == ".kt" || ext == ".kts" || ext == ".ktm")
            return lang_kotlin;
        if (ext == ".scala" || ext == ".sc")
            return lang_scala;
        if (ext == ".groovy" || ext == ".gvy" || ext == ".gy" || ext == ".gsh")
            return lang_groovy;
        if (ext == ".clj" || ext == ".cljs" || ext == ".cljc" || ext == ".edn")
            return lang_clojure;
        if (ext == ".lua" || ext == ".luau" || ext == ".rockspec")
            return lang_lua;
        if (ext == ".vim" || ext == ".vimrc" || ext == ".gvimrc")
            return lang_vim;
        if (ext == ".el" || ext == ".elc")
            return lang_emacs;
        if (ext == ".php" || ext == ".phtml" || ext == ".php3" || ext == ".php4" || ext == ".php5" ||
            ext == ".phps" || ext == ".phpt")
            return lang_php;
        if (ext == ".pl" || ext == ".pm" || ext == ".t" || ext == ".pod")
            return lang_perl;
        if (ext == ".raku" || ext == ".rakumod" || ext == ".rakudoc" || ext == ".rakutest" || ext == ".p6")
            return lang_raku;
        if (ext == ".r" || ext == ".rdata" || ext == ".rds" || ext == ".rda")
            return lang_r;
        if (ext == ".jl")
            return lang_julia;
        if (ext == ".mat")
            return lang_matlab_mat;
        if (ext == ".hs" || ext == ".lhs" || ext == ".hsc")
            return lang_haskell;
        if (ext == ".ml" || ext == ".mli" || ext == ".mll" || ext == ".mly")
            return lang_ocaml;
        if (ext == ".ex" || ext == ".exs" || ext == ".eex" || ext == ".heex" || ext == ".leex")
            return lang_elixir;
        if (ext == ".erl" || ext == ".hrl")
            return lang_erlang;
        if (ext == ".nim" || ext == ".nims" || ext == ".nimble")
            return lang_nim;
        if (ext == ".cr")
            return lang_crystal;
        if (ext == ".zig" || ext == ".zon")
            return lang_zig;
        if (ext == ".v" || ext == ".vv")
            return lang_v;
        if (ext == ".dart")
            return lang_dart;
        if (ext == ".swift")
            return lang_swift;
        if (ext == ".f" || ext == ".for" || ext == ".f90" || ext == ".f95" || ext == ".f03" || ext == ".f08")
            return lang_fortran;
        if (ext == ".cob" || ext == ".cbl" || ext == ".cpy")
            return lang_cobol;
        return nullptr;
    }
}
