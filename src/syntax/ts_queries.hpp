#pragma once
namespace ts_hl_detail
{
    constexpr const char *kQueryC = R"TS(
(comment) @comment
(string_literal) @string
(system_lib_string) @string
(char_literal) @string
(number_literal) @number
(primitive_type) @type
(type_identifier) @type
(preproc_include) @preprocessor
(preproc_def) @preprocessor
(preproc_function_def) @preprocessor
(preproc_call) @preprocessor
[
  "break" "case" "const" "continue" "default" "do" "else" "enum"
  "extern" "for" "if" "inline" "return" "sizeof" "static" "struct"
  "switch" "typedef" "union" "volatile" "while" "goto" "restrict"
] @keyword
(call_expression
  function: (identifier) @function)
(function_declarator
  declarator: (identifier) @function)
)TS";

    constexpr const char *kQueryLua = R"TS(
(comment) @comment
(string) @string
(number) @number
(true) @constant
(false) @constant
(nil) @constant
[
  "and" "do" "else" "elseif" "end" "for" "function"
  "goto" "if" "in" "local" "not" "or" "repeat" "return"
  "then" "until" "while"
] @keyword
(function_call
  name: (identifier) @function)
(function_declaration
  name: (identifier) @function)
)TS";

    constexpr const char *kQueryMarkdown = R"TS(
(atx_heading) @keyword
(setext_heading) @keyword
(fenced_code_block) @string
(indented_code_block) @string
(link_destination) @string
(link_title) @string
)TS";

    constexpr const char *kQueryPython = R"TS(
(comment) @comment
(string) @string
(escape_sequence) @string
(integer) @number
(float) @number
(true) @constant
(false) @constant
(none) @constant
[
  "and" "as" "assert" "async" "await" "break" "class" "continue"
  "def" "del" "elif" "else" "except" "finally" "for" "from"
  "global" "if" "import" "in" "is" "lambda" "nonlocal" "not"
  "or" "pass" "raise" "return" "try" "while" "with" "yield"
  "match" "case"
] @keyword
(function_definition
  name: (identifier) @function)
(call
  function: (identifier) @function)
(type) @type
)TS";

    constexpr const char *kQueryJS = R"TS(
(comment) @comment
(string) @string
(template_string) @string
(number) @number
(true) @constant
(false) @constant
(null) @constant
(undefined) @constant
[
  "as" "async" "await" "break" "case" "catch" "class" "const"
  "continue" "debugger" "default" "delete" "do" "else" "export"
  "extends" "finally" "for" "from" "function" "get" "if" "import"
  "in" "instanceof" "let" "new" "of" "return" "set" "static"
  "switch" "throw" "try" "typeof" "var" "void" "while" "with" "yield"
] @keyword
(call_expression
  function: (identifier) @function)
(function_declaration
  name: (identifier) @function)
(method_definition
  name: (property_identifier) @function)
)TS";

    constexpr const char *kQueryRust = R"TS(
(line_comment) @comment
(block_comment) @comment
(string_literal) @string
(raw_string_literal) @string
(integer_literal) @number
(float_literal) @number
(boolean_literal) @constant
[
  "as" "async" "await" "break" "const" "continue" "crate" "dyn"
  "else" "enum" "extern" "fn" "for" "if" "impl" "in" "let"
  "loop" "match" "mod" "move" "mut" "pub" "ref" "return" "self"
  "Self" "static" "struct" "super" "trait" "type" "unsafe" "use"
  "where" "while"
] @keyword
(call_expression
  function: (identifier) @function)
(function_item
  name: (identifier) @function)
(type_identifier) @type
(primitive_type) @type
)TS";

    constexpr const char *kQueryBash = R"TS(
(comment) @comment
(string) @string
(raw_string) @string
(number) @number
[
  "if" "then" "else" "elif" "fi" "case" "esac" "for" "select"
  "while" "until" "do" "done" "in" "function" "time" "coproc"
] @keyword
(command_name) @function
)TS";

    constexpr const char *kQueryJson = R"TS(
(comment) @comment
(string) @string
(number) @number
(true) @constant
(false) @constant
(null) @constant
)TS";
}
