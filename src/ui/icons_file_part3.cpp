#include <ui/icons.hpp>
#include "icons_lookup.hpp"

namespace Icons
{
    const wchar_t *for_file_part3([[maybe_unused]] const std::string &name,
                                 [[maybe_unused]] const std::string &ext)
    {
        if (ext == ".pas" || ext == ".pp" || ext == ".dpr" || ext == ".lpr")
            return lang_pascal;
        if (ext == ".asm" || ext == ".s" || ext == ".nasm" || ext == ".yasm")
            return lang_assembly;
        if (ext == ".wasm" || ext == ".wat")
            return lang_wasm;
        if (ext == ".sol")
            return lang_solidity;
        if (ext == ".graphql" || ext == ".gql")
            return lang_graphql;
        if (ext == ".prisma")
            return lang_prisma;
        if (ext == ".proto")
            return lang_proto;
        if (ext == ".thrift")
            return lang_thrift;
        if (ext == ".tf" || ext == ".tfvars" || ext == ".hcl")
            return lang_terraform;
        if (ext == ".pulumi")
            return lang_pulumi;
        if (ext == ".nix")
            return lang_nix;
        if (ext == ".d")
            return lang_d;
        if (ext == ".ada" || ext == ".adb" || ext == ".ads")
            return lang_ada;
        if (ext == ".lisp" || ext == ".lsp" || ext == ".cl" || ext == ".fasl")
            return lang_lisp;
        if (ext == ".scm" || ext == ".ss")
            return lang_scheme;
        if (ext == ".rkt" || ext == ".rktd" || ext == ".rktl")
            return lang_racket;
        if (ext == ".pro" || ext == ".P")
            return lang_prolog;
        if (ext == ".sml" || ext == ".sig" || ext == ".fun")
            return lang_sml;
        if (ext == ".tcl" || ext == ".tk")
            return lang_tcl;
        if (ext == ".awk" || ext == ".gawk" || ext == ".mawk")
            return lang_awk;
        if (ext == ".sed")
            return lang_sed;
        if (ext == ".hx" || ext == ".hxml")
            return lang_haxe;
        if (ext == ".vala" || ext == ".vapi")
            return lang_vala;
        if (ext == ".re" || ext == ".rei")
            return lang_reason;
        if (ext == ".res" || ext == ".resi")
            return lang_rescript;
        if (ext == ".elm")
            return lang_elm;
        if (ext == ".purs")
            return lang_purescript;
        if (ext == ".hack")
            return lang_hack;
        if (ext == ".bal")
            return lang_ballerina;
        if (ext == ".vhd" || ext == ".vhdl")
            return lang_vhdl;
        if (ext == ".sv" || ext == ".svh")
            return lang_systemverilog;
        if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".geom" || ext == ".comp" ||
            ext == ".tesc" || ext == ".tese")
            return lang_glsl;
        if (ext == ".hlsl" || ext == ".fx" || ext == ".fxh")
            return lang_hlsl;
        if (ext == ".wgsl")
            return lang_wgsl;
        if (ext == ".metal")
            return lang_metal;
        if (ext == ".cu" || ext == ".cuh")
            return lang_cuda;
        if (ext == ".cl")
            return lang_opencl;
        if (ext == ".shader" || ext == ".cginc" || ext == ".compute")
            return lang_shader;
        if (ext == ".gd" || ext == ".tscn" || ext == ".tres" || ext == ".godot")
            return lang_godot;
        if (ext == ".unity" || ext == ".prefab" || ext == ".asset" || ext == ".meta")
            return lang_unity;
        if (ext == ".uasset" || ext == ".umap")
            return lang_unreal;
        if (ext == ".blend" || ext == ".blend1")
            return lang_blender;
        if (ext == ".fbx" || ext == ".obj" || ext == ".stl" || ext == ".gltf" || ext == ".glb" ||
            ext == ".dae" || ext == ".3ds" || ext == ".max" || ext == ".ma" || ext == ".mb")
            return lang_3d;
        if (ext == ".toml")
            return lang_toml;
        if (ext == ".yml" || ext == ".yaml")
            return lang_yaml;
        if (ext == ".json" || ext == ".jsonc" || ext == ".json5" || ext == ".geojson" || ext == ".har")
            return json;
        if (ext == ".xml" || ext == ".xsl" || ext == ".xslt" || ext == ".xsd" || ext == ".dtd" ||
            ext == ".ent" || ext == ".svg")
            return lang_xml;
        if (ext == ".ini" || ext == ".cfg" || ext == ".conf" || ext == ".config" || ext == ".prefs" ||
            ext == ".properties" || ext == ".prop")
            return lang_ini;
        if (ext == ".env")
            return lang_env;
        if (ext == ".sql" || ext == ".ddl" || ext == ".dml" || ext == ".pgsql" || ext == ".mysql" ||
            ext == ".plsql" || ext == ".psql")
            return lang_sql;
        if (ext == ".sqlite" || ext == ".sqlite3" || ext == ".db" || ext == ".db3")
            return lang_sqlite;
        if (ext == ".bson")
            return lang_mongodb;
        if (ext == ".rdb")
            return lang_redis;
        if (ext == ".md" || ext == ".markdown" || ext == ".mdown" || ext == ".mkd" || ext == ".mkdn" ||
            ext == ".mdwn" || ext == ".mdx")
            return markdown;
        if (ext == ".rst" || ext == ".rest")
            return lang_rst;
        if (ext == ".adoc" || ext == ".asciidoc" || ext == ".asc")
            return lang_adoc;
        if (ext == ".org")
            return lang_orgmode;
        if (ext == ".textile")
            return lang_textile;
        if (ext == ".wiki" || ext == ".mediawiki")
            return lang_wiki;
        if (ext == ".tex" || ext == ".ltx")
            return lang_tex;
        if (ext == ".sty" || ext == ".cls" || ext == ".dtx" || ext == ".ins")
            return lang_sty;
        if (ext == ".bib" || ext == ".bst")
            return lang_bib;
        if (ext == ".sh" || ext == ".bash" || ext == ".zsh" || ext == ".fish" || ext == ".ksh" ||
            ext == ".csh" || ext == ".tcsh" || ext == ".command")
            return lang_sh;
        if (ext == ".ps1" || ext == ".psm1" || ext == ".psd1")
            return lang_powershell;
        if (ext == ".bat" || ext == ".cmd" || ext == ".btm")
            return lang_batch;
        if (ext == ".diff" || ext == ".patch")
            return lang_diff;
        if (ext == ".csv")
            return lang_csv;
        if (ext == ".tsv" || ext == ".tab")
            return lang_tsv;
        if (ext == ".xls" || ext == ".xlsx" || ext == ".xlsm" || ext == ".xlsb" || ext == ".xltx")
            return lang_excel;
        if (ext == ".doc" || ext == ".docx" || ext == ".docm" || ext == ".dotx")
            return lang_word;
        if (ext == ".ppt" || ext == ".pptx" || ext == ".pptm" || ext == ".potx")
            return lang_powerpoint;
        if (ext == ".odt")
            return lang_odt;
        if (ext == ".ods")
            return lang_ods;
        if (ext == ".odp")
            return lang_odp;
        return nullptr;
    }
}
